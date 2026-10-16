/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "appstore_pages.hpp"

#include "appstore_fonts.hpp"

#include <algorithm>

namespace appstore_ui {
namespace {

lv_obj_t *label(lv_obj_t *root, const std::string &text, int x, int y, int w, int h,
                const lv_font_t *font, uint32_t color,
                lv_label_long_mode_t mode = LV_LABEL_LONG_CLIP)
{
    lv_obj_t *obj = lv_label_create(root);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_text_font(obj, font_for_text(text, font), 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_text_letter_space(obj, 0, 0);
    lv_label_set_long_mode(obj, mode);
    lv_label_set_text(obj, text.c_str());
    if (mode == LV_LABEL_LONG_SCROLL_CIRCULAR)
        lv_obj_set_style_anim_duration(obj, 4000, LV_PART_MAIN | LV_STATE_DEFAULT);
    return obj;
}

lv_obj_t *center_label(lv_obj_t *root, const std::string &text, int x, int y, int w, int h,
                       const lv_font_t *font, uint32_t color,
                       lv_label_long_mode_t mode = LV_LABEL_LONG_CLIP)
{
    lv_obj_t *obj = label(root, text, x, y, w, h, font, color, mode);
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    return obj;
}

void strong_label(lv_obj_t *root, const std::string &text, int x, int y, int w, int h,
                  const lv_font_t *font, uint32_t color,
                  lv_label_long_mode_t mode = LV_LABEL_LONG_DOT)
{
    label(root, text, x, y, w, h, font, color, mode);
    label(root, text, x + 1, y, w, h, font, color, mode);
}

void right_strong_label(lv_obj_t *root, const std::string &text, int x, int y,
                        int w, int h, const lv_font_t *font, uint32_t color)
{
    lv_obj_t *first = label(root, text, x, y, w, h, font, color);
    lv_obj_set_style_text_align(first, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_t *second = label(root, text, x + 1, y, w - 1, h, font, color);
    lv_obj_set_style_text_align(second, LV_TEXT_ALIGN_RIGHT, 0);
}

void center_strong_label(lv_obj_t *root, const std::string &text, int x, int y, int w, int h,
                         const lv_font_t *font, uint32_t color,
                         lv_label_long_mode_t mode = LV_LABEL_LONG_CLIP)
{
    center_label(root, text, x, y, w, h, font, color, mode);
    center_label(root, text, x + 1, y, w, h, font, color, mode);
}

lv_obj_t *box(lv_obj_t *root, int x, int y, int w, int h, uint32_t color,
              uint32_t border = 0x2A3A46, int border_width = 1, int radius = 0,
              lv_opa_t bg_opa = LV_OPA_COVER)
{
    lv_obj_t *obj = lv_obj_create(root);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, bg_opa, 0);
    lv_obj_set_style_border_width(obj, border_width, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(border), 0);
    return obj;
}

// A field keeps its color on the home and detail pages.
constexpr uint32_t kVersionColor = 0x7EE787;
constexpr uint32_t kMaintainerColor = 0xFFA657;
constexpr uint32_t kSizeColor = 0x79C0FF;
constexpr uint32_t kDateColor = 0xD2A8FF;

void field_chip(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color)
{
    // Keep the tint faint: the value is drawn in the same color on top.
    lv_obj_t *chip = box(parent, x, y, w, h, color, color, 1, 5, LV_OPA_20);
    lv_obj_set_style_border_opa(chip, LV_OPA_40, 0);
}

void field_value(lv_obj_t *parent, const std::string &value, int x, int y, int w,
                 uint32_t color, HorizontalMarquee *marquee, const std::string &key,
                 uint16_t size = 14)
{
    const lv_font_t *font = store_font(value, size);
    if (marquee)
        marquee_label(parent, value, x, y, w, font->line_height, font, color, *marquee, key);
    else
        label(parent, value, x, y, w, font->line_height, font, color, LV_LABEL_LONG_DOT);
}

// 12 px values so a full date and a size such as "1023.9 KB" fit their tags.
constexpr uint16_t kHomeChipFontSize = 12;

void home_field(lv_obj_t *root, const std::string &value, int x, int y, int w,
                uint32_t color, HorizontalMarquee *marquee = nullptr,
                const std::string &key = {})
{
    constexpr int pad = 5, height = catalog_home::kTagHeight;
    field_chip(root, x, y, w, height, color);
    const int line_height = store_font(value, kHomeChipFontSize)->line_height;
    field_value(root, value, x + pad, y + (height - line_height) / 2, w - 2 * pad,
                color, marquee, key, kHomeChipFontSize);
}

// Top of a glyph's ink, measured from the top of the font's line box.
int glyph_top(const lv_font_t *font, uint32_t letter)
{
    lv_font_glyph_dsc_t glyph = {};
    if (!lv_font_get_glyph_dsc(font, &glyph, letter, 0)) return 0;
    return font->line_height - font->base_line - glyph.ofs_y - glyph.box_h;
}

void detail_field(lv_obj_t *parent, const char *caption, const std::string &value,
                  int x, int y, int w, uint32_t color, HorizontalMarquee *marquee = nullptr,
                  const std::string &key = {})
{
    constexpr int height = 31, pad = 7;
    field_chip(parent, x, y, w, height, color);
    lv_obj_t *title = label(parent, caption, x + pad, y + 2, w - 2 * pad, 12,
                            &lv_font_montserrat_10, color);
    lv_obj_set_style_text_opa(title, LV_OPA_70, 0);
    field_value(parent, value, x + pad, y + 13, w - 2 * pad, color, marquee, key);
}

int detail_info_row(lv_obj_t *parent, const char *caption, const std::string &value, int y)
{
    label(parent, caption, 10, y + 2, 64, 12, &lv_font_montserrat_10, 0x8B949E);
    lv_obj_t *text = label(parent, value, 76, y, 234, LV_SIZE_CONTENT,
                           store_font(value, 12), 0xC9D1D9, LV_LABEL_LONG_WRAP);
    lv_obj_update_layout(text);
    return y + lv_obj_get_height(text) + 4;
}

// Content that runs under a fixed edge fades out instead of being cut mid-line.
void edge_fade(lv_obj_t *root, int y, int height, uint32_t color, bool towards_bottom)
{
    lv_obj_t *fade = box(root, 0, y, 320, height, color, color, 0);
    lv_obj_set_style_bg_grad_color(fade, lv_color_hex(color), 0);
    lv_obj_set_style_bg_grad_dir(fade, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_main_opa(fade, towards_bottom ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_opa(fade, towards_bottom ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
}

void permission_badge(lv_obj_t *parent, int x, int y, uint32_t color)
{
    constexpr int size = catalog_home::kPermissionSize;
    lv_obj_t *badge = box(parent, x, y, size, size, color, color, 1, 4, LV_OPA_20);
    lv_obj_set_style_border_opa(badge, LV_OPA_40, 0);
}

// Filled mark, so the glyph stays readable inside a 14 px badge.
void permission_fill(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int radius = 0)
{
    box(parent, x, y, w, h, color, color, 0, radius);
}

// One badge per declared permission, in the registry's order.
void permission_icons(lv_obj_t *parent, int x, int y, uint32_t permissions)
{
    constexpr int step = 18;
    constexpr uint32_t color = 0x8B949E;
    using appstore::kPermissionBackground;
    using appstore::kPermissionCamera;
    using appstore::kPermissionDisplay;
    using appstore::kPermissionHardware;
    using appstore::kPermissionImu;
    using appstore::kPermissionMicrophone;
    using appstore::kPermissionNetwork;
    struct Item { uint32_t bit; void (*draw)(lv_obj_t *, int, int); };
    const Item items[] = {
        {kPermissionNetwork, [](lv_obj_t *parent, int x, int y) {
            permission_badge(parent, x, y, color);
            permission_fill(parent, x + 6, y + 2, 2, 3, color);
            permission_fill(parent, x + 4, y + 4, 6, 2, color);
            permission_fill(parent, x + 6, y + 6, 2, 6, color);
        }},
        {kPermissionMicrophone, [](lv_obj_t *parent, int x, int y) {
            permission_badge(parent, x, y, color);
            permission_fill(parent, x + 5, y + 2, 4, 6, color, 2);
            permission_fill(parent, x + 3, y + 6, 2, 3, color);
            permission_fill(parent, x + 9, y + 6, 2, 3, color);
            permission_fill(parent, x + 3, y + 8, 8, 2, color);
            permission_fill(parent, x + 6, y + 10, 2, 2, color);
        }},
        {kPermissionCamera, [](lv_obj_t *parent, int x, int y) {
            permission_badge(parent, x, y, color);
            permission_fill(parent, x + 2, y + 5, 10, 6, color, 1);
            permission_fill(parent, x + 4, y + 3, 3, 2, color);
            box(parent, x + 5, y + 6, 4, 4, 0x081010, 0x081010, 0, 2);
        }},
        {kPermissionImu, [](lv_obj_t *parent, int x, int y) {
            permission_badge(parent, x, y, color);
            permission_fill(parent, x + 3, y + 3, 8, 8, color, 4);
            box(parent, x + 5, y + 5, 4, 4, 0x081010, 0x081010, 0, 2);
        }},
        {kPermissionHardware, [](lv_obj_t *parent, int x, int y) {
            permission_badge(parent, x, y, color);
            permission_fill(parent, x + 3, y + 5, 8, 6, color, 1);
            permission_fill(parent, x + 5, y + 2, 1, 3, color);
            permission_fill(parent, x + 8, y + 2, 1, 3, color);
        }},
        {kPermissionBackground, [](lv_obj_t *parent, int x, int y) {
            permission_badge(parent, x, y, color);
            permission_fill(parent, x + 3, y + 3, 8, 2, color);
            permission_fill(parent, x + 6, y + 5, 2, 6, color);
        }},
        {kPermissionDisplay, [](lv_obj_t *parent, int x, int y) {
            permission_badge(parent, x, y, color);
            permission_fill(parent, x + 2, y + 3, 10, 6, color, 1);
            permission_fill(parent, x + 6, y + 9, 2, 2, color);
            permission_fill(parent, x + 4, y + 11, 6, 1, color);
        }},
    };
    int drawn = 0;
    for (const auto &item : items) {
        if ((permissions & item.bit) == 0) continue;
        item.draw(parent, x + drawn * step, y);
        ++drawn;
    }
}

void glide_page(lv_obj_t *document, DetailMediaState &media, uint32_t now)
{
    const uint32_t remaining = media.page_glide_remaining(now);
    if (remaining == 0) return;
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, document);
    lv_anim_set_user_data(&animation, &media);
    lv_anim_set_values(&animation, 0, static_cast<int32_t>(remaining));
    lv_anim_set_duration(&animation, remaining);
    lv_anim_set_custom_exec_cb(&animation, [](lv_anim_t *anim, int32_t) {
        auto *media = static_cast<DetailMediaState *>(lv_anim_get_user_data(anim));
        lv_obj_set_y(static_cast<lv_obj_t *>(anim->var), -media->page_scroll_at(lv_tick_get()));
    });
    // The animation timer can finish a frame early; land exactly on the target.
    lv_anim_set_completed_cb(&animation, [](lv_anim_t *anim) {
        auto *media = static_cast<DetailMediaState *>(lv_anim_get_user_data(anim));
        lv_obj_set_y(static_cast<lv_obj_t *>(anim->var), -media->page_scroll());
    });
    lv_anim_start(&animation);
}

void set_progress_scan_x(void *object, int32_t x)
{
    lv_obj_set_x(static_cast<lv_obj_t *>(object), x);
}

void modal_backdrop(lv_obj_t *root)
{
    lv_obj_t *obj = lv_obj_create(root);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 170);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x05070A), 0);
    lv_obj_set_style_bg_opa(obj, static_cast<lv_opa_t>(176), 0);
    lv_obj_set_style_blur_backdrop(obj, true, 0);
    lv_obj_set_style_blur_radius(obj, 6, 0);
    lv_obj_set_style_blur_quality(obj, LV_BLUR_QUALITY_SPEED, 0);
}

void scrolling_status_label(lv_obj_t *root, const std::string &text, int x, int y, int w, int h,
                            uint32_t color = 0xCCCC33)
{
    lv_obj_t *obj = label(root, text, x, y, w, h, &lv_font_montserrat_10, color,
                          LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_duration(obj, 4000, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void radio_option(lv_obj_t *root, int x, int y, const std::string &text,
                  bool selected, bool focused)
{
    const uint32_t border = focused ? 0xCCCC33 : (selected ? 0x58A6FF : 0x6E7681);
    const uint32_t text_color = selected ? 0xFFFFFF : 0x8B949E;
    box(root, x, y + 2, 10, 10, 0x0D1117, border, 1, 5);
    if (selected)
        box(root, x + 3, y + 5, 4, 4, focused ? 0xCCCC33 : 0x58A6FF,
            focused ? 0xCCCC33 : 0x58A6FF, 0, 2);
    label(root, text, x + 15, y, text == "Default" ? 72 : 44, 18, store_font(text, 12), text_color,
          LV_LABEL_LONG_DOT);
}

} // namespace

int glyph_middle(const lv_font_t *font, uint32_t letter)
{
    if (!font) return 0;
    lv_font_glyph_dsc_t glyph = {};
    if (!lv_font_get_glyph_dsc(font, &glyph, letter, 0)) return font->line_height / 2;
    return font->line_height - font->base_line - glyph.ofs_y - glyph.box_h / 2;
}

void draw_arrow_key(lv_obj_t *parent, ArrowKey arrow, int center_x, int center_y,
                    int width, int height)
{
    const int x = center_x - width / 2;
    // Raised look: a lighter top fading to a darker bottom, with a hairline edge.
    lv_obj_t *button = box(parent, x, center_y - height / 2, width, height, 0x34424D,
                           0x4E5D69, 1, 5);
    lv_obj_set_style_bg_grad_color(button, lv_color_hex(0x182127), 0);
    lv_obj_set_style_bg_grad_dir(button, LV_GRAD_DIR_VER, 0);

    // Solid triangles, in ArrowKey order: left, up, right, down.
    static const char *const glyphs[] = {"\xE2\x97\x80", "\xE2\x96\xB2", "\xE2\x96\xB6",
                                         "\xE2\x96\xBC"};
    static constexpr uint32_t codepoints[] = {0x25C0, 0x25B2, 0x25B6, 0x25BC};
    const int index = static_cast<int>(arrow);
    const lv_font_t *font = store_font(glyphs[index], 10);
    lv_obj_t *glyph = label(parent, glyphs[index], x,
                            center_y - glyph_middle(font, codepoints[index]), width,
                            font->line_height, font, 0xFF7A45);
    lv_obj_set_style_text_align(glyph, LV_TEXT_ALIGN_CENTER, 0);
}

lv_obj_t *marquee_label(lv_obj_t *parent, const std::string &text, int x, int y,
                        int width, int height, const lv_font_t *font, uint32_t color,
                        HorizontalMarquee &state, const std::string &key,
                        lv_text_align_t align, bool strong)
{
    state.select(key, lv_tick_get());
    lv_obj_t *viewport = lv_obj_create(parent);
    lv_obj_remove_style_all(viewport);
    lv_obj_set_pos(viewport, x, y);
    lv_obj_set_size(viewport, width, height);
    lv_obj_clear_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);
    // A content-sized, single-line label never starts LVGL's vertical marquee.
    lv_obj_t *content = nullptr;
    if (strong) {
        content = lv_obj_create(viewport);
        lv_obj_remove_style_all(content);
        lv_obj_set_size(content, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
        label(content, text, 0, 0, LV_SIZE_CONTENT, font->line_height, font, color);
        label(content, text, 1, 0, LV_SIZE_CONTENT, font->line_height, font, color);
    } else {
        content = label(viewport, text, 0, 0, LV_SIZE_CONTENT, font->line_height, font, color);
    }
    lv_obj_update_layout(content);
    const int distance = lv_obj_get_width(content) - width;
    if (distance <= 0) {
        if (align == LV_TEXT_ALIGN_CENTER) lv_obj_set_x(content, -distance / 2);
        else if (align == LV_TEXT_ALIGN_RIGHT) lv_obj_set_x(content, -distance);
        return viewport;
    }
    lv_obj_set_x(content, state.offset(lv_tick_get(), distance));
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, content);
    lv_anim_set_user_data(&animation, &state);
    lv_anim_set_values(&animation, 0, 1000);
    lv_anim_set_duration(&animation, 1000);
    lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_custom_exec_cb(&animation, [](lv_anim_t *anim, int32_t) {
        auto *content = static_cast<lv_obj_t *>(anim->var);
        auto *state = static_cast<HorizontalMarquee *>(lv_anim_get_user_data(anim));
        const int distance = lv_obj_get_width(content) - lv_obj_get_width(lv_obj_get_parent(content));
        lv_obj_set_x(content, state->offset(lv_tick_get(), distance));
    });
    lv_anim_start(&animation);
    return viewport;
}

void render_exit_hint(lv_obj_t *root)
{
    // Match Calculator's centered exit-hint card and dimmed backdrop.
    lv_obj_t *overlay = box(root, 0, 0, 320, 170, 0x000000, 0x000000, 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_80, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *card = box(overlay, 34, 58, 252, 54, 0x18181C, 0x3C3C40, 1, 10);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *text = label(card, "Hold ESC for 3s to exit.", 0, 0, 224,
                           LV_SIZE_CONTENT, store_font("", 14), 0xFFFFFF,
                           LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(text, store_font("", 14), 0);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(text);
}

AppStoreUiPage::AppStoreUiPage()
{
    set_page_title("Store");
    enable_top_bar();
}

void AppStoreUiPage::activate()
{
    cp0_lvgl_start_app_page(*this);
}

void InitializationProgressPage::render(const PageRenderContext &context,
                                        const InitializationProgressViewModel &model)
{
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, 0x0D1117, 0x0D1117, 0);
    box(root, 0, 20, 320, 22, 0x101B2D, 0x101B2D, 0);
    const bool preparing_catalog = model.phase == "CATALOG";
    label(root, preparing_catalog ? "Preparing App Catalog" : "Checking Network",
          8, 24, 190, 15, &lv_font_montserrat_12, 0xFFFFFF);
    label(root, "Hold ESC", 254, 24, 60, 14, &lv_font_montserrat_10, 0xAECBFA);
    center_strong_label(root, appstore::upper_ascii(model.phase), 28, 47, 264, 16,
                        &lv_font_montserrat_12, 0x58A6FF, LV_LABEL_LONG_DOT);
    label(root, appstore::one_line(model.url, 62), 18, 68, 284, 20,
          &lv_font_montserrat_10, 0xC9D1D9, LV_LABEL_LONG_DOT);
    box(root, 20, 94, 280, 15, 0x111923, 0x30363D, 1, 3);
    for (int i = 0; i < 7; ++i) box(root, 24 + i * 39, 98, 28, 7, 0x162235, 0x162235, 0, 2);
    if (model.percent >= 0) {
        const int fill = std::max(5, std::min(272, model.percent * 272 / 100));
        box(root, 24, 98, fill, 7, 0x2FEC8D, 0x2FEC8D, 0, 2);
        box(root, std::max(24, std::min(286, 24 + fill - 4)), 96, 8, 11,
            0xD8FF6A, 0xD8FF6A, 0, 3);
    } else {
        lv_obj_t *scan = box(root, 24, 97, 52, 9, 0x2FEC8D, 0xD8FF6A, 1, 3);
        lv_anim_t animation;
        lv_anim_init(&animation);
        lv_anim_set_var(&animation, scan);
        lv_anim_set_values(&animation, 24, 244);
        lv_anim_set_duration(&animation, 900);
        lv_anim_set_playback_duration(&animation, 900);
        lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&animation, lv_anim_path_ease_in_out);
        lv_anim_set_exec_cb(&animation, set_progress_scan_x);
        lv_anim_start(&animation);
    }
    center_label(root, appstore::one_line(model.detail, 52), 18, 119, 284, 14,
                 &lv_font_montserrat_10, 0xB8B8B8, LV_LABEL_LONG_DOT);
    center_strong_label(root, model.cancel_requested ? "Cancelling..." :
                        (preparing_catalog ? "Building app list...   Tab: Settings" :
                         "Tab: Settings   Hold ESC: Exit"), 18, 151, 284, 12,
                        &lv_font_montserrat_10, 0xCCCC33, LV_LABEL_LONG_DOT);
    if (!model.failed) return;

    modal_backdrop(root);
    box(root, 16, 27, 288, 136, 0x111923, 0xF85149, 2, 4);
    box(root, 16, 27, 288, 24, 0x8B2F34, 0x8B2F34, 0);
    center_strong_label(root, model.error_title, 32, 32, 256, 15,
                        &lv_font_montserrat_12, 0xFFFFFF, LV_LABEL_LONG_DOT);
    center_strong_label(root, appstore::one_line(model.error_message, 38), 30, 61, 260, 15,
                        &lv_font_montserrat_12, 0xFFD2D2, LV_LABEL_LONG_DOT);
    int y = 82;
    for (const auto &line_text : model.error_detail_lines) {
        center_label(root, appstore::one_line(line_text, 44), 30, y, 260, 12,
                     &lv_font_montserrat_10, 0xB8B8B8, LV_LABEL_LONG_DOT);
        y += 13;
        if (y > 95) break;
    }
    const bool exit_focused = model.failure_focus == 0;
    const bool settings_focused = model.failure_focus == 1;
    box(root, 45, 119, 92, 19, exit_focused ? 0x8B2F34 : 0x4B1F24,
        exit_focused ? 0xCCCC33 : 0xF85149, exit_focused ? 2 : 1, 2);
    center_strong_label(root, "EXIT", 49, 123, 84, 12,
                        &lv_font_montserrat_10, 0xFFFFFF);
    box(root, 166, 119, 109, 19, settings_focused ? 0x2EA043 : 0x1A6A2A,
        settings_focused ? 0xCCCC33 : 0x2EA043, settings_focused ? 2 : 1, 2);
    center_strong_label(root, "SETTINGS", 170, 123, 101, 12,
                        &lv_font_montserrat_10, 0xFFFFFF);
    center_label(root, "Tab: Settings   Enter confirm", 48, 146, 224, 12,
                 &lv_font_montserrat_10, 0xCCCC33);
}

void CatalogDisplayPage::render(const PageRenderContext &context,
                                const CatalogDisplayViewModel &model,
                                const std::function<void()> &draw_category,
                                const std::function<void()> &draw_icon,
                                const std::function<void()> &draw_shortcuts)
{
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, 0x081010, 0x081010, 0);
    draw_category();
    draw_icon();
    const int details_width = catalog_home::kDetailsRight - catalog_home::kDetailsX;
    if (model.show_empty) {
        const lv_font_t *font = store_font(model.empty_message, 14);
        label(root, model.empty_message, catalog_home::kDetailsX,
              catalog_home::kIconCenterY - font->line_height, details_width,
              font->line_height * 2, font, 0xFFFFFF, LV_LABEL_LONG_WRAP);
    } else {
        // Fixed rows, whether or not an app declares permissions, so the block
        // stays centered on the icon.
        const lv_font_t *name_font = store_font(model.name, 20, true);
        constexpr int chip_width = 84, column_gap = 4;
        marquee_label(root, model.name, catalog_home::kDetailsX,
                      catalog_home::kTopY - glyph_top(name_font, 'H'), details_width,
                      name_font->line_height, name_font, 0xFFFFFF, title_marquee_, model.name);
        const int value_x = catalog_home::kDetailsX + chip_width + column_gap;
        const int value_width = details_width - chip_width - column_gap;
        home_field(root, model.version, catalog_home::kDetailsX, catalog_home::kTagRow1Y,
                   chip_width, kVersionColor);
        home_field(root, model.author, value_x, catalog_home::kTagRow1Y, value_width,
                   kMaintainerColor, &author_marquee_, model.name + model.author);
        home_field(root, model.size, catalog_home::kDetailsX, catalog_home::kTagRow2Y,
                   chip_width, kSizeColor);
        home_field(root, model.updated, value_x, catalog_home::kTagRow2Y, value_width,
                   kDateColor);
        permission_icons(root, catalog_home::kDetailsX, catalog_home::kPermissionsY,
                         model.permissions);
    }
    if (model.show_status) {
        // A status message temporarily takes over the permission row.
        box(root, catalog_home::kDetailsX - 2, catalog_home::kPermissionsY - 1,
            details_width + 4, 16, 0x081010, 0x081010, 0);
        scrolling_status_label(root, model.status, catalog_home::kDetailsX,
                               catalog_home::kPermissionsY + 1, details_width, 12);
    }
    draw_shortcuts();
}

void CatalogDisplayPage::render_text_entry(const PageRenderContext &context,
                                           const TextEntryViewModel &model)
{
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, 0x0D1117, 0x0D1117, 0);
    strong_label(root, appstore::upper_ascii(model.title), 10, 27, 210, 17,
                 &lv_font_montserrat_14, model.accent, LV_LABEL_LONG_DOT);
    label(root, "TEXT INPUT", 238, 29, 72, 12,
          &lv_font_montserrat_10, 0x6E7681, LV_LABEL_LONG_DOT);
    center_strong_label(root, model.prompt, 58, 49, 204, 16,
                        &lv_font_montserrat_12, model.accent, LV_LABEL_LONG_DOT);
    box(root, model.input_x, 67, model.input_width, 43, 0x111923, model.accent, 2, 3);
    const std::string display = model.input.empty() ? model.placeholder :
        appstore::upper_ascii(model.input);
    center_strong_label(root, appstore::one_line(display, model.display_limit),
                        model.input_x + 10, 78, model.input_width - 20, 23,
                        &lv_font_montserrat_20,
                        model.input.empty() ? 0x6E7681 : 0xE6EDF3, LV_LABEL_LONG_DOT);
    center_label(root, appstore::one_line(model.message, 48), 10, 118, 300, 14,
                 &lv_font_montserrat_10, 0xB8B8B8, LV_LABEL_LONG_DOT);
    center_label(root, "Esc Back   BS Delete   Enter Confirm", 10, 153, 300, 12,
                 &lv_font_montserrat_10, 0xCCCC33, LV_LABEL_LONG_DOT);
}

void CatalogDisplayPage::render_search(const PageRenderContext &context,
                                       const SearchPageViewModel &model)
{
    if (!model.results_active) {
        render_text_entry(context, {"Search", "SEARCH APPS", "app name", model.input,
                                    model.message, 0x7ED957, 50, 220, 16});
        return;
    }
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, 0x0D1117, 0x0D1117, 0);
    strong_label(root, "SEARCH RESULTS", 10, 25, 190, 17,
                 &lv_font_montserrat_14, 0x7ED957, LV_LABEL_LONG_DOT);
    right_strong_label(root, std::to_string(model.result_count) + " APPS",
                       230, 27, 80, 13, &lv_font_montserrat_10, 0xB8B8B8);
    box(root, 10, 44, 300, 18, 0x111923, 0x2EA043, 1, 3);
    label(root, "QUERY", 17, 48, 42, 11, &lv_font_montserrat_10, 0x7ED957);
    label(root, appstore::upper_ascii(model.input), 62, 47, 240, 12,
          &lv_font_montserrat_10, 0xE6EDF3, LV_LABEL_LONG_SCROLL_CIRCULAR);
    for (size_t row = 0; row < model.rows.size(); ++row) {
        const auto &item = model.rows[row];
        const int y = 66 + static_cast<int>(row) * 27;
        box(root, 10, y, 300, 24, item.selected ? 0x142817 : 0x10171D,
            item.selected ? 0x7ED957 : 0x27313A, 1, 3);
        const std::string name = appstore::upper_ascii(item.name);
        const lv_font_t *name_font = context.font_for_text(
            name, item.selected ? &lv_font_montserrat_12 : &lv_font_montserrat_10);
        if (item.selected)
            marquee_label(root, name, 17, y + 2, 218, 14, name_font, 0xFFFFFF,
                          search_marquee_, name, LV_TEXT_ALIGN_LEFT, true);
        else
            strong_label(root, name, 17, y + 2, 218, 14, name_font, 0xB8B8B8, LV_LABEL_LONG_DOT);
        label(root, appstore::one_line(item.meta.empty() ? "-" : item.meta, 28),
              17, y + 14, 218, 9, &lv_font_montserrat_10,
              item.selected ? 0x7ED957 : 0x6E7681,
              LV_LABEL_LONG_DOT);
        lv_obj_t *version = label(
            root, "V" + appstore::one_line(item.version.empty() ? "0" : item.version, 7),
            239, y + 6, 63, 12, &lv_font_montserrat_10,
            item.selected ? 0xFFFFFF : 0x8B949E, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(version, LV_TEXT_ALIGN_RIGHT, 0);
    }
    center_label(root, "Fn+F/X Move   BS Edit   Enter Open   Esc Back",
                 10, 153, 300, 12, &lv_font_montserrat_10, 0xCCCC33,
                 LV_LABEL_LONG_DOT);
}

void AppDetailPage::render(const PageRenderContext &context, const AppDetailViewModel &model,
                           DetailMediaState &media, const std::vector<std::string> &screenshots,
                           const std::function<bool(lv_obj_t *, const std::string &, int, int)> &draw_thumbnail,
                           const std::function<void(const appstore::StoreApp &)> &draw_shortcuts)
{
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    constexpr uint32_t background = 0x081010;
    constexpr int viewport_y = 20;
    constexpr int viewport_height = 129;
    box(root, 0, 20, 320, 150, background, background, 0);
    if (!model.has_app) {
        label(root, "No selected app", 10, 50, 304, 20, store_font("", 12), 0xFFFFFF);
        return;
    }
    // Clip the entire detail document between the system bar and fixed action row.
    lv_obj_t *viewport = lv_obj_create(root);
    lv_obj_remove_style_all(viewport);
    lv_obj_set_pos(viewport, 0, viewport_y);
    lv_obj_set_size(viewport, 320, viewport_height);
    lv_obj_clear_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *document = lv_obj_create(viewport);
    lv_obj_remove_style_all(document);
    lv_obj_set_width(document, 320);
    lv_obj_clear_flag(document, LV_OBJ_FLAG_SCROLLABLE);

    const appstore::StoreApp &app = model.app;
    const lv_font_t *title_font = store_font(app.name, 20, true);
    marquee_label(document, app.name, 10, 4, 300, title_font->line_height, title_font,
                  0xFFFFFF, title_marquee_, app.id + app.name);
    int y = 8 + title_font->line_height;
    // Same fields, arrangement and colors as the home page.
    detail_field(document, "VERSION", model.version, 10, y, 146, kVersionColor);
    detail_field(document, "MAINTAINER", model.author, 164, y, 146, kMaintainerColor,
                 &author_marquee_, app.id + model.author);
    y += 34;
    detail_field(document, "SIZE", app.size, 10, y, 146, kSizeColor);
    detail_field(document, "UPDATED", model.updated, 164, y, 146, kDateColor);
    y += 36;

    const uint32_t state_color = !app.installed ? 0x6E7681 :
        (appstore::can_upgrade_app(app) ? 0xF0B429 : 0x52D05D);
    box(document, 11, y + 5, 8, 8, state_color, state_color, 0, 4);
    label(document, model.state, 25, y, 285, 17, store_font(model.state, 14),
          app.installed ? 0xE6EDF3 : 0xB1BAC4, LV_LABEL_LONG_DOT);
    y += 23;
    box(document, 10, y, 300, 1, 0x1E2A30, 0x1E2A30, 0);
    y += 7;
    lv_obj_t *description = label(document, model.description, 10, y, 300, LV_SIZE_CONTENT,
                                  store_font(model.description, 12), 0xC9D1D9,
                                  LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_line_space(description, 2, 0);
    lv_obj_update_layout(description);
    y += lv_obj_get_height(description) + 10;

    media.normalize_images(app.id, static_cast<int>(screenshots.size()));
    if (!screenshots.empty()) {
        const int index = media.image_index();
        label(document, "SCREENSHOTS", 10, y, 150, 12, &lv_font_montserrat_10, 0x8B949E);
        if (screenshots.size() > 1) {
            lv_obj_t *count = label(document, std::to_string(index + 1) + "/" +
                                    std::to_string(screenshots.size()), 250, y, 60, 12,
                                    &lv_font_montserrat_10, 0x8B949E);
            lv_obj_set_style_text_align(count, LV_TEXT_ALIGN_RIGHT, 0);
        }
        const int shot_y = y + 16;
        // One complete 160 x 85 preview and a glimpse of the next image.
        draw_thumbnail(document, screenshots[index], 20, shot_y);
        if (screenshots.size() > 1) {
            draw_thumbnail(document, screenshots[(index + 1) % screenshots.size()], 185, shot_y);
            draw_arrow_key(document, ArrowKey::Left, 10, shot_y + 42);
            draw_arrow_key(document, ArrowKey::Right, 311, shot_y + 42);
        }
        y = shot_y + 85 + 10;
    }
    y = detail_info_row(document, "PACKAGE", app.package.empty() ? "-" : app.package, y);
    y = detail_info_row(document, "CATEGORY", model.categories, y);
    y += 6;
    lv_obj_set_height(document, y);
    media.normalize_page(app.id, y, viewport_height);
    const uint32_t now = lv_tick_get();
    lv_obj_set_y(document, -media.page_scroll_at(now));
    glide_page(document, media, now);

    if (media.page_scroll() > 0)
        edge_fade(root, viewport_y, 10, background, false);
    if (media.page_scroll() < y - viewport_height)
        edge_fade(root, viewport_y + viewport_height - 12, 12, background, true);
    if (model.show_status) {
        // Fixed above the actions so feedback is visible at any scroll position.
        lv_obj_t *toast = box(root, 0, 132, 320, 17, background, 0x2A3A46, 1);
        lv_obj_set_style_border_side(toast, LV_BORDER_SIDE_TOP, 0);
        scrolling_status_label(root, model.status, 10, 135, 300, 12);
    }
    draw_shortcuts(app);
}

void AppDetailPage::render_confirmation(const PageRenderContext &context,
                                        const ConfirmationViewModel &model)
{
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, 0x0D1117, 0x0D1117, 0);
    const bool ownership_conflict =
        !model.lines.empty() && model.lines[0] == "FILE OWNERSHIP CONFLICT";
    const bool safety_warning = model.lines.size() >= 4 &&
        model.lines[1].find("cannot verify") != std::string::npos;
    const uint32_t accent = ownership_conflict || safety_warning ? 0xF85149 : 0x58A6FF;
    const uint32_t header = ownership_conflict || safety_warning ? 0x8B2F34 : 0x1F6FEB;
    box(root, 18, 30, 284, 116, 0x111923, accent, 2, 4);
    box(root, 18, 30, 284, 23, header, header, 0);
    center_strong_label(root, ownership_conflict ? "FORCE OVERWRITE?" :
                        (safety_warning ? "SAFETY WARNING" : "CONFIRM ACTION"),
                        38, 35, 244, 15,
                        &lv_font_montserrat_12, 0xFFFFFF);
    int y = 60;
    for (size_t index = 0; index < model.lines.size(); ++index) {
        const std::string &line_text = model.lines[index];
        const bool action_line = index == 0 && !ownership_conflict;
        center_strong_label(root, appstore::one_line(line_text, 44),
                            28, y, 264, action_line ? 15 : 12,
                            action_line ? &lv_font_montserrat_12 : &lv_font_montserrat_10,
                            index >= 1 && safety_warning ? 0xFFD2D2 : 0xE6EDF3,
                            LV_LABEL_LONG_DOT);
        y += action_line ? 18 : 14;
        if (y > 111) break;
    }
    const bool yes = model.focus == 0;
    const bool no = model.focus == 1;
    box(root, 62, 122, 84, 17, yes ? 0x2EA043 : 0x1A6A2A,
        yes ? 0xCCCC33 : 0x2EA043, yes ? 2 : 1);
    center_strong_label(root, "YES", 66, 125, 76, 12,
                        &lv_font_montserrat_10, 0xFFFFFF);
    box(root, 174, 122, 84, 17, no ? 0x8B2F34 : 0x4B1F24,
        no ? 0xCCCC33 : 0xF85149, no ? 2 : 1);
    center_strong_label(root, "NO", 178, 125, 76, 12,
                        &lv_font_montserrat_10, 0xFFFFFF);
}

void AppDetailPage::render_progress(const PageRenderContext &context,
                                    const PackageProgressViewModel &model)
{
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, 0x0D1117, 0x0D1117, 0);
    box(root, 0, 20, 320, 22, 0x1F6FEB, 0x1F6FEB, 0);
    label(root, "Package Operation", 8, 24, 190, 15, &lv_font_montserrat_12, 0xFFFFFF);
    label(root, "Esc Exit", 262, 24, 52, 14, &lv_font_montserrat_10, 0xAECBFA);
    center_strong_label(root, appstore::upper_ascii(model.action), 24, 50, 272, 18,
                        &lv_font_montserrat_14, 0xCCCC33, LV_LABEL_LONG_DOT);
    const std::string title = appstore::one_line(model.title, 26);
    center_strong_label(root, title, 24, 70, 272, 16,
                        context.font_for_text(title, &lv_font_montserrat_12),
                        0xE6EDF3, LV_LABEL_LONG_DOT);
    box(root, 28, 92, 264, 12, 0x30363D, 0x30363D, 0, 2);
    if (model.progress >= 0) {
        const int fill = std::max(4, std::min(264, model.progress * 264 / 100));
        box(root, 28, 92, fill, 12, 0xCCCC33, 0xCCCC33, 0, 2);
    } else {
        const int offset = static_cast<int>((lv_tick_get() / 90) % 216);
        box(root, 28 + offset, 92, 48, 12, 0xCCCC33, 0xCCCC33, 0, 2);
    }
    center_label(root, appstore::one_line(model.detail, 48), 18, 113, 284, 14,
                 &lv_font_montserrat_10, 0xB8B8B8, LV_LABEL_LONG_DOT);
    center_strong_label(root, "Elapsed " + std::to_string(model.elapsed_seconds) + "s",
                        88, 132, 144, 13, &lv_font_montserrat_10, 0x58A6FF,
                        LV_LABEL_LONG_DOT);
    center_label(root, "Keep Store open until this finishes.", 18, 153, 284, 12,
                 &lv_font_montserrat_10, 0xCCCC33, LV_LABEL_LONG_DOT);
}

