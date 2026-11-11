/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

// Streaming package-manager process helpers.
//
// The package helper must keep the UI informed while apt-get downloads large
// dependency sets, and it must not inherit a localized apt/dpkg environment.
// Everything here is header-only so the progress parser and the stream runner
// can be unit-tested without root privileges.

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif

namespace appstore {
namespace package_stream {

constexpr int kHeartbeatIntervalSeconds = 5;
constexpr size_t kMaxPendingLineBytes = 8192;

struct ProgressUpdate {
    bool recognized = false;
    bool error = false;
    std::string phase;
    std::string message;
    int percent = -1;
};

inline void apply_child_locale()
{
    setenv("LC_ALL", "C", 1);
    setenv("LANG", "C", 1);
    unsetenv("LANGUAGE");
}

inline std::string trim_line(std::string line)
{
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
    return line;
}

inline int parse_percent(const std::string &text)
{
    if (text.empty()) return -1;
    char *end = nullptr;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (!end || end == text.c_str() || value < 0) return -1;
    return static_cast<int>(std::min<long>(value, 100));
}

inline ProgressUpdate parse_progress_line(const std::string &raw)
{
    ProgressUpdate update;
    const std::string line = trim_line(raw);
    if (line.empty()) return update;
    if (line.rfind("dlstatus:", 0) == 0) {
        // dlstatus:<already-downloaded>:<percent>:<message> (APT
        // doc/progress-reporting.md); keep the message free of the counters.
        const size_t downloaded_end = line.find(':', 9);
        if (downloaded_end == std::string::npos) return update;
        const size_t percent_end = line.find(':', downloaded_end + 1);
        if (percent_end == std::string::npos) return update;
        update.percent = parse_percent(
            line.substr(downloaded_end + 1, percent_end - downloaded_end - 1));
        update.phase = "dependencies";
        update.message = "Downloading dependencies: " + line.substr(percent_end + 1);
        update.recognized = true;
        return update;
    }
    if (line.rfind("pmstatus:", 0) == 0) {
        // pmstatus:<package>:<percent>:<message>
        const size_t package_end = line.find(':', 9);
        if (package_end == std::string::npos) return update;
        const size_t percent_end = line.find(':', package_end + 1);
        if (percent_end == std::string::npos) return update;
        update.percent = parse_percent(line.substr(package_end + 1, percent_end - package_end - 1));
        update.phase = "package-manager";
        update.message = line.substr(percent_end + 1);
        update.recognized = true;
        return update;
    }
    if (line.rfind("pmerror:", 0) == 0 || line.rfind("error:", 0) == 0 ||
        line.rfind("E:", 0) == 0 || line.rfind("dpkg: error", 0) == 0) {
        update.error = true;
        update.message = line;
        return update;
    }
    if (line.rfind("Get:", 0) == 0) {
        update.phase = "dependencies";
        update.message = line;
        update.recognized = true;
        return update;
    }
    for (const char *prefix : {"Unpacking ", "Setting up ", "Removing ", "Preparing to unpack "}) {
        if (line.rfind(prefix, 0) == 0) {
            update.phase = "package-manager";
            update.message = line;
            update.recognized = true;
            return update;
        }
    }
    return update;
}

inline std::string format_progress_line(const ProgressUpdate &update)
{
    if (!update.recognized) return "";
    std::string message = update.message;
    for (char &ch : message) {
        if (ch == '\t' || ch == '\r' || ch == '\n') ch = ' ';
    }
    return "PROGRESS\t" + update.phase + "\t0\t0\t" + std::to_string(update.percent) + "\t" +
        message + "\n";
}

inline std::string timeout_failure_message(int timeout_seconds)
{
    return "package manager timed out after " + std::to_string(timeout_seconds) +
        " seconds; it was stopped before completion";
}

inline std::string truncate_error_line(std::string line, size_t limit = 200)
{
    line = trim_line(std::move(line));
    if (line.size() > limit) line = line.substr(0, limit) + "...";
    return line;
}

inline std::string failure_message(int exit_code, const std::string &last_error)
{
    std::string message = "package manager failed with exit code " + std::to_string(exit_code);
    if (!last_error.empty()) message += ": " + truncate_error_line(last_error);
    return message;
}

inline void sleep_milliseconds(long milliseconds)
{
    struct timespec delay {};
    delay.tv_sec = milliseconds / 1000;
    delay.tv_nsec = (milliseconds % 1000) * 1000000L;
    while (nanosleep(&delay, &delay) != 0 && errno == EINTR) {}
}

// Run a command with stdout/stderr captured into one pipe.  Every complete
// output line is handed to on_line while the process runs; on_heartbeat is
// invoked after kHeartbeatIntervalSeconds without output so the UI never goes
// silent during a slow dependency download.  Only the child's own output is
// consumed (never echoed) to keep the job output buffer small.
//
// Returns the same codes as run_bounded(): the exit status of the command,
// 128+signal if it was killed by a signal, 124 on timeout, -1 on setup errors.
inline int run_streaming_command(const std::vector<std::string> &args, int timeout_seconds,
                                 const std::function<void(const std::string &)> &on_line,
                                 const std::function<void()> &on_heartbeat = {})
{
    if (args.empty()) return -1;
    int pipe_fds[2];
    if (::pipe(pipe_fds) != 0) return -1;
    const pid_t pid = ::fork();
    if (pid < 0) {
        ::close(pipe_fds[0]);
        ::close(pipe_fds[1]);
        return -1;
    }
    if (pid == 0) {
        ::close(pipe_fds[0]);
        ::dup2(pipe_fds[1], STDOUT_FILENO);
        ::dup2(pipe_fds[1], STDERR_FILENO);
        ::close(pipe_fds[1]);
        setpgid(0, 0);
#ifdef __linux__
        const pid_t parent = getppid();
        prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (getppid() != parent) _exit(128 + SIGTERM);
#endif
        apply_child_locale();
        std::vector<const char *> view;
        for (const auto &item : args) view.push_back(item.c_str());
        view.push_back(nullptr);
        execvp(view[0], const_cast<char *const *>(view.data()));
        _exit(127);
    }
    setpgid(pid, pid);
    ::close(pipe_fds[1]);
    const int read_flags = ::fcntl(pipe_fds[0], F_GETFL, 0);
    if (read_flags >= 0) ::fcntl(pipe_fds[0], F_SETFL, read_flags | O_NONBLOCK);

    const auto started = std::chrono::steady_clock::now();
    const auto deadline = started + std::chrono::seconds(std::max(1, timeout_seconds));
    auto last_activity = started;
    auto last_heartbeat = started;
    auto drain_deadline = std::chrono::steady_clock::time_point::max();
    bool child_exited = false;
    bool wait_failed = false;
    bool pipe_open = true;
    int status = 0;
    std::string pending;

    auto flush_pending = [&]() {
        if (pending.empty()) return;
        on_line(pending);
        pending.clear();
    };

    // Stop the child the same way for both timeout paths: give it a short grace
    // period after SIGTERM, then force SIGKILL and reap it.
    auto terminate_child = [&]() {
        if (::kill(-pid, SIGTERM) != 0) ::kill(pid, SIGTERM);
        const auto term_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (std::chrono::steady_clock::now() < term_deadline) {
            const pid_t terminated = ::waitpid(pid, &status, WNOHANG);
            if (terminated == pid || (terminated < 0 && errno != EINTR)) {
                child_exited = true;
                break;
            }
            sleep_milliseconds(50);
        }
        if (!child_exited) {
            ::kill(-pid, SIGKILL);
            ::kill(pid, SIGKILL);
            while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
            child_exited = true;
        }
    };

    while (pipe_open) {
        const auto now = std::chrono::steady_clock::now();
        if (!child_exited) {
            const pid_t waited = ::waitpid(pid, &status, WNOHANG);
            if (waited == pid) {
                child_exited = true;
                drain_deadline = now + std::chrono::milliseconds(1000);
            } else if (waited < 0 && errno != EINTR) {
                child_exited = true;
                wait_failed = true;
                drain_deadline = now + std::chrono::milliseconds(1000);
            } else if (now >= deadline) {
                terminate_child();
                flush_pending();
                ::close(pipe_fds[0]);
                return 124;
            }
        }

        struct pollfd descriptor {};
        descriptor.fd = pipe_fds[0];
        descriptor.events = POLLIN;
        const int ready = ::poll(&descriptor, 1, 200);
        if (ready > 0) {
            char buffer[4096];
            for (;;) {
                const ssize_t count = ::read(pipe_fds[0], buffer, sizeof(buffer));
                if (count > 0) {
                    last_activity = std::chrono::steady_clock::now();
                    pending.append(buffer, static_cast<size_t>(count));
                    size_t newline = 0;
                    while ((newline = pending.find('\n')) != std::string::npos) {
                        std::string line = pending.substr(0, newline);
                        pending.erase(0, newline + 1);
                        line = trim_line(std::move(line));
                        if (!line.empty()) on_line(line);
                    }
                    if (pending.size() > kMaxPendingLineBytes) flush_pending();
                    continue;
                }
                if (count == 0) {
                    pipe_open = false;
                } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                    pipe_open = false;
                }
                break;
            }
        } else if (ready < 0 && errno != EINTR) {
            pipe_open = false;
        }

        const auto after = std::chrono::steady_clock::now();
        if (pipe_open && !child_exited && on_heartbeat &&
            after - last_activity >= std::chrono::seconds(kHeartbeatIntervalSeconds) &&
            after - last_heartbeat >= std::chrono::seconds(kHeartbeatIntervalSeconds)) {
            on_heartbeat();
            last_heartbeat = after;
        }
        if (child_exited && after >= drain_deadline) break;
    }

    ::close(pipe_fds[0]);
    if (wait_failed) return -1;
    // A command can close stdout/stderr but keep running; the timeout still
    // applies after the pipe closes, otherwise this wait would block forever.
    while (!child_exited) {
        const pid_t waited = ::waitpid(pid, &status, WNOHANG);
        if (waited == pid) {
            child_exited = true;
            break;
        }
        if (waited < 0 && errno != EINTR) {
            wait_failed = true;
            child_exited = true;
            break;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            terminate_child();
            flush_pending();
            return 124;
        }
        sleep_milliseconds(50);
    }
    flush_pending();
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return -1;
}

}  // namespace package_stream
}  // namespace appstore
