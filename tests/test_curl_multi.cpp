/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

// Direct tests for the dedicated libcurl multi worker used by the registry and
// artwork network layer. A local HTTP server verifies downloads, result order,
// 4xx/empty-body failures, bounded concurrency and cancellation while a request
// is already connected but has not received its response yet.

#include "../main/backend/appstore_curl_multi.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef APPSTORE_USE_LIBCURL

int main()
{
    std::cout << "curl multi worker tests skipped: libcurl transport not compiled in\n";
    return 0;
}

#else

namespace {

namespace fs = std::filesystem;

int checks = 0;
#define CHECK(condition)                                                                      \
    do {                                                                                      \
        ++checks;                                                                             \
        if (!(condition)) {                                                                   \
            std::cerr << "FAIL " << __LINE__ << ": " #condition "\n";                         \
            std::exit(1);                                                                     \
        }                                                                                     \
    } while (0)

std::string read_file(const fs::path &path)
{
    std::ifstream input(path, std::ios::binary);
    std::ostringstream contents;
    contents << input.rdbuf();
    return contents.str();
}

std::string read_request(int fd)
{
    std::string request;
    char buffer[1024];
    while (request.find("\r\n\r\n") == std::string::npos) {
        const ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
        if (bytes <= 0) break;
        request.append(buffer, static_cast<size_t>(bytes));
        if (request.size() > 16384) break;
    }
    return request;
}

void send_all(int fd, const std::string &data)
{
    size_t offset = 0;
    while (offset < data.size()) {
        const ssize_t bytes = send(fd, data.data() + offset, data.size() - offset, MSG_NOSIGNAL);
        if (bytes <= 0) return;
        offset += static_cast<size_t>(bytes);
    }
}

// Minimal thread-per-connection HTTP/1.1 server. It records the highest number
// of simultaneous requests so the test can assert the worker's six-transfer cap.
class LocalHttpServer {
public:
    LocalHttpServer()
    {
        listener_ = socket(AF_INET, SOCK_STREAM, 0);
        CHECK(listener_ >= 0);
        int reuse = 1;
        setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        CHECK(bind(listener_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0);
        CHECK(listen(listener_, 64) == 0);
        socklen_t length = sizeof(address);
        CHECK(getsockname(listener_, reinterpret_cast<sockaddr *>(&address), &length) == 0);
        port_ = ntohs(address.sin_port);
        accept_thread_ = std::thread([this]() { accept_loop(); });
    }

    ~LocalHttpServer() { stop(); }

    void stop()
    {
        if (stopped_.exchange(true)) return;
        shutdown(listener_, SHUT_RDWR);
        close(listener_);
        if (accept_thread_.joinable()) accept_thread_.join();
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (handlers_.load() > 0 && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    std::string url(const std::string &path) const
    {
        return "http://127.0.0.1:" + std::to_string(port_) + path;
    }

    int max_concurrency() const { return max_concurrency_.load(); }
    int hang_connections() const { return hang_connections_.load(); }

private:
    static void pause(int milliseconds)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    void accept_loop()
    {
        for (;;) {
            const int client = accept(listener_, nullptr, nullptr);
            if (client < 0) return;
            ++handlers_;
            std::thread([this, client]() { serve(client); }).detach();
        }
    }

    void wait_for_stop(int timeout_ms)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
        while (!stopped_.load() && std::chrono::steady_clock::now() < deadline) pause(10);
    }

    void respond(int client, const std::string &status, const std::string &body)
    {
        std::ostringstream response;
        response << "HTTP/1.1 " << status << "\r\n"
                 << "Content-Length: " << body.size() << "\r\n"
                 << "Connection: close\r\n\r\n"
                 << body;
        send_all(client, response.str());
    }

    void serve(int client)
    {
        const std::string request = read_request(client);
        const size_t line_end = request.find("\r\n");
        std::string path;
        if (line_end != std::string::npos) {
            std::istringstream line(request.substr(0, line_end));
            std::string method, target, version;
            line >> method >> target >> version;
            path = target;
        }
        const int current = ++active_;
        int previous = max_concurrency_.load();
        while (current > previous && !max_concurrency_.compare_exchange_weak(previous, current)) {
        }
        if (path == "/slow") {
            wait_for_stop(150);
            respond(client, "200 OK", "slow-body");
        } else if (path == "/ok1") {
            respond(client, "200 OK", "first-body");
        } else if (path == "/ok2") {
            respond(client, "200 OK", "second-body");
        } else if (path == "/empty") {
            respond(client, "200 OK", "");
        } else if (path == "/missing") {
            respond(client, "404 Not Found", "missing-body");
        } else if (path == "/hang") {
            // Accept the connection but never answer: the client is blocked in
            // the recv phase, which is exactly the case cancellation must abort.
            ++hang_connections_;
            wait_for_stop(30000);
        } else {
            respond(client, "500 Internal Server Error", "unexpected-path");
        }
        --active_;
        close(client);
        --handlers_;
    }

    int listener_ = -1;
    int port_ = 0;
    std::atomic<bool> stopped_{false};
    std::atomic<int> active_{0};
    std::atomic<int> max_concurrency_{0};
    std::atomic<int> hang_connections_{0};
    std::atomic<int> handlers_{0};
    std::thread accept_thread_;
};

void batch_cases(LocalHttpServer &server, const fs::path &root)
{
    CHECK(appstore::curl_multi_available());
    CHECK(appstore::curl_multi_download_batch({}, nullptr, 5).empty());

    fs::create_directories(root / "nested");
    const std::vector<appstore::CurlDownload> downloads = {
        {server.url("/ok1"), (root / "nested/one.bin").string()},
        {server.url("/ok2"), (root / "two.bin").string()},
    };
    const std::vector<bool> results = appstore::curl_multi_download_batch(downloads, nullptr, 10);
    CHECK(results.size() == downloads.size());
    CHECK(results[0]);
    CHECK(results[1]);
    CHECK(read_file(root / "nested/one.bin") == "first-body");
    CHECK(read_file(root / "two.bin") == "second-body");
    CHECK(!fs::exists(root / "nested/one.bin.tmp"));
    CHECK(!fs::exists(root / "two.bin.tmp"));

    const std::vector<bool> failed = appstore::curl_multi_download_batch(
        {{server.url("/missing"), (root / "missing.bin").string()},
         {server.url("/empty"), (root / "empty.bin").string()}},
        nullptr, 10);
    CHECK(failed.size() == 2);
    CHECK(!failed[0]);
    CHECK(!failed[1]);
    CHECK(!fs::exists(root / "missing.bin"));
    CHECK(!fs::exists(root / "missing.bin.tmp"));
    CHECK(!fs::exists(root / "empty.bin"));
    CHECK(!fs::exists(root / "empty.bin.tmp"));
}

void concurrency_cases(LocalHttpServer &server, const fs::path &root)
{
    std::vector<appstore::CurlDownload> downloads;
    for (int index = 0; index < 9; ++index) {
        downloads.push_back(
            {server.url("/slow"), (root / ("slow-" + std::to_string(index) + ".bin")).string()});
    }
    const std::vector<bool> results = appstore::curl_multi_download_batch(downloads, nullptr, 10);
    CHECK(results.size() == downloads.size());
    for (bool result : results) CHECK(result);
    CHECK(server.max_concurrency() >= 2);
    CHECK(server.max_concurrency() <= 6);
}

void concurrent_submit_cases(LocalHttpServer &server, const fs::path &root)
{
    std::vector<bool> first;
    std::vector<bool> second;
    std::thread one([&]() {
        first = appstore::curl_multi_download_batch({{server.url("/ok1"), (root / "submit-one.bin").string()}},
                                                    nullptr, 10);
    });
    std::thread two([&]() {
        second = appstore::curl_multi_download_batch({{server.url("/ok2"), (root / "submit-two.bin").string()}},
                                                     nullptr, 10);
    });
    one.join();
    two.join();
    CHECK(first.size() == 1 && first[0]);
    CHECK(second.size() == 1 && second[0]);
    CHECK(read_file(root / "submit-one.bin") == "first-body");
    CHECK(read_file(root / "submit-two.bin") == "second-body");
}

void cancellation_cases(LocalHttpServer &server, const fs::path &root)
{
    std::atomic<bool> cancel{false};
    const fs::path destination = root / "cancelled.bin";
    std::vector<bool> results;
    std::chrono::steady_clock::duration total;
    std::thread fetch([&]() {
        const auto started = std::chrono::steady_clock::now();
        results = appstore::curl_multi_download_batch({{server.url("/hang"), destination.string()}},
                                                      &cancel, 30);
        total = std::chrono::steady_clock::now() - started;
    });

    // Wait until the server has accepted the connection, so the request is
    // definitely blocked in the recv phase rather than still connecting.
    const auto accepted = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (server.hang_connections() == 0 && std::chrono::steady_clock::now() < accepted) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    CHECK(server.hang_connections() > 0);

    const auto cancel_at = std::chrono::steady_clock::now();
    cancel = true;
    fetch.join();
    const auto cancel_latency =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - cancel_at);
    const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total);
    std::cout << "cancel latency: " << cancel_latency.count() << " ms (total " << total_ms.count()
              << " ms)\n";
    CHECK(results.size() == 1);
    CHECK(!results[0]);
    CHECK(cancel_latency.count() < 1000);
    CHECK(!fs::exists(destination));
    CHECK(!fs::exists(destination.string() + ".tmp"));
}

}  // namespace

int main()
{
    const fs::path root = fs::temp_directory_path() / ("appstore-curl-multi-" + std::to_string(getpid()));
    fs::remove_all(root);
    fs::create_directories(root);
    {
        LocalHttpServer server;
        batch_cases(server, root);
        concurrency_cases(server, root);
        concurrent_submit_cases(server, root);
        cancellation_cases(server, root);
    }
    fs::remove_all(root);
    std::cout << "curl multi worker tests passed: " << checks << " checks\n";
    return 0;
}

#endif