void AppDetailPage::render_error(const PageRenderContext &context,
                                 const ErrorDialogViewModel &model)
{
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, 0x0D1117, 0x0D1117, 0);
    box(root, 18, 34, 284, 116, 0x111923, 0xF85149, 2, 4);
    box(root, 18, 34, 284, 23, 0x8B2F34, 0x8B2F34, 0);
    center_strong_label(root, model.title, 32, 39, 256, 15,
                        &lv_font_montserrat_12, 0xFFFFFF, LV_LABEL_LONG_DOT);
    center_strong_label(root, appstore::one_line(model.message, 38), 30, 68, 260, 15,
                        &lv_font_montserrat_12, 0xFFD2D2, LV_LABEL_LONG_DOT);
    int y = 89;
    for (size_t i = 0; i < model.detail_lines.size() && i < 3; ++i) {
        center_label(root, appstore::one_line(model.detail_lines[i], 44), 30, y, 260, 12,
                     &lv_font_montserrat_10, 0xB8B8B8, LV_LABEL_LONG_DOT);
        y += 13;
    }
    box(root, 118, 128, 84, 17, 0x2EA043, 0xCCCC33, 2, 2);
    center_strong_label(root, model.repairable ? "REPAIR" : "OK", 122, 131, 76, 12,
                        &lv_font_montserrat_10, 0xFFFFFF);
    center_label(root, model.repairable ? "Enter Repair  B Back" : "Enter OK",
                 model.repairable ? 82 : 102, 154, model.repairable ? 156 : 116, 12,
                 &lv_font_montserrat_10, 0xCCCC33);
}

