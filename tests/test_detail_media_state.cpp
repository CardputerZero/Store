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
    state.scroll_page(1);
    assert(state.page_scroll() == 24);
    state.scroll_page(20);
    assert(state.page_scroll() == 140);
    state.scroll_page(-20);
    assert(state.page_scroll() == 0);
    state.scroll_page(2);
    state.normalize_page("b", 269, 129);
    assert(state.page_scroll() == 0);
    state.normalize_page("b", 90, 129);
    state.scroll_page(10);
    assert(state.page_scroll() == 0);
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
