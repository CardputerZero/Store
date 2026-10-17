/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "appstore_shortcut_bar.hpp"
#include "appstore_fonts.hpp"

namespace appstore_ui {
namespace {

constexpr int kScreenHeight = 170;

// A bookmark tab: the same tinted chip as the detail fields, but the rounded
// bottom continues past the screen, so the page edge cuts it with no border.
void shortcut_label(lv_obj_t *root, int center_x,
                    const std::string &text, uint32_t color, int width = 52,
                    bool active = true)
{
    constexpr int top = 152, radius = 7;
    const int x = center_x - width / 2;
    const int visible = kScreenHeight - top;
    lv_obj_t *tab = lv_obj_create(root);
    lv_obj_remove_style_all(tab);
    lv_obj_set_pos(tab, x, top);
    lv_obj_set_size(tab, width, visible + radius);
    lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(tab, radius, 0);
    lv_obj_set_style_bg_color(tab, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(tab, active ? LV_OPA_50 : LV_OPA_20, 0);
    lv_obj_set_style_border_width(tab, 1, 0);
    lv_obj_set_style_border_color(tab, lv_color_hex(color), 0);
    lv_obj_set_style_border_opa(tab, LV_OPA_50, 0);
    lv_obj_set_style_border_side(tab, static_cast<lv_border_side_t>(
        LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_LEFT | LV_BORDER_SIDE_RIGHT), 0);

    const lv_font_t *font = store_font(text, 12);
    lv_obj_t *label = lv_label_create(root);
    lv_obj_set_pos(label, x + 2, top + (visible - font->line_height) / 2);
    lv_obj_set_size(label, width - 4, font->line_height);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(active ? 0xFFFFFF : 0x8E8E93), 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_label_set_text(label, text.c_str());
}

} // namespace

void AppStoreShortcutBar::render_catalog(lv_obj_t *root,
                                         const appstore::StoreApp *app)
{
    shortcut_label(root, 48, "settings", 0xF0B429);
    shortcut_label(root, 104, "share", 0x168CE5);
    shortcut_label(root, 160, "search", 0x7ED957);
    if (app && appstore::can_install_app(*app))
        shortcut_label(root, 216, app->installed ? "reinstall" : "install", 0xB069FF);
    if (app)
        shortcut_label(root, 272, "detail", 0xFF8A2A);
}

void AppStoreShortcutBar::render_detail(lv_obj_t *root, const std::string &app_dir,
                                        const appstore::StoreApp &app)
{
    (void)app_dir;
    shortcut_label(root, 48, "back", 0xF0B429);
    if (!app.installed && appstore::can_install_app(app))
        shortcut_label(root, 160, "install", 0xB069FF);
    if (appstore::can_reinstall_app(app))
        shortcut_label(root, 160, "reinstall", 0xB069FF);
    if (appstore::can_upgrade_app(app))
        shortcut_label(root, 216, "upgrade", 0xB069FF);
    if (app.installed)
        shortcut_label(root, 272, "remove", 0xFF5B4A);
}

void AppStoreShortcutBar::render_settings(lv_obj_t *root, bool has_entry,
                                          bool builtin_entry,
                                          bool actions_available)
{
    if (!actions_available) return;
    shortcut_label(root, 48, "add", 0x0A84FF);
    if (has_entry && !builtin_entry)
        shortcut_label(root, 104, "edit", 0x0A84FF);
    if (has_entry && !builtin_entry)
        shortcut_label(root, 216, "delete", 0xFF453A);
    shortcut_label(root, 272, "sync", 0x30D158);
}

void AppStoreShortcutBar::render_editor(lv_obj_t *root, bool save_active)
{
    shortcut_label(root, 272, "save", save_active ? 0x30D158 : 0x3A3A3C, 52, save_active);
}

} // namespace appstore_ui
