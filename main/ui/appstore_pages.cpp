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
lv_obj_t *edge_fade(lv_obj_t *root, int y, int height, uint32_t color, bool towards_bottom)
{
    lv_obj_t *fade = box(root, 0, y, 320, height, color, color, 0);
    lv_obj_set_style_bg_grad_color(fade, lv_color_hex(color), 0);
    lv_obj_set_style_bg_grad_dir(fade, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_main_opa(fade, towards_bottom ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_opa(fade, towards_bottom ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    return fade;
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

// A small chevron, drawn rather than taken from a font so it stays crisp.
void chevron(lv_obj_t *root, int x, int middle, bool left, uint32_t color)
{
    constexpr int arm = 5;
    for (int row = 0; row < arm; ++row) {
        const int inset = left ? arm - 1 - row : row;
        box(root, x + inset, middle - arm + row, 2, 2, color, color, 0);
        box(root, x + inset, middle + arm - 2 - row, 2, 2, color, color, 0);
    }
}

// Shows the slice of a field that holds the caret, with the caret drawn in it.
void editor_field(lv_obj_t *root, const std::string &value, size_t caret, int x, int y,
                  int width, bool blink)
{
    const lv_font_t *font = store_font(value, 13);
    lv_obj_t *viewport = lv_obj_create(root);
    lv_obj_remove_style_all(viewport);
    lv_obj_set_pos(viewport, x, y);
    lv_obj_set_size(viewport, width, font->line_height);
    lv_obj_clear_flag(viewport, LV_OBJ_FLAG_SCROLLABLE);

    const std::string before = value.substr(0, std::min(caret, value.size()));
    const int caret_x = text_width(before, font);
    // Keep the caret a few pixels inside the field, scrolling the text to do it.
    constexpr int margin = 8;
    int scroll = 0;
    if (caret_x > width - margin) scroll = caret_x - (width - margin);
    const int text_width_px = text_width(value, font);
    if (scroll > 0 && text_width_px - scroll < width - margin)
        scroll = std::max(0, text_width_px - (width - margin));

    if (!value.empty())
        label(viewport, value, -scroll, 0, LV_SIZE_CONTENT, font->line_height, font, 0xFFFFFF);
    lv_obj_t *caret_bar = box(viewport, caret_x - scroll, 1, 1, font->line_height - 2, kSystemBlue,
                              kSystemBlue, 0);
    if (!blink) return;
    lv_anim_t blink_anim;
    lv_anim_init(&blink_anim);
    lv_anim_set_var(&blink_anim, caret_bar);
    lv_anim_set_values(&blink_anim, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&blink_anim, 530);
    lv_anim_set_playback_duration(&blink_anim, 530);
    lv_anim_set_repeat_count(&blink_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&blink_anim, [](void *obj, int32_t value) {
        lv_obj_set_style_bg_opa(static_cast<lv_obj_t *>(obj), static_cast<lv_opa_t>(value), 0);
    });
    lv_anim_start(&blink_anim);
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
    box(root, 0, 20, 320, 150, kPageBackground, kPageBackground, 0);

    const lv_font_t *title_font = store_font("Settings", 17, true);
    center_strong_label(root, "Settings", 12, 26 - glyph_top(title_font, 'S'), 296,
                        title_font->line_height, title_font, 0xFFFFFF);

    // Region is a segmented control: the chosen segment is filled blue.
    constexpr int region_y = 48, region_height = 30;
    const bool region_focused = model.focus == 0;
    settings_group(root, region_y, region_height, region_focused);
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
    settings_group(root, source_y, source_height, model.focus == 1 && model.has_entry);
    if (!model.has_entry) {
        const std::string message =
            model.loading ? "Loading registries\u2026" : "No registries configured";
        const lv_font_t *font = store_font(message, 13);
        center_label(root, message, 24, source_y + (source_height - font->line_height) / 2, 272,
                     font->line_height, font, kSecondaryText, LV_LABEL_LONG_DOT);
    } else {
        const auto &entry = model.entry;
        const std::string source = model.region_code == "auto"
            ? "Source \xC2\xB7 " + model.active_region : "Source";
        const lv_font_t *caption_font = store_font(source, 11);
        label(root, source, 24, source_y + 6, 180, caption_font->line_height, caption_font,
              kSecondaryText, LV_LABEL_LONG_DOT);
        const std::string summary = entry.count + " apps \xC2\xB7 " +
                                    std::to_string(model.selected_index + 1) + " of " +
                                    std::to_string(model.entry_count);
        lv_obj_t *count = label(root, summary, 150, source_y + 6, 146, caption_font->line_height,
                                caption_font, kSecondaryText, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(count, LV_TEXT_ALIGN_RIGHT, 0);

        const lv_font_t *name_font = store_font(entry.name, 14);
        // Chevrons bracket the name, so left/right reads as a choice.
        constexpr int name_x = 40, name_right = 280;
        const int name_middle = source_y + 21 + name_font->line_height / 2;
        const uint32_t arrow = model.focus == 1 ? kSystemBlue : 0x636366;
        chevron(root, 24, name_middle, true, arrow);
        chevron(root, name_right, name_middle, false, arrow);
        marquee_label(root, entry.name, name_x, source_y + 21, name_right - name_x,
                      name_font->line_height, name_font, 0xFFFFFF, source_marquee_,
                      entry.url + entry.name, LV_TEXT_ALIGN_CENTER);
    }

    if (!model.status.empty()) {
        // Above the bookmark tabs, so a long status can scroll without covering them.
        lv_obj_t *status = label(root, model.status, 16, 134, 288, 14,
                                 store_font(model.status, 11),
                                 model.status_is_error ? 0xFF453A : kSecondaryText,
                                 LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_anim_duration(status, 8000, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void StoreSettingsPage::render_editor(const PageRenderContext &context,
                                      const RegistryEditorViewModel &model)
{
    context.prepare();
    context.draw_system_bar();
    lv_obj_t *root = screen();
    box(root, 0, 20, 320, 150, kPageBackground, kPageBackground, 0);

    const std::string title = model.editing ? "Edit Registry" : "Add Registry";
    const lv_font_t *title_font = store_font(title, 17, true);
    center_strong_label(root, title, 12, 26 - glyph_top(title_font, 'E'), 296,
                        title_font->line_height, title_font, 0xFFFFFF);

    const char *const captions[] = {"Name", "URL"};
    const std::string values[] = {model.name, model.url};
    // The second field stops above the bookmark tabs.
    constexpr int row_y[] = {48, 92};
    for (int i = 0; i < 2; ++i) {
        constexpr int height = 40;
        const bool focused = model.editing_field && model.focus == i;
        settings_group(root, row_y[i], height, focused);
        const lv_font_t *caption_font = store_font(captions[i], 11);
        label(root, captions[i], 24, row_y[i] + 4, 272, caption_font->line_height, caption_font,
              focused ? kSystemBlue : kSecondaryText);
        if (!model.editing_field) {
            const lv_font_t *font = store_font(values[i], 13);
            label(root, values[i], 24, row_y[i] + 19, 272, font->line_height, font, 0xFFFFFF,
                  LV_LABEL_LONG_DOT);
        } else if (!focused && values[i].empty()) {
            const lv_font_t *font = store_font("Required", 13);
            label(root, "Required", 24, row_y[i] + 19, 272, font->line_height, font, 0x636366);
        } else {
            // The caret keeps the typed end in view, scrolling the field with it.
            editor_field(root, values[i], focused ? model.caret : values[i].size(), 24,
                         row_y[i] + 19, 272, focused);
        }
    }
    if (!model.status.empty()) {
        lv_obj_t *status = label(root, model.status, 16, 136, 240, 14,
                                 store_font(model.status, 11),
                                 model.status_is_error ? 0xFF453A : kSecondaryText,
                                 LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(status, LV_TEXT_ALIGN_CENTER, 0);
    }
}

} // namespace appstore_ui
