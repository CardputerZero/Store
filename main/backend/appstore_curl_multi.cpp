/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "appstore_curl_multi.hpp"

#ifdef APPSTORE_USE_LIBCURL
#include <curl/curl.h>
#endif

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace appstore {

#ifdef APPSTORE_USE_LIBCURL

namespace {

namespace fs = std::filesystem;

constexpr size_t kMaxConcurrentTransfers = 6;
constexpr int kPollTimeoutMs = 40;
constexpr int kCancelPollMs = 20;

enum WorkerState { kWorkerStarting = 0, kWorkerReady = 1, kWorkerFailed = 2 };

struct BatchJob;

// The single dedicated worker. It is allocated once and never destroyed: the
// CURLM handle must outlive every caller that might call curl_multi_wakeup, and
// the process must not block on a join during teardown.
struct Worker {
    std::atomic<int> state{kWorkerStarting};
    std::mutex init_mutex;
    std::condition_variable init_cv;
    std::mutex multi_mutex;
    CURLM *multi = nullptr;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::deque<std::shared_ptr<BatchJob>> queue;
};

struct BatchJob {
    std::vector<CurlDownload> downloads;
    // The worker only reads this job-owned flag. The waiting caller copies the
    // external cancel atomic into it, so returning at the deadline cannot leave
    // the worker holding a dangling pointer to a caller-owned atomic.
    std::atomic<bool> cancel_requested{false};
    int timeout_seconds = 30;
    std::mutex mutex;
    std::condition_variable done_cv;
    std::vector<bool> results;
    bool done = false;
};

struct TransferEntry {
    CURL *easy = nullptr;
    FILE *file = nullptr;
    struct curl_slist *headers = nullptr;
    std::string temporary;
    std::string destination;
    size_t body_bytes = 0;
    long status_code = 0;
    bool success = false;
};

size_t write_callback(char *data, size_t size, size_t nmemb, void *userdata)
{
    auto *entry = static_cast<TransferEntry *>(userdata);
    const size_t bytes = size * nmemb;
    if (!entry->file || bytes == 0) return bytes;
    if (std::fwrite(data, 1, bytes, entry->file) != bytes) return 0;
    entry->body_bytes += bytes;
    return bytes;
}

void release_easy(TransferEntry &entry)
{
    if (entry.easy) {
        curl_easy_cleanup(entry.easy);
        entry.easy = nullptr;
    }
    if (entry.headers) {
        curl_slist_free_all(entry.headers);
        entry.headers = nullptr;
    }
}

void close_file(TransferEntry &entry)
{
    if (entry.file) {
        std::fclose(entry.file);
        entry.file = nullptr;
    }
}

void remove_temporary(TransferEntry &entry)
{
    if (entry.temporary.empty()) return;
    std::error_code error;
    fs::remove(entry.temporary, error);
}

void discard_entry(TransferEntry &entry)
{
    close_file(entry);
    release_easy(entry);
    remove_temporary(entry);
    entry.success = false;
}

void finish_entry(TransferEntry &entry, bool success)
{
    close_file(entry);
    release_easy(entry);
    if (success) {
        std::error_code error;
        fs::rename(entry.temporary, entry.destination, error);
        success = !error;
    }
    if (!success) remove_temporary(entry);
    entry.success = success;
}

bool start_entry(CURLM *multi, const BatchJob &job, TransferEntry &entry, size_t index)
{
    const CurlDownload &download = job.downloads[index];
    entry.destination = download.destination;
    entry.temporary = download.destination + ".tmp";
    std::error_code error;
    const fs::path destination(download.destination);
    if (destination.has_parent_path()) fs::create_directories(destination.parent_path(), error);
    entry.file = std::fopen(entry.temporary.c_str(), "wb");
    if (!entry.file) return false;
    entry.easy = curl_easy_init();
    if (!entry.easy) {
        discard_entry(entry);
        return false;
    }
    entry.headers = curl_slist_append(nullptr, "Cache-Control: no-cache");
    if (entry.headers) curl_easy_setopt(entry.easy, CURLOPT_HTTPHEADER, entry.headers);

    const long timeout_ms = static_cast<long>(std::max(1, job.timeout_seconds)) * 1000L;
    const long connect_ms = std::min(10000L, timeout_ms);
    curl_easy_setopt(entry.easy, CURLOPT_URL, download.url.c_str());
    curl_easy_setopt(entry.easy, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(entry.easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(entry.easy, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(entry.easy, CURLOPT_USERAGENT, "CardputerZero-AppStore/0.1");
    curl_easy_setopt(entry.easy, CURLOPT_CONNECTTIMEOUT_MS, connect_ms);
    curl_easy_setopt(entry.easy, CURLOPT_TIMEOUT_MS, timeout_ms);
    curl_easy_setopt(entry.easy, CURLOPT_WRITEFUNCTION, &write_callback);
    curl_easy_setopt(entry.easy, CURLOPT_WRITEDATA, &entry);
    curl_easy_setopt(entry.easy, CURLOPT_PRIVATE, reinterpret_cast<char *>(&entry));
    // Proxy discovery is left to libcurl: it honours http_proxy, https_proxy,
    // all_proxy and no_proxy (including HTTPS CONNECT tunneling) itself.
    if (curl_multi_add_handle(multi, entry.easy) != CURLM_OK) {
        discard_entry(entry);
        return false;
    }
    return true;
}

void drain_completed(CURLM *multi, size_t &active)
{
    int remaining = 0;
    while (CURLMsg *message = curl_multi_info_read(multi, &remaining)) {
        if (message->msg != CURLMSG_DONE) continue;
        CURL *easy = message->easy_handle;
        char *private_data = nullptr;
        curl_easy_getinfo(easy, CURLINFO_PRIVATE, &private_data);
        auto *entry = reinterpret_cast<TransferEntry *>(private_data);
        curl_multi_remove_handle(multi, easy);
        if (!entry) {
            curl_easy_cleanup(easy);
            continue;
        }
        const CURLcode result = message->data.result;
        curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &entry->status_code);
        entry->easy = nullptr;
        curl_easy_cleanup(easy);
        const bool success = result == CURLE_OK && entry->status_code >= 200 &&
                             entry->status_code < 300 && entry->body_bytes > 0;
        finish_entry(*entry, success);
        if (active > 0) --active;
    }
}

void abort_transfers(CURLM *multi, std::vector<TransferEntry> &entries, size_t &active)
{
    for (auto &entry : entries) {
        if (entry.easy) {
            curl_multi_remove_handle(multi, entry.easy);
            curl_easy_cleanup(entry.easy);
            entry.easy = nullptr;
        }
        discard_entry(entry);
    }
    active = 0;
}

void run_job(CURLM *multi, BatchJob &job)
{
    const size_t count = job.downloads.size();
    job.results.assign(count, false);
    std::vector<TransferEntry> entries(count);
    size_t next = 0;
    size_t active = 0;
    const long long timeout_seconds = std::max(1, job.timeout_seconds);
    // Jobs are started in waves of six; allow every wave its own timeout before
    // aborting the whole batch.
    const long long waves = static_cast<long long>((count + kMaxConcurrentTransfers - 1) /
                                                   kMaxConcurrentTransfers);
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(timeout_seconds * std::max<long long>(1, waves) + 15);
    try {
        for (;;) {
            if (job.cancel_requested.load()) {
                abort_transfers(multi, entries, active);
                break;
            }
            while (active < kMaxConcurrentTransfers && next < count) {
                if (start_entry(multi, job, entries[next], next)) ++active;
                ++next;
            }
            int running = 0;
            curl_multi_perform(multi, &running);
            drain_completed(multi, active);
            if (next >= count && active == 0) break;
            if (std::chrono::steady_clock::now() >= deadline) {
                abort_transfers(multi, entries, active);
                break;
            }
            curl_multi_poll(multi, nullptr, 0, running > 0 ? kPollTimeoutMs : 1, nullptr);
        }
    } catch (...) {
        abort_transfers(multi, entries, active);
    }
    for (size_t i = 0; i < count; ++i) job.results[i] = entries[i].success;
}

void worker_main(Worker *worker)
{
    CURLM *multi = curl_multi_init();
    {
        std::lock_guard<std::mutex> lock(worker->init_mutex);
        if (multi) {
            std::lock_guard<std::mutex> multi_lock(worker->multi_mutex);
            worker->multi = multi;
            worker->state.store(kWorkerReady);
        } else {
            worker->state.store(kWorkerFailed);
        }
    }
    worker->init_cv.notify_all();
    if (!multi) return;
    // The worker and its CURLM handle live for the rest of the process, so the
    // handle is never cleaned up and curl_multi_wakeup cannot race a teardown.
    for (;;) {
        std::shared_ptr<BatchJob> job;
        {
            std::unique_lock<std::mutex> lock(worker->queue_mutex);
            worker->queue_cv.wait(lock, [worker]() { return !worker->queue.empty(); });
            job = std::move(worker->queue.front());
            worker->queue.pop_front();
        }
        run_job(multi, *job);
        {
            std::lock_guard<std::mutex> lock(job->mutex);
            job->done = true;
        }
        job->done_cv.notify_all();
    }
}

Worker *worker_instance()
{
    static std::once_flag once;
    static Worker *worker = nullptr;
    std::call_once(once, []() {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return;
        Worker *created = new Worker();
        try {
            std::thread(worker_main, created).detach();
        } catch (...) {
            delete created;
            return;
        }
        std::unique_lock<std::mutex> lock(created->init_mutex);
        created->init_cv.wait_for(lock, std::chrono::seconds(2),
                                  [created]() { return created->state.load() != kWorkerStarting; });
        worker = created;
    });
    return worker;
}

}  // namespace

std::vector<bool> curl_multi_download_batch(const std::vector<CurlDownload> &downloads,
                                            const std::atomic<bool> *cancel, int timeout_seconds)
{
    if (downloads.empty()) return {};
    Worker *worker = worker_instance();
    if (!worker || worker->state.load() != kWorkerReady) return {};

    auto job = std::make_shared<BatchJob>();
    job->downloads = downloads;
    job->timeout_seconds = std::max(1, timeout_seconds);
    {
        std::lock_guard<std::mutex> lock(worker->queue_mutex);
        worker->queue.push_back(job);
    }
    worker->queue_cv.notify_one();

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::seconds(static_cast<long long>(job->timeout_seconds) + 30);
    std::unique_lock<std::mutex> lock(job->mutex);
    while (!job->done) {
        job->done_cv.wait_for(lock, std::chrono::milliseconds(kCancelPollMs));
        if (job->done) break;
        if (cancel && cancel->load()) {
            job->cancel_requested.store(true);
            // Wake the worker out of curl_multi_poll so it aborts the active
            // handles immediately instead of waiting for the poll timeout.
            std::lock_guard<std::mutex> multi_lock(worker->multi_mutex);
            if (worker->multi) curl_multi_wakeup(worker->multi);
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            // The worker must not keep using the caller-owned atomic after this
            // return; stop it through the job-owned flag and let it clean up.
            job->cancel_requested.store(true);
            std::lock_guard<std::mutex> multi_lock(worker->multi_mutex);
            if (worker->multi) curl_multi_wakeup(worker->multi);
            return std::vector<bool>(downloads.size(), false);
        }
    }
    return job->results;
}

bool curl_multi_available()
{
    Worker *worker = worker_instance();
    return worker && worker->state.load() == kWorkerReady;
}

#else

std::vector<bool> curl_multi_download_batch(const std::vector<CurlDownload> &downloads,
                                            const std::atomic<bool> *cancel, int timeout_seconds)
{
    (void)downloads;
    (void)cancel;
    (void)timeout_seconds;
    return {};
}

bool curl_multi_available()
{
    return false;
}

#endif

}  // namespace appstore
