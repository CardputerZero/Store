/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "appstore_presenter.hpp"

#include "appstore_fonts.hpp"
#include "appstore_paths.hpp"
#include "appstore_shortcut_bar.hpp"

#include <cctype>

namespace appstore_ui {
namespace {

constexpr int kCategoryX = 28;
constexpr int kCategoryY = 24;
constexpr int kCategoryWidth = 86;
constexpr int kCategoryKeyWidth = 18;
constexpr uint32_t kStatusVisibleMs = 6000;

lv_obj_t *label(lv_obj_t *parent, const std::string &text, int x, int y, int w, int h,
                const lv_font_t *font, uint32_t color,
                lv_label_long_mode_t mode = LV_LABEL_LONG_CLIP)
{
    lv_obj_t *obj = lv_label_create(parent);
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

void strong_label(lv_obj_t *parent, const std::string &text, int x, int y, int w, int h,
                  const lv_font_t *font, uint32_t color,
                  lv_label_long_mode_t mode = LV_LABEL_LONG_DOT)
{
    label(parent, text, x, y, w, h, font, color, mode);
    label(parent, text, x + 1, y, w, h, font, color, mode);
}

void center_strong_label(lv_obj_t *parent, const std::string &text, int x, int y,
                         int w, int h, const lv_font_t *font, uint32_t color,
                         lv_label_long_mode_t mode = LV_LABEL_LONG_CLIP)
{
    lv_obj_t *first = label(parent, text, x, y, w, h, font, color, mode);
    lv_obj_set_style_text_align(first, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_t *second = label(parent, text, x + 1, y, w, h, font, color, mode);
    lv_obj_set_style_text_align(second, LV_TEXT_ALIGN_CENTER, 0);
}

void box(lv_obj_t *root, int x, int y, int w, int h, uint32_t color,
         uint32_t border, int border_width, int radius)
{
    lv_obj_t *obj = lv_obj_create(root);
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, border_width, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(border), 0);
}

std::string app_initial(const appstore::StoreApp &app)
{
    for (char ch : app.name) {
        const unsigned char value = static_cast<unsigned char>(ch);
        if (std::isalnum(value))
            return std::string(1, static_cast<char>(std::toupper(value)));
    }
    return "A";
}

constexpr int kCounterIconGap = 4;

int counter_width(const std::string &text)
{
    lv_point_t size = {};
    lv_text_get_size(&size, text.c_str(), store_font("", 14), 0, 0, LV_COORD_MAX,
                     LV_TEXT_FLAG_NONE);
    return size.x;
}

// Gray header count at x, centered on `middle`; "..." if it needs more than `width`.
void counter_label(lv_obj_t *root, const std::string &text, int x, int width, int middle)
{
    const lv_font_t *font = store_font("", 14);
    label(root, text, x, middle - glyph_middle(font, '0'), width, font->line_height, font,
          catalog_home::kMuted, LV_LABEL_LONG_DOT);
}

// Gray count, right-aligned at `right` and centered on `middle`. Returns its left edge.
int header_count(lv_obj_t *root, const std::string &text, int right, int middle)
{
    const int width = counter_width(text);
    counter_label(root, text, right - width, width + 4, middle);
    return right - width;
}

// Catalog size: Store icon, then the count. Returns the icon's left edge.
int draw_catalog_total(lv_obj_t *root, AppStoreImageRenderer &images, int total,
                       int right, int middle)
{
    // The Store artwork is a badge with its own padding, so the box has to be
    // larger than the category's letter height to look the same size.
    constexpr int icon_size = 18;
    const int icon_x = header_count(root, std::to_string(total), right, middle) -
                       kCounterIconGap - icon_size;
    if (lv_obj_t *icon = images.draw_packaged(root, "store_100.png", icon_x, 0)) {
        const int scale = std::max(1, std::min(icon_size * 256 / lv_obj_get_width(icon),
                                               icon_size * 256 / lv_obj_get_height(icon)));
        lv_image_set_scale(icon, static_cast<uint16_t>(scale));
        lv_image_set_inner_align(icon, LV_IMAGE_ALIGN_CENTER);
        lv_obj_set_size(icon, icon_size, icon_size);
        lv_obj_set_y(icon, middle - icon_size / 2);
    } else {
        const lv_font_t *font = store_font("", 14);
        strong_label(root, "*", icon_x, middle - glyph_middle(font, '*'), icon_size,
                     font->line_height, font, catalog_home::kMuted);
    }
    return icon_x;
}

// Selected app within the category: the up/down keys that change it, then "x / y".
// Returns the group's left edge.
int draw_selection_position(lv_obj_t *root, int position, int count, int right, int middle)
{
    constexpr int key = 16, key_gap = 2;
    const int text_x = header_count(root, std::to_string(position) + " / " +
                                    std::to_string(count), right, middle);
    const int down_x = text_x - kCounterIconGap - key / 2;
    const int up_x = down_x - key - key_gap;
    draw_arrow_key(root, ArrowKey::Down, down_x, middle, key, key);
    draw_arrow_key(root, ArrowKey::Up, up_x, middle, key, key);
    return up_x - key / 2;
}

} // namespace

AppStorePresenter::AppStorePresenter(
    AppStoreSessionState &session, CatalogController &catalog,
    AppStoreViewModelFactory &view_models,
    InitializationProgressPage &initialization_page,
    CatalogDisplayPage &catalog_page, AppDetailPage &detail_page,
    StoreSettingsPage &settings_page, AppStoreImageRenderer &images,
    std::function<void()> draw_system_bar)
    : session_(session), catalog_(catalog), view_models_(view_models),
      initialization_page_(initialization_page), catalog_page_(catalog_page),
      detail_page_(detail_page), settings_page_(settings_page), images_(images),
      draw_system_bar_(std::move(draw_system_bar))
{
}

PageRenderContext AppStorePresenter::context(AppStoreUiPage &page)
{
    return {[this, &page]() { prepare(page); }, draw_system_bar_,
            [](const std::string &text, const lv_font_t *font) {
                return font_for_text(text, font);
            }};
}

void AppStorePresenter::prepare(AppStoreUiPage &page)
{
    page.clear_content();
    lv_obj_t *root = page.screen();
    rendered_root_ = root;
    images_.begin_frame();
    lv_obj_set_style_bg_color(root, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
}

void AppStorePresenter::draw_category_selector(lv_obj_t *root)
{
    const std::string category = appstore::upper_ascii(session_.catalog.current_category_name());
    const lv_font_t *font = store_font(category, 14, true);
    marquee_label(root, category, kCategoryX, kCategoryY, kCategoryWidth, 20, font, 0xFFFFFF,
                  category_marquee_, category, LV_TEXT_ALIGN_CENTER);
    // The name is uppercase: center on its capitals, not on the line box,
    // which also reserves room below the baseline for descenders.
    const int middle = kCategoryY + glyph_middle(font, 'H');
    const int right_key_x = kCategoryX + kCategoryWidth + 10;
    draw_arrow_key(root, ArrowKey::Left, kCategoryX - 10, middle, kCategoryKeyWidth);
    draw_arrow_key(root, ArrowKey::Right, right_key_x, middle, kCategoryKeyWidth);

    // Header counters, right to left: catalog size, then the selected app's
    // position in the category. The category's own position follows its right
    // key and gets whatever room is left.
    constexpr int counter_gap = 10;
    int free_right = draw_catalog_total(root, images_,
                                        static_cast<int>(session_.catalog.apps().size()),
                                        catalog_home::kRight, middle) - counter_gap;
    const int selected = session_.catalog.selected_index();
    const int visible = static_cast<int>(session_.catalog.visible().size());
    if (selected >= 0 && selected < visible)
        free_right = draw_selection_position(root, selected + 1, visible, free_right, middle) -
                     counter_gap;

    const auto &categories = session_.catalog.categories();
    const int index = session_.catalog.category_index();
    if (index < 0 || index >= static_cast<int>(categories.size())) return;
    const std::string position = std::to_string(index + 1) + " / " +
                                 std::to_string(categories.size());
    const int position_x = right_key_x + kCategoryKeyWidth / 2 + kCounterIconGap;
    const int width = std::min(counter_width(position) + 4, free_right - position_x);
    if (width > 0) counter_label(root, position, position_x, width, middle);
}

void AppStorePresenter::draw_home_icon_panel(lv_obj_t *root,
                                              const appstore::StoreApp *app)
{
    using catalog_home::kIconCenterY;
    using catalog_home::kIconSize;
    using catalog_home::kIconX;
    const lv_font_t *font = &lv_font_montserrat_20;
    if (!app) {
        center_strong_label(root, "-", kIconX, kIconCenterY - glyph_middle(font, '-'),
                            kIconSize, font->line_height, font, 0x9A9A9A);
    } else if (!images_.draw_app_icon(root, session_.app_dir, *app)) {
        center_strong_label(root, app_initial(*app), kIconX,
                            kIconCenterY - glyph_middle(font, 'H'), kIconSize,
                            font->line_height, font, app->installed ? 0x52D05D : 0xFFFFFF);
    }
}

bool AppStorePresenter::draw_detail_background(lv_obj_t *root,
                                                const appstore::StoreApp &app)
{
    const auto screenshots = detail_screenshot_paths(session_.app_dir, app);
    session_.detail_media.normalize_images(app.id, static_cast<int>(screenshots.size()));
    return !screenshots.empty() &&
           images_.draw_screenshot(root, screenshots[session_.detail_media.image_index()]);
}

void AppStorePresenter::render(Screen screen, bool registry_operation_running)
{
    rendered_root_ = nullptr;
    switch (screen) {
        case Screen::StartupSync:
            initialization_page_.render(context(initialization_page_), view_models_.initialization());
            break;
        case Screen::Home: {
            lv_obj_t *root = catalog_page_.screen();
            catalog_page_.render(
                context(catalog_page_), view_models_.catalog(lv_tick_get(), kStatusVisibleMs),
                [this, root]() { draw_category_selector(root); },
                [this, root]() { draw_home_icon_panel(root, session_.catalog.selected_app()); },
                [this, root]() {
                    AppStoreShortcutBar::render_catalog(
                        root, session_.catalog.selected_app());
                });
            break;
        }
        case Screen::Detail: {
            appstore::StoreApp *selected = catalog_.ensure_selected();
            const auto screenshots = selected ? detail_screenshot_paths(session_.app_dir, *selected)
                                              : std::vector<std::string>{};
            detail_page_.render(
                context(detail_page_),
                view_models_.detail(selected, lv_tick_get(), kStatusVisibleMs),
                session_.detail_media, screenshots,
                [this](lv_obj_t *parent, const std::string &path, int x, int y) {
                    return images_.draw_thumbnail(parent, path, x, y);
                },
                [this](const appstore::StoreApp &app) {
                    AppStoreShortcutBar::render_detail(
                        detail_page_.screen(), session_.app_dir, app);
                });
            break;
        }
        case Screen::Confirm:
            detail_page_.render_confirmation(context(detail_page_), view_models_.confirmation());
            break;
        case Screen::Progress:
            detail_page_.render_progress(context(detail_page_), view_models_.progress(lv_tick_get()));
            break;
        case Screen::ErrorDialog:
            detail_page_.render_error(context(detail_page_), view_models_.error());
            break;
        case Screen::Registry: {
            const StoreSettingsViewModel model =
                view_models_.settings(registry_operation_running);
            settings_page_.render(context(settings_page_), model);
            AppStoreShortcutBar::render_settings(
                settings_page_.screen(), model.has_entry,
                model.has_entry && model.entry.builtin,
                !model.loading && !model.operation_running &&
                    !model.region_commit_pending);
            break;
        }
        case Screen::RegistryEdit:
            settings_page_.render_editor(context(settings_page_), view_models_.registry_editor());
            break;
        case Screen::ShareCode:
            catalog_page_.render_text_entry(context(catalog_page_), view_models_.share_code());
            break;
        case Screen::Search: {
            const SearchPageViewModel model = view_models_.search();
            catalog_page_.render_search(context(catalog_page_), model);
            break;
        }
        case Screen::Screenshots: {
            appstore::StoreApp *app = catalog_.ensure_selected();
            std::vector<std::string> screenshots;
            if (app) {
                screenshots = detail_screenshot_paths(session_.app_dir, *app);
                session_.detail_media.normalize_images(app->id,
                                                       static_cast<int>(screenshots.size()));
            }
            session_.detail_media.hide_overlay_if_elapsed(lv_tick_get(), 2000);
            ScreenshotOverlayViewModel model;
            model.has_app = app != nullptr;
            model.has_screenshots = !screenshots.empty();
            model.overlay_visible = session_.detail_media.overlay_visible();
            model.image_index = session_.detail_media.image_index();
            model.image_count = static_cast<int>(screenshots.size());
            model.loading = session_.detail_media.loading();
            model.load_failed = session_.detail_media.load_failed();
            detail_page_.render_screenshot_overlay(
                context(detail_page_), model,
                [this, app]() { if (app) draw_detail_background(detail_page_.screen(), *app); });
            break;
        }
    }
    if (session_.exit_hint_visible && rendered_root_) render_exit_hint(rendered_root_);
}

} // namespace appstore_ui