void AppDetailPage::render_screenshot_overlay(const PageRenderContext &context,
                                              const ScreenshotOverlayViewModel &model,
                                              const std::function<void()> &draw_background)
{
    context.prepare();
    lv_obj_t *root = screen();
    box(root, 0, 0, 320, 170, 0x05070A, 0x05070A, 0);
    if (!model.has_app) {
        box(root, 0, 20, 320, 150, 0x0D1117, 0x0D1117, 0);
        label(root, "No selected app", 10, 50, 304, 14, &lv_font_montserrat_12, 0xE6EDF3);
        return;
    }
    if (model.has_screenshots) draw_background();
    if (!model.has_screenshots || model.overlay_visible) {
        box(root, 0, 0, 320, 23, 0x05070A, 0x05070A, 0, 0, static_cast<lv_opa_t>(210));
        box(root, 0, 145, 320, 25, 0x05070A, 0x05070A, 0, 0, static_cast<lv_opa_t>(210));
        label(root, "Screenshots", 8, 5, 120, 14, &lv_font_montserrat_12, 0xFFFFFF);
        label(root, "Esc Back", 258, 5, 56, 14, &lv_font_montserrat_10, 0xAECBFA);
    }
    if (model.loading) {
        lv_obj_t *spinner = lv_spinner_create(root);
        lv_obj_set_size(spinner, 34, 34);
        lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -8);
        lv_obj_set_style_arc_color(spinner, lv_color_hex(0x30363D), LV_PART_MAIN);
        lv_obj_set_style_arc_color(spinner, lv_color_hex(0x58A6FF), LV_PART_INDICATOR);
        center_label(root, "Loading screenshots...", 52, 108, 216, 14,
                     &lv_font_montserrat_12, 0xAECBFA, LV_LABEL_LONG_DOT);
        return;
    }
    if (!model.has_screenshots) {
        center_strong_label(root, model.load_failed ? "LOAD FAILED" : "NO SCREENSHOTS", 52, 76, 216, 16,
                            &lv_font_montserrat_14, 0xCCCC33, LV_LABEL_LONG_DOT);
        return;
    }
    if (!model.overlay_visible) return;
    center_strong_label(root, std::to_string(model.image_index + 1) + "/" +
                        std::to_string(model.image_count), 136, 5, 48, 14,
                        &lv_font_montserrat_10, 0xAECBFA, LV_LABEL_LONG_DOT);
    center_label(root, "Z / < previous        C / > next", 36, 153, 248, 12,
                 &lv_font_montserrat_10, 0xCCCC33, LV_LABEL_LONG_DOT);
}

