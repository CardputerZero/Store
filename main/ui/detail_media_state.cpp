/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "detail_media_state.hpp"

#include <algorithm>

namespace appstore_ui {

void DetailMediaState::normalize_images(const std::string &app_id, int image_count)
{
    if (image_app_id_ != app_id) {
        image_app_id_ = app_id;
        image_index_ = 0;
    }
    if (image_count <= 0) image_index_ = 0;
    else {
        if (image_index_ < 0) image_index_ = image_count - 1;
        if (image_index_ >= image_count) image_index_ = 0;
    }
}

void DetailMediaState::cycle_image(int delta, int image_count)
{
    if (image_count <= 0) return;
    image_index_ = (image_index_ + delta) % image_count;
    if (image_index_ < 0) image_index_ += image_count;
}

void DetailMediaState::show_overlay(uint32_t now)
{
    overlay_visible_ = true;
    overlay_activity_tick_ = now;
}

void DetailMediaState::normalize_page(const std::string &app_id, int content_height,
                                     int viewport_height)
{
    if (page_app_id_ != app_id) {
        page_app_id_ = app_id;
        page_scroll_ = 0;
    }
    page_max_scroll_ = std::max(0, content_height - viewport_height);
    page_scroll_ = std::clamp(page_scroll_, 0, page_max_scroll_);
}

void DetailMediaState::scroll_page(int delta)
{
    page_scroll_ = std::clamp(page_scroll_ + delta * 24, 0, page_max_scroll_);
}

bool DetailMediaState::hide_overlay_if_elapsed(uint32_t now, uint32_t timeout_ms)
{
    if (loading_ || !overlay_visible_ || static_cast<uint32_t>(now - overlay_activity_tick_) < timeout_ms)
        return false;
    overlay_visible_ = false;
    return true;
}

void DetailMediaState::begin_loading(const std::string &app_id)
{
    loading_app_id_ = app_id;
    image_app_id_ = app_id;
    image_index_ = 0;
    loading_ = true;
    load_failed_ = false;
}

void DetailMediaState::finish_loading(const std::string &app_id, bool failed)
{
    if (loading_app_id_ != app_id) return;
    loading_ = false;
    load_failed_ = failed;
}

} // namespace appstore_ui
