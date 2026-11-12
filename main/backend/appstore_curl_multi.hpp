/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

// Dedicated libcurl multi worker used by the registry/artwork network layer.
//
// The worker thread owns the only CURLM handle in the process; callers from any
// thread only submit a batch and wait for its result. This keeps libcurl off
// the UI/event-loop thread entirely and makes cancellation effective while a
// transfer is still connecting or waiting for its response.

#include <atomic>
#include <string>
#include <vector>

namespace appstore {

// One HTTP GET whose body is written to `destination` only when the response is
// a complete 2xx body.
struct CurlDownload {
    std::string url;
    std::string destination;
};

// Downloads a batch of requests and returns one success flag per entry, in the
// same order as `downloads`. At most six transfers run concurrently.
//
// `cancel` is polled by the waiting caller (which wakes the worker with
// curl_multi_wakeup) and by the worker itself, so a cancelled batch aborts
// during connect/transfer, removes its handles and temporary files, and marks
// every entry as failed within a few tens of milliseconds.
//
// Returns an empty vector when the libcurl transport is not compiled in or the
// worker could not start; callers may then fall back to the curl subprocess
// transport.
std::vector<bool> curl_multi_download_batch(const std::vector<CurlDownload> &downloads,
                                            const std::atomic<bool> *cancel = nullptr,
                                            int timeout_seconds = 30);

// True when the libcurl multi transport is compiled in and its worker is ready.
bool curl_multi_available();

}  // namespace appstore
