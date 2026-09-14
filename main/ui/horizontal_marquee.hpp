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
struct HorizontalMarquee {
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
        constexpr uint32_t start_pause = 2000, end_pause = 1500;
        constexpr uint32_t pixels_per_second = 20;
        const uint32_t travel = (static_cast<uint64_t>(distance) * 1000 +
                                 pixels_per_second - 1) / pixels_per_second;
        const uint32_t phase = static_cast<uint32_t>(now - started_at) %
                              (start_pause + travel + end_pause);
        if (phase < start_pause) return 0;
        return -std::min(distance, static_cast<int>(
            static_cast<uint64_t>(phase - start_pause) * pixels_per_second / 1000));
    }
};

} // namespace appstore_ui
