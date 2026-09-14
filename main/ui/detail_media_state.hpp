/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint>
#include <string>

namespace appstore_ui {

class DetailMediaState
{
public:
    void normalize_images(const std::string &app_id, int image_count);
    void cycle_image(int delta, int image_count);
    void normalize_page(const std::string &app_id, int content_height, int viewport_height);
    void scroll_page(int delta);
    int page_scroll() const { return page_scroll_; }
    bool needs_loading(const std::string &app_id) const
    { return !loading_ && loading_app_id_ != app_id; }
    void show_overlay(uint32_t now);
    bool hide_overlay_if_elapsed(uint32_t now, uint32_t timeout_ms);
    void begin_loading(const std::string &app_id);
    void finish_loading(const std::string &app_id, bool failed);

    int image_index() const { return image_index_; }
    bool overlay_visible() const { return overlay_visible_; }
    uint32_t overlay_activity_tick() const { return overlay_activity_tick_; }
    bool loading() const { return loading_; }
    bool load_failed() const { return load_failed_; }

private:
    std::string image_app_id_;
    int image_index_ = 0;
    uint32_t overlay_activity_tick_ = 0;
    bool overlay_visible_ = true;
    bool loading_ = false;
    bool load_failed_ = false;
    std::string loading_app_id_;
    std::string page_app_id_;
    int page_scroll_ = 0;
    int page_max_scroll_ = 0;
};

} // namespace appstore_ui
