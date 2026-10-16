/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

namespace appstore_ui {

// Keep the reading position across redraws of the system/status bars.
// Long text bounces like APPLaunch's selected title: hold, scroll to the end,
// hold, scroll back. Speed and clamps match LVGL's LONG_SCROLL default.
struct HorizontalMarquee {
    static constexpr uint32_t kPauseMs = 2000;
    static constexpr uint32_t kPixelsPerSecond = 40;
    static constexpr uint32_t kMinTravelMs = 300;
    static constexpr uint32_t kMaxTravelMs = 10000;

    std::string key;
    uint32_t started_at = 0;

    void select(const std::string &value, uint32_t now)
    {
        if (key == value) return;
        key = value;
        started_at = now;
    }

    int offset(uint32_t now, int distance) const
    {
        if (distance <= 0) return 0;
        const uint32_t travel = std::clamp<uint32_t>(
            static_cast<uint32_t>(static_cast<uint64_t>(distance) * 1000 / kPixelsPerSecond),
            kMinTravelMs, kMaxTravelMs);
        const uint32_t phase = static_cast<uint32_t>(now - started_at) %
                               (2 * (kPauseMs + travel));
        const auto moved = [&](uint32_t elapsed) {
            return static_cast<int>(static_cast<uint64_t>(elapsed) * distance / travel);
        };
        if (phase < kPauseMs) return 0;
        if (phase < kPauseMs + travel) return -moved(phase - kPauseMs);
        if (phase < 2 * kPauseMs + travel) return -distance;
        return moved(phase - 2 * kPauseMs - travel) - distance;
    }
};

} // namespace appstore_ui