void StoreSettingsPage::render(const PageRenderContext &context,
                               const StoreSettingsViewModel &model)
{
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, 0x0D1117, 0x0D1117, 0);
    box(root, 0, 20, 320, 22, 0x1F6FEB, 0x1F6FEB, 0);
    label(root, "Registry Settings", 8, 24, 220, 18, store_font("", 14), 0xFFFFFF);
    label(root, "Esc Back", 250, 25, 64, 18, store_font("", 12), 0xAECBFA);
    const bool region_focused = model.focus == 0;
    label(root, "Region", 24, 46, 48, 18, store_font("", 12),
          region_focused ? 0xCCCC33 : 0x58A6FF);
    radio_option(root, 76, 46, "Auto", model.region_code == "auto", region_focused);
    radio_option(root, 142, 46, "Default", model.region_code == "default", region_focused);
    radio_option(root, 234, 46, "CN", model.region_code == "CN", region_focused);
    if (!model.has_entry) {
        label(root, model.loading ? "Loading registries..." : "No registries configured.",
              24, 82, 272, 36, store_font("", 12), 0xB8B8B8, LV_LABEL_LONG_WRAP);
    } else {
        const auto &entry = model.entry;
        const std::string source = model.region_code == "auto"
            ? "Source (" + model.active_region + ")" : "Source";
        label(root, source, 24, 68, 210, 18, store_font(source, 12),
              model.focus == 1 ? 0xCCCC33 : 0x58A6FF, LV_LABEL_LONG_DOT);
        center_label(root, std::to_string(model.selected_index + 1) + "/" +
                     std::to_string(model.entry_count), 248, 68, 48, 18,
                     store_font("", 12), 0x8B949E, LV_LABEL_LONG_DOT);
        const uint32_t nav = model.focus == 1 ? 0xFF6A3D : 0x6E7681;
        label(root, "<", 6, 85, 16, 22, store_font("", 20), nav);
        label(root, ">", 302, 85, 16, 22, store_font("", 20), nav);
        marquee_label(root, entry.name, 24, 86, 194, 20, store_font(entry.name, 14),
                      0xFFFFFF, source_marquee_, entry.url + entry.name);
        lv_obj_t *count = label(root, entry.count + " apps", 224, 87, 72, 18,
                               store_font("", 12), 0xCCCC33, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(count, LV_TEXT_ALIGN_RIGHT, 0);
        label(root, "URL", 24, 111, 32, 18, store_font("", 12), 0x58A6FF);
        box(root, 60, 108, 236, 24, 0x111923, 0x2A3A46, 1, 2);
        const std::string url = entry.builtin ? model.registry_url : entry.url;
        marquee_label(root, url, 66, 112, 224, 18, store_font(url, 12), 0xFFFFFF,
                      url_marquee_, url);
    }
    if (!model.status.empty()) {
        // Keep the full status readable without taking space from the action row.
        lv_obj_t *status = label(root, model.status, 10, 134, 300, 17,
                                store_font(model.status, 12),
                                model.status_is_error ? 0xF85149 : 0xCCCC33,
                                LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_anim_duration(status, 8000, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void StoreSettingsPage::render_editor(const PageRenderContext &context,
                                      const RegistryEditorViewModel &model)
{
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, 0x0D1117, 0x0D1117, 0);
    box(root, 0, 20, 320, 22, 0x1F6FEB, 0x1F6FEB, 0);
    label(root, model.editing ? "Edit Registry" : "Add Registry",
          8, 24, 220, 18, store_font("", 14), 0xFFFFFF);
    label(root, "Esc Back", 250, 25, 64, 18, store_font("", 12), 0xAECBFA);

    label(root, "Name", 10, 46, 50, 18, store_font("", 12),
          model.focus == 0 ? 0xCCCC33 : 0x58A6FF);
    box(root, 10, 65, 300, 24, 0x111923,
        model.focus == 0 ? 0xCCCC33 : 0x2A3A46, 1, 2);
    label(root, model.name, 16, 68, 288, 18,
          store_font(model.name, 12), 0xE6EDF3, LV_LABEL_LONG_DOT);

    label(root, "URL", 10, 92, 50, 18, store_font("", 12),
          model.focus == 1 ? 0xCCCC33 : 0x58A6FF);
    box(root, 10, 110, 300, 35, 0x111923,
        model.focus == 1 ? 0xCCCC33 : 0x2A3A46, 1, 2);
    label(root, model.url, 16, 113, 288, 28, store_font(model.url, 12),
          0xE6EDF3, LV_LABEL_LONG_WRAP);

    center_label(root, "Tab Next   BS Delete   Enter Save",
                 10, 152, 300, 18, store_font("", 12), 0xCCCC33,
                 LV_LABEL_LONG_DOT);
}

} // namespace appstore_ui
