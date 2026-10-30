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
lv_obj_t *edge_fade(lv_obj_t *root, int y, int height, uint32_t color, bool towards_bottom)
{
    lv_obj_t *fade = box(root, 0, y, 320, height, color, color, 0);
    lv_obj_set_style_bg_grad_color(fade, lv_color_hex(color), 0);
    lv_obj_set_style_bg_grad_dir(fade, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_main_opa(fade, towards_bottom ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_opa(fade, towards_bottom ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    return fade;
}

constexpr int kPermissionDesignSize = 14;

// The marks are drawn on a 14 px grid and scaled to the label's line height.
int permission_px(int value, int size)
{
    return value * size / kPermissionDesignSize;
}

int permission_span(int value, int size)
{
    return std::max(1, permission_px(value, size));
}

void permission_badge(lv_obj_t *parent, int x, int y, int size, uint32_t color)
{
    lv_obj_t *badge = box(parent, x, y, size, size, color, color, 1,
                          permission_span(4, size), LV_OPA_20);
    lv_obj_set_style_border_opa(badge, LV_OPA_40, 0);
}

void permission_fill(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int radius = 0)
{
    box(parent, x, y, w, h, color, color, 0, radius);
}

// One badge per declared permission, in the registry's order.
void permission_icons(lv_obj_t *parent, int x, int y, int size, uint32_t permissions)
{
    const int step = size + 4;
    constexpr uint32_t color = 0x8B949E;
    using appstore::kPermissionBackground;
    using appstore::kPermissionCamera;
    using appstore::kPermissionDisplay;
    using appstore::kPermissionHardware;
    using appstore::kPermissionImu;
    using appstore::kPermissionMicrophone;
    using appstore::kPermissionNetwork;
    struct Item { uint32_t bit; void (*draw)(lv_obj_t *, int, int, int); };
    const Item items[] = {
        {kPermissionNetwork, [](lv_obj_t *parent, int x, int y, int size) {
            permission_badge(parent, x, y, size, color);
            permission_fill(parent, x + permission_px(6, size), y + permission_px(2, size),
                            permission_span(2, size), permission_span(3, size), color);
            permission_fill(parent, x + permission_px(4, size), y + permission_px(4, size),
                            permission_span(6, size), permission_span(2, size), color);
            permission_fill(parent, x + permission_px(6, size), y + permission_px(6, size),
                            permission_span(2, size), permission_span(6, size), color);
        }},
        {kPermissionMicrophone, [](lv_obj_t *parent, int x, int y, int size) {
            permission_badge(parent, x, y, size, color);
            permission_fill(parent, x + permission_px(5, size), y + permission_px(2, size),
                            permission_span(4, size), permission_span(6, size), color,
                            permission_span(2, size));
            permission_fill(parent, x + permission_px(3, size), y + permission_px(6, size),
                            permission_span(2, size), permission_span(3, size), color);
            permission_fill(parent, x + permission_px(9, size), y + permission_px(6, size),
                            permission_span(2, size), permission_span(3, size), color);
            permission_fill(parent, x + permission_px(3, size), y + permission_px(8, size),
                            permission_span(8, size), permission_span(2, size), color);
            permission_fill(parent, x + permission_px(6, size), y + permission_px(10, size),
                            permission_span(2, size), permission_span(2, size), color);
        }},
        {kPermissionCamera, [](lv_obj_t *parent, int x, int y, int size) {
            permission_badge(parent, x, y, size, color);
            permission_fill(parent, x + permission_px(2, size), y + permission_px(5, size),
                            permission_span(10, size), permission_span(6, size), color,
                            permission_span(1, size));
            permission_fill(parent, x + permission_px(4, size), y + permission_px(3, size),
                            permission_span(3, size), permission_span(2, size), color);
            box(parent, x + permission_px(5, size), y + permission_px(6, size),
                permission_span(4, size), permission_span(4, size), 0x081010, 0x081010, 0,
                permission_span(2, size));
        }},
        {kPermissionImu, [](lv_obj_t *parent, int x, int y, int size) {
            permission_badge(parent, x, y, size, color);
            permission_fill(parent, x + permission_px(3, size), y + permission_px(3, size),
                            permission_span(8, size), permission_span(8, size), color,
                            permission_span(4, size));
            box(parent, x + permission_px(5, size), y + permission_px(5, size),
                permission_span(4, size), permission_span(4, size), 0x081010, 0x081010, 0,
                permission_span(2, size));
        }},
        {kPermissionHardware, [](lv_obj_t *parent, int x, int y, int size) {
            permission_badge(parent, x, y, size, color);
            permission_fill(parent, x + permission_px(3, size), y + permission_px(5, size),
                            permission_span(8, size), permission_span(6, size), color,
                            permission_span(1, size));
            permission_fill(parent, x + permission_px(5, size), y + permission_px(2, size),
                            permission_span(1, size), permission_span(3, size), color);
            permission_fill(parent, x + permission_px(8, size), y + permission_px(2, size),
                            permission_span(1, size), permission_span(3, size), color);
        }},
        {kPermissionBackground, [](lv_obj_t *parent, int x, int y, int size) {
            permission_badge(parent, x, y, size, color);
            permission_fill(parent, x + permission_px(3, size), y + permission_px(3, size),
                            permission_span(8, size), permission_span(2, size), color);
            permission_fill(parent, x + permission_px(6, size), y + permission_px(5, size),
                            permission_span(2, size), permission_span(6, size), color);
        }},
        {kPermissionDisplay, [](lv_obj_t *parent, int x, int y, int size) {
            permission_badge(parent, x, y, size, color);
            permission_fill(parent, x + permission_px(2, size), y + permission_px(3, size),
                            permission_span(10, size), permission_span(6, size), color,
                            permission_span(1, size));
            permission_fill(parent, x + permission_px(6, size), y + permission_px(9, size),
                            permission_span(2, size), permission_span(2, size), color);
            permission_fill(parent, x + permission_px(4, size), y + permission_px(11, size),
                            permission_span(6, size), permission_span(1, size), color);
        }},
    };
    int drawn = 0;
    for (const auto &item : items) {
        if ((permissions & item.bit) == 0) continue;
        item.draw(parent, x + drawn * step, y, size);
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

// The startup screen follows Apple's dark system colors.
constexpr uint32_t kPageBackground = 0x081010;
constexpr uint32_t kSecondaryText = 0x8E8E93;
constexpr uint32_t kSystemBlue = 0x0A84FF;
constexpr uint32_t kSystemGray = 0x3A3A3C;

namespace startup {
constexpr int kLogoSize = 44;
constexpr int kLogoY = 30;
// The artwork has its own padding, so the visible gap is larger than this.
constexpr int kTitleCapsY = kLogoY + kLogoSize + 9;
constexpr int kBarWidth = 184;
constexpr int kBarHeight = 4;
constexpr int kBarX = (320 - kBarWidth) / 2;
constexpr int kBarY = kTitleCapsY + 24;
constexpr int kCaptionY = kBarY + kBarHeight + 7;
constexpr int kHintMiddle = 154;
constexpr int kSweepWidth = 56;
constexpr uint32_t kSweepMs = 1400;
constexpr uint32_t kProgressGlideMs = 400;
} // namespace startup

int text_width(const std::string &text, const lv_font_t *font)
{
    lv_point_t size = {};
    lv_text_get_size(&size, text.c_str(), font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size.x;
}

// A key name in a small keycap, then what the key does, centered on center_x.
void key_hint(lv_obj_t *root, const char *key, const char *action, int center_x, int middle)
{
    const lv_font_t *key_font = store_font(key, 10);
    const lv_font_t *action_font = store_font(action, 12);
    constexpr int pad = 5, gap = 6, cap_height = 15;
    const int cap_width = text_width(key, key_font) + 2 * pad;
    const int action_width = text_width(action, action_font);
    const int x = center_x - (cap_width + gap + action_width) / 2;
    box(root, x, middle - cap_height / 2, cap_width, cap_height, kPageBackground, 0x48484A, 1, 4);
    label(root, key, x + pad, middle - glyph_middle(key_font, 'b'), cap_width - pad,
          key_font->line_height, key_font, kSecondaryText);
    label(root, action, x + cap_width + gap, middle - glyph_middle(action_font, 'S'),
          action_width + 2, action_font->line_height, action_font, kSecondaryText);
}

// Indeterminate progress: a short segment sweeping across the track. Timed by
// the clock, so redrawing the page does not restart it.
void sweep_progress(lv_obj_t *track)
{
    using namespace startup;
    lv_obj_t *segment = box(track, -kSweepWidth, 0, kSweepWidth, kBarHeight, kSystemBlue,
                            kSystemBlue, 0, kBarHeight / 2);
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, segment);
    // The callback only runs when this value changes, so let it change every tick.
    lv_anim_set_values(&animation, 0, static_cast<int32_t>(kSweepMs));
    lv_anim_set_duration(&animation, kSweepMs);
    lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_custom_exec_cb(&animation, [](lv_anim_t *anim, int32_t) {
        // Cubic ease-in-out from off the left edge to off the right edge.
        const int64_t p = static_cast<int64_t>(lv_tick_get() % kSweepMs) * 1024 / kSweepMs;
        const int64_t q = 1024 - p;
        const int64_t eased = p < 512 ? 4 * p * p * p / (1024 * 1024)
                                      : 1024 - 4 * q * q * q / (1024 * 1024);
        lv_obj_set_x(static_cast<lv_obj_t *>(anim->var),
                     static_cast<int32_t>(-kSweepWidth + (kBarWidth + kSweepWidth) * eased / 1024));
    });
    lv_anim_start(&animation);
}

int progress_width(int percent)
{
    using namespace startup;
    if (percent <= 0) return 0;
    return std::max(kBarHeight, kBarWidth * std::min(percent, 100) / 100);
}

// An alert in the style of Apple's: title, message, and two buttons, the
// focused one filled.
void startup_alert(lv_obj_t *root, const InitializationProgressViewModel &model)
{
    modal_backdrop(root);
    constexpr int width = 256, x = (320 - width) / 2, pad = 14, line_space = 2;
    constexpr int button_height = 26, button_gap = 8;
    const lv_font_t *title_font = store_font(model.error_title, 15, true);
    const lv_font_t *body_font = store_font(model.error_message, 12);
    lv_point_t body = {};
    lv_text_get_size(&body, model.error_message.c_str(), body_font, 0, line_space,
                     width - 2 * pad, LV_TEXT_FLAG_NONE);
    const int body_height =
        std::min<int>(body.y, 3 * body_font->line_height + 2 * line_space);
    const int height =
        pad + title_font->line_height + 4 + body_height + 14 + button_height + pad;
    const int y = 20 + (150 - height) / 2;
    box(root, x, y, width, height, 0x1C1C1E, 0x2C2C2E, 1, 14);

    int row = y + pad;
    center_strong_label(root, model.error_title, x + pad, row, width - 2 * pad,
                        title_font->line_height, title_font, 0xFFFFFF, LV_LABEL_LONG_DOT);
    row += title_font->line_height + 4;
    lv_obj_t *message = center_label(root, model.error_message, x + pad, row, width - 2 * pad,
                                     body_height, body_font, kSecondaryText, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_line_space(message, line_space, 0);
    row += body_height + 14;

    const int button_width = (width - 2 * pad - button_gap) / 2;
    const char *const buttons[] = {"Exit", "Settings"};
    for (int i = 0; i < 2; ++i) {
        const bool focused = model.failure_focus == i;
        const int button_x = x + pad + i * (button_width + button_gap);
        const uint32_t fill = focused ? kSystemBlue : kSystemGray;
        box(root, button_x, row, button_width, button_height, fill, fill, 0, 8);
        const lv_font_t *font = store_font(buttons[i], 13);
        const int text_y = row + button_height / 2 - glyph_middle(font, 'E');
        if (focused)
            center_strong_label(root, buttons[i], button_x, text_y, button_width,
                                font->line_height, font, 0xFFFFFF);
        else
            center_label(root, buttons[i], button_x, text_y, button_width, font->line_height,
                         font, 0xFFFFFF);
    }
}

void scrolling_status_label(lv_obj_t *root, const std::string &text, int x, int y, int w, int h,
                            uint32_t color = 0xCCCC33)
{
    lv_obj_t *obj = label(root, text, x, y, w, h, &lv_font_montserrat_10, color,
                          LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_anim_duration(obj, 4000, LV_PART_MAIN | LV_STATE_DEFAULT);
}

// A grouped settings card. The focused one is outlined in system blue.
lv_obj_t *settings_group(lv_obj_t *root, int y, int height, bool focused)
{
    return box(root, 12, y, 296, height, 0x1C1C1E, focused ? kSystemBlue : 0x2C2C2E, 1, 12);
}

} // namespace

int glyph_middle(const lv_font_t *font, uint32_t letter)
{
    if (!font) return 0;
    lv_font_glyph_dsc_t glyph = {};
    if (!lv_font_get_glyph_dsc(font, &glyph, letter, 0)) return font->line_height / 2;
    return font->line_height - font->base_line - glyph.ofs_y - glyph.box_h / 2;
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

void InitializationProgressPage::render(
    const PageRenderContext &context, const InitializationProgressViewModel &model,
    const std::function<void(lv_obj_t *, int, int, int)> &draw_logo)
{
    using namespace startup;
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, kPageBackground, kPageBackground, 0);
    draw_logo(root, (320 - kLogoSize) / 2, kLogoY, kLogoSize);
    if (model.failed) {
        startup_alert(root, model);
        return;
    }

    const lv_font_t *title_font = store_font(model.title, 17, true);
    center_strong_label(root, model.title, 10, kTitleCapsY - glyph_top(title_font, 'C'), 300,
                        title_font->line_height, title_font, 0xFFFFFF, LV_LABEL_LONG_DOT);

    lv_obj_t *track = box(root, kBarX, kBarY, kBarWidth, kBarHeight, kSystemGray, kSystemGray, 0,
                          kBarHeight / 2);
    lv_obj_set_style_clip_corner(track, true, 0);
    const uint32_t now = lv_tick_get();
    if (model.percent != progress_to_) {
        // Glide on from the drawn value; a new phase starts from empty.
        progress_from_ = progress_to_ >= 0 && model.percent > progress_to_ ? progress_at(now) : 0;
        progress_to_ = model.percent;
        progress_changed_at_ = now;
    }
    if (model.percent < 0) {
        sweep_progress(track);
    } else {
        lv_obj_t *fill = box(track, 0, 0, progress_width(progress_at(now)), kBarHeight,
                             kSystemBlue, kSystemBlue, 0, kBarHeight / 2);
        const uint32_t remaining = kProgressGlideMs - std::min(kProgressGlideMs, now - progress_changed_at_);
        if (remaining > 0) {
            lv_anim_t animation;
            lv_anim_init(&animation);
            lv_anim_set_var(&animation, fill);
            lv_anim_set_user_data(&animation, this);
            lv_anim_set_values(&animation, 0, static_cast<int32_t>(remaining));
            lv_anim_set_duration(&animation, remaining);
            lv_anim_set_custom_exec_cb(&animation, [](lv_anim_t *anim, int32_t) {
                auto *page = static_cast<InitializationProgressPage *>(lv_anim_get_user_data(anim));
                lv_obj_set_width(static_cast<lv_obj_t *>(anim->var),
                                 progress_width(page->progress_at(lv_tick_get())));
            });
            lv_anim_set_completed_cb(&animation, [](lv_anim_t *anim) {
                auto *page = static_cast<InitializationProgressPage *>(lv_anim_get_user_data(anim));
                lv_obj_set_width(static_cast<lv_obj_t *>(anim->var), progress_width(page->progress_to_));
            });
            lv_anim_start(&animation);
        }
    }

    std::string caption = model.source;
    if (model.percent >= 0)
        caption += (caption.empty() ? "" : " \xC2\xB7 ") + std::to_string(model.percent) + "%";
    const lv_font_t *caption_font = store_font(caption, 11);
    center_label(root, caption, 10, kCaptionY, 300, caption_font->line_height, caption_font,
                 kSecondaryText, LV_LABEL_LONG_DOT);
    key_hint(root, "tab", "Settings", 160, kHintMiddle);
}

int InitializationProgressPage::progress_at(uint32_t now) const
{
    const uint32_t elapsed = now - progress_changed_at_;
    if (progress_to_ < 0 || elapsed >= startup::kProgressGlideMs) return progress_to_;
    // Quadratic ease-out.
    const int64_t left = startup::kProgressGlideMs - elapsed;
    return progress_to_ - static_cast<int>((progress_to_ - progress_from_) * left * left /
                                           (startup::kProgressGlideMs * startup::kProgressGlideMs));
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
    const lv_font_t *header_font = store_font("", 12);
    const int header_y = catalog_header::kCenterY - glyph_middle(header_font, 'H');
    label(root, "In category: " + std::to_string(model.show_empty ? 0 : model.selected_index + 1) +
          "/" + std::to_string(model.app_count), 138, header_y, 116, header_font->line_height,
          header_font, 0xFFFFFF, LV_LABEL_LONG_DOT);
    label(root, std::to_string(model.total_count), catalog_header::kCountX,
          header_y, 318 - catalog_header::kCountX, header_font->line_height,
          header_font, 0xFFFFFF, LV_LABEL_LONG_DOT);
    if (model.show_empty) {
        label(root, model.empty_message, 137, 76, 178, 40,
              store_font(model.empty_message, 14), 0xFFFFFF, LV_LABEL_LONG_WRAP);
    } else {
        marquee_label(root, model.name, 137, 58, 178, 27, store_font(model.name, 20, true),
                      0xFFFFFF, title_marquee_, model.name);
        label(root, model.version, 137, 86, 66, 20,
              store_font(model.version, 14), kVersionColor, LV_LABEL_LONG_DOT);
        marquee_label(root, model.author, 207, 86, 108, 20, store_font(model.author, 14),
                      kMaintainerColor, author_marquee_, model.name + model.author);
        label(root, model.size, 137, 105, 68, 20,
              store_font(model.size, 14), kSizeColor, LV_LABEL_LONG_DOT);
        label(root, model.updated, 207, 105, 110, 20,
              store_font(model.updated, 14), kDateColor, LV_LABEL_LONG_DOT);
        if (model.update_available)
            label(root, "UPDATE AVAILABLE", 137, 126, 178, 11,
                  &lv_font_montserrat_10, 0xF0B429);
    }
    if (model.show_status) scrolling_status_label(root, model.status, 110, 138, 205, 12);
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
                           const std::function<int(lv_obj_t *, const std::string &, int, int)> &draw_thumbnail,
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
    document_ = document;
    // The fades are deleted along with the document when the page is cleared.
    lv_obj_add_event_cb(document, [](lv_event_t *event) {
        auto *page = static_cast<AppDetailPage *>(lv_event_get_user_data(event));
        page->document_ = page->top_fade_ = page->bottom_fade_ = nullptr;
    }, LV_EVENT_DELETE, this);

    const appstore::StoreApp &app = model.app;
    const lv_font_t *title_font = store_font(app.name, 20, true);
    marquee_label(document, app.name, 10, 4, 300, title_font->line_height, title_font,
                  0xFFFFFF, title_marquee_, app.id + app.name);
    int y = 8 + title_font->line_height;
    detail_field(document, "VERSION", model.version, 10, y, 146, kVersionColor);
    detail_field(document, "MAINTAINER", model.author, 164, y, 146, kMaintainerColor,
                 &author_marquee_, app.id + model.author);
    y += 34;
    detail_field(document, "SIZE", app.size, 10, y, 146, kSizeColor);
    detail_field(document, "UPDATED", model.updated, 164, y, 146, kDateColor);
    y += 36;

    const uint32_t state_color = !app.installed ? 0x6E7681 :
        (appstore::can_upgrade_app(app) ? 0xF0B429 : 0x52D05D);
    const lv_font_t *state_font = store_font(model.state, 14);
    // Centre the dot on the state label's capitals instead of its line box.
    constexpr int state_dot = 8;
    box(document, 11, y + glyph_middle(state_font, 'I') - state_dot / 2,
        state_dot, state_dot, state_color, state_color, 0, state_dot / 2);
    label(document, model.state, 25, y, 285, 17, state_font,
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
        label(document, "SCREENSHOTS", 10, y, 150, 12, &lv_font_montserrat_10, 0x8B949E);
        y += 16;
        for (const auto &path : screenshots) {
            const int height = draw_thumbnail(document, path, 10, y);
            if (height > 0) {
                y += height + 10;
            } else {
                label(document, "Screenshot unavailable", 10, y, 300, 14,
                      store_font("", 12), 0x8B949E);
                y += 24;
            }
        }
    }
    y = detail_info_row(document, "PACKAGE", app.package.empty() ? "-" : app.package, y);
    y = detail_info_row(document, "CATEGORY", model.categories, y);
    if (model.app.permissions != 0) {
        // Match the info-row captions: same font, colour, and height.
        const lv_font_t *permission_font = &lv_font_montserrat_10;
        constexpr uint32_t caption_color = 0x8B949E;
        constexpr int caption_height = 12;
        const int size = permission_font->line_height;
        lv_point_t caption_size = {};
        lv_text_get_size(&caption_size, "PERMISSIONS", permission_font, 0, 0,
                         LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        label(document, "PERMISSIONS", 10, y + 2, caption_size.x, caption_height,
              permission_font, caption_color);
        permission_icons(document, std::max(76, 10 + caption_size.x + 6),
                         y + 2 + glyph_middle(permission_font, 'P') - size / 2,
                         size, model.app.permissions);
        y += std::max(caption_height, size) + 4;
    }
    y += 6;
    lv_obj_set_height(document, y);
    media.normalize_page(app.id, y, viewport_height);
    const uint32_t now = lv_tick_get();
    lv_obj_set_y(document, -media.page_scroll_at(now));
    glide_page(document, media, now);

    top_fade_ = edge_fade(root, viewport_y, 10, background, false);
    bottom_fade_ = edge_fade(root, viewport_y + viewport_height - 12, 12, background, true);
    show_page_edges(media);
    if (model.show_status) {
        // Fixed above the actions so feedback is visible at any scroll position.
        lv_obj_t *toast = box(root, 0, 132, 320, 17, background, 0x2A3A46, 1);
        lv_obj_set_style_border_side(toast, LV_BORDER_SIDE_TOP, 0);
        scrolling_status_label(root, model.status, 10, 135, 300, 12);
    }
    draw_shortcuts(app);
}

bool AppDetailPage::scroll(DetailMediaState &media)
{
    if (!document_) return false;
    const uint32_t now = lv_tick_get();
    lv_anim_delete(document_, nullptr);
    lv_obj_set_y(document_, -media.page_scroll_at(now));
    glide_page(document_, media, now);
    show_page_edges(media);
    return true;
}

void AppDetailPage::show_page_edges(const DetailMediaState &media)
{
    // Fade only the edges with more of the page beyond them.
    lv_obj_set_flag(top_fade_, LV_OBJ_FLAG_HIDDEN, media.page_scroll() <= 0);
    lv_obj_set_flag(bottom_fade_, LV_OBJ_FLAG_HIDDEN,
                    media.page_scroll() >= media.page_max_scroll());
}

void AppDetailPage::render_confirmation(const PageRenderContext &context,
                                        const ConfirmationViewModel &model)
{
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, kPageBackground, kPageBackground, 0);
    const bool ownership_conflict =
        !model.lines.empty() && model.lines[0] == "FILE OWNERSHIP CONFLICT";
    const bool safety_warning = model.lines.size() >= 4 &&
        model.lines[1].find("cannot verify") != std::string::npos;
    const bool warning = ownership_conflict || safety_warning;
    box(root, 18, 26, 284, 122, 0x1C1C1E, 0x2C2C2E, 1, 14);
    const std::string heading = ownership_conflict ? "Force overwrite?" :
        (safety_warning ? "Safety warning" : "Confirm action");
    const lv_font_t *heading_font = store_font(heading, 15, true);
    center_strong_label(root, heading, 28, 33, 264, heading_font->line_height,
                        heading_font, warning ? 0xFF9F0A : 0xFFFFFF);
    int y = 56;
    for (size_t index = 0; index < model.lines.size() && index < 4; ++index) {
        const std::string &text = model.lines[index];
        const bool action_line = index == 0 && !ownership_conflict;
        const lv_font_t *font = store_font(text, action_line ? 12 : 10);
        marquee_label(root, text, 28, y, 264, font->line_height, font,
                      action_line ? 0xFFFFFF : (warning ? 0xFFD2D2 : kSecondaryText),
                      confirmation_marquees_[index], text, LV_TEXT_ALIGN_CENTER);
        y += action_line ? 18 : 14;
    }
    const char *const buttons[] = {"Yes", "No"};
    for (int i = 0; i < 2; ++i) {
        const bool focused = model.focus == i;
        const int x = 32 + i * 132;
        const uint32_t fill = focused ? (i == 0 && warning ? 0xD63B30 : kSystemBlue)
                                     : kSystemGray;
        box(root, x, 118, 124, 26, fill, fill, 0, 8);
        const lv_font_t *font = store_font(buttons[i], 13);
        center_strong_label(root, buttons[i], x, 131 - glyph_middle(font, 'H'),
                            124, font->line_height, font, 0xFFFFFF);
    }
    key_hint(root, "enter", "Confirm", 160, 159);
}

void AppDetailPage::render_progress(const PageRenderContext &context,
                                    const PackageProgressViewModel &model,
                                    const std::function<void(lv_obj_t *, int, int, int)> &draw_logo)
{
    using namespace startup;
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, kPageBackground, kPageBackground, 0);
    draw_logo(root, (320 - kLogoSize) / 2, kLogoY, kLogoSize);
    const std::string action = model.action + "...";
    const lv_font_t *action_font = store_font(action, 15, true);
    center_strong_label(root, action, 10, kTitleCapsY - glyph_top(action_font, 'H'),
                        300, action_font->line_height, action_font, 0xFFFFFF);
    const lv_font_t *title_font = store_font(model.title, 12);
    marquee_label(root, model.title, 10, 99, 300, title_font->line_height, title_font,
                  0xFFFFFF, progress_title_marquee_, model.action + model.title,
                  LV_TEXT_ALIGN_CENTER);
    constexpr int bar_y = 119;
    lv_obj_t *track = box(root, kBarX, bar_y, kBarWidth, kBarHeight,
                          kSystemGray, kSystemGray, 0, kBarHeight / 2);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);
    if (model.progress >= 0) {
        box(track, 0, 0, progress_width(model.progress), kBarHeight,
            kSystemBlue, kSystemBlue, 0, kBarHeight / 2);
    } else {
        sweep_progress(track);
    }
    const lv_font_t *detail_font = store_font(model.detail, 10);
    marquee_label(root, model.detail, 10, 128, 300, detail_font->line_height, detail_font,
                  kSecondaryText, progress_detail_marquee_, model.detail,
                  LV_TEXT_ALIGN_CENTER);
    const std::string footer = "Keep Store open \xC2\xB7 " +
        std::to_string(model.elapsed_seconds) + "s";
    const lv_font_t *footer_font = store_font(footer, 10);
    center_label(root, footer, 10, kHintMiddle - glyph_middle(footer_font, 'H'),
                 300, footer_font->line_height, footer_font, kSecondaryText,
                 LV_LABEL_LONG_DOT);
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
    box(root, 0, 20, 320, 150, kPageBackground, kPageBackground, 0);

    const lv_font_t *title_font = store_font("Settings", 17, true);
    center_strong_label(root, "Settings", 12, 26 - glyph_top(title_font, 'S'), 296,
                        title_font->line_height, title_font, 0xFFFFFF);

    // Region is a segmented control: the chosen segment is filled blue.
    constexpr int region_y = 48, region_height = 30;
    settings_group(root, region_y, region_height, true);
    const char *const regions[] = {"Auto", "Default", "CN"};
    const char *const codes[] = {"auto", "default", "CN"};
    constexpr int segment_width = 98;
    for (int i = 0; i < 3; ++i) {
        const bool selected = model.region_code == codes[i];
        const int x = 13 + i * segment_width;
        if (selected)
            box(root, x, region_y + 1, segment_width - 2, region_height - 2, kSystemBlue,
                kSystemBlue, 0, 11);
        const lv_font_t *font = store_font(regions[i], 13);
        const int text_y = region_y + region_height / 2 - glyph_middle(font, 'A');
        if (selected)
            center_strong_label(root, regions[i], x, text_y, segment_width - 2,
                                font->line_height, font, 0xFFFFFF);
        else
            center_label(root, regions[i], x, text_y, segment_width - 2, font->line_height,
                         font, kSecondaryText);
    }

    constexpr int source_y = 84, source_height = 46;
    settings_group(root, source_y, source_height, false);
    const std::string source = "Active server \xC2\xB7 " +
        std::string(model.active_region == "CN" ? "CN" : "Default");
    const lv_font_t *caption_font = store_font(source, 11);
    label(root, source, 24, source_y + 6, 272, caption_font->line_height, caption_font,
          kSecondaryText, LV_LABEL_LONG_DOT);
    const lv_font_t *source_font = store_font(model.registry_url, 13);
    marquee_label(root, model.registry_url, 24, source_y + 23, 272,
                  source_font->line_height, source_font, 0xFFFFFF, source_marquee_,
                  model.registry_url, LV_TEXT_ALIGN_LEFT);

    if (!model.status.empty()) {
        // Keep scrolling status text above the fixed shortcut row.
        lv_obj_t *status = label(root, model.status, 16, 134, 288, 14,
                                 store_font(model.status, 11),
                                 model.status_is_error ? 0xFF453A : kSecondaryText,
                                 LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_anim_duration(status, 8000, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

} // namespace appstore_ui
