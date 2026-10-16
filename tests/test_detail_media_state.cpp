/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "detail_media_state.hpp"

#include <cassert>
#include <iostream>

int main()
{
    appstore_ui::DetailMediaState state;
    state.normalize_images("a", 3);
    state.cycle_image(-1, 3);
    assert(state.image_index() == 2);
    state.normalize_images("b", 2);
    assert(state.image_index() == 0);
    state.normalize_page("a", 269, 129);
    state.scroll_page(1, 1000);
    assert(state.page_scroll() == 24);
    assert(state.page_scroll_at(1000) == 0 && state.page_glide_remaining(1000) == 180);
    assert(state.page_scroll_at(1090) == 21); // Ease-out: most of the way at half time.
    assert(state.page_scroll_at(1180) == 24 && state.page_glide_remaining(1180) == 0);
    state.scroll_page(1, 1300);
    state.scroll_page(1, 1390); // Key repeat continues from the drawn position.
    assert(state.page_scroll() == 72 && state.page_scroll_at(1390) == 45);
    state.scroll_page(20, 2000);
    assert(state.page_scroll() == 140);
    state.scroll_page(-20, 3000);
    assert(state.page_scroll() == 0);
    state.scroll_page(2, 4000);
    state.normalize_page("b", 269, 129);
    assert(state.page_scroll() == 0 && state.page_scroll_at(4010) == 0);
    state.normalize_page("b", 90, 129);
    state.scroll_page(10, 5000);
    assert(state.page_scroll() == 0 && state.page_glide_remaining(5000) == 0);
    state.normalize_page("c", 400, 129);
    state.scroll_page(1, 0xffffffa0U);
    assert(state.page_glide_remaining(0x14U) == 64 && state.page_scroll_at(0x14U) == 23);
    assert(state.needs_loading("a"));
    state.begin_loading("a");
    state.normalize_images("b", 0);
    state.finish_loading("a", false);
    assert(!state.loading() && !state.needs_loading("a"));
    state.show_overlay(0xfffffff0U);
    assert(!state.hide_overlay_if_elapsed(0x00000010U, 40));
    assert(state.hide_overlay_if_elapsed(0x00000020U, 40));
    assert(!state.overlay_visible());
    std::cout << "detail media state tests passed\n";
}
