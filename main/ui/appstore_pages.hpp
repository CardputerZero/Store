/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "appstore_view_models.hpp"
#include "detail_media_state.hpp"
#include "horizontal_marquee.hpp"
#include "ui_app_page.hpp"

#include <functional>
#include <string>
#include <vector>

namespace appstore_ui {

void render_exit_hint(lv_obj_t *root);

// Middle of a glyph's ink, measured from the top of the font's line box.
int glyph_middle(const lv_font_t *font, uint32_t letter);

enum class ArrowKey { Left, Up, Right, Down };

// A small raised button with a solid arrow, for on-screen navigation hints.
void draw_arrow_key(lv_obj_t *parent, ArrowKey arrow, int center_x, int center_y,
                    int width = 18, int height = 16);

// Home catalog geometry shared by the page and the icon panel.
namespace catalog_home {
// Content edges: the category's left key and the header counters' right end.
constexpr int kLeft = 9;
constexpr int kRight = 312;
// Left rows: the name's capitals, two tag rows and the permission icons,
// centered as one block between the header and the bookmark tabs.
constexpr int kTopY = 54;
constexpr int kTagHeight = 17;
// Clears the 20 px name, including its descenders.
constexpr int kTagRow1Y = kTopY + 23;
constexpr int kTagRow2Y = kTagRow1Y + kTagHeight + 3;
constexpr int kPermissionSize = 14;
constexpr int kPermissionsY = kTagRow2Y + kTagHeight + 6;
// The icon on the right fills the height between the header and the bookmark
// tabs, centered on the left block.
constexpr int kIconSize = 100;
constexpr int kIconX = kRight - kIconSize;
constexpr int kIconCenterY = (kTopY + kPermissionsY + kPermissionSize) / 2;
constexpr int kIconY = kIconCenterY - kIconSize / 2;
constexpr int kDetailsX = kLeft;
constexpr int kDetailsRight = kIconX - 9;
constexpr uint32_t kMuted = 0x8B949E;
} // namespace catalog_home

// One line of text; when it does not fit, it bounces like APPLaunch's
// selected title. Text that fits is placed according to align.
lv_obj_t *marquee_label(lv_obj_t *parent, const std::string &text, int x, int y,
                        int width, int height, const lv_font_t *font, uint32_t color,
                        HorizontalMarquee &state, const std::string &key,
                        lv_text_align_t align = LV_TEXT_ALIGN_LEFT, bool strong = false);

struct PageRenderContext {
    std::function<void()> prepare;
    std::function<void()> draw_system_bar;
    std::function<const lv_font_t *(const std::string &, const lv_font_t *)> font_for_text;
};

class AppStoreUiPage : public AppPageRoot
{
public:
    AppStoreUiPage();
    ~AppStoreUiPage() override = default;

    void activate();
};

class InitializationProgressPage : public AppStoreUiPage
{
public:
    // draw_logo draws the Store icon in the size x size square at x, y.
    void render(const PageRenderContext &context,
                const InitializationProgressViewModel &model,
                const std::function<void(lv_obj_t *, int, int, int)> &draw_logo);
private:
    int progress_at(uint32_t now) const;

    // The bar glides from the drawn value when progress is reported.
    int progress_from_ = 0;
    int progress_to_ = -1;
    uint32_t progress_changed_at_ = 0;
};

class CatalogDisplayPage : public AppStoreUiPage
{
public:
    void render(const PageRenderContext &context,
                const CatalogDisplayViewModel &model,
                const std::function<void()> &draw_category,
                const std::function<void()> &draw_icon,
                const std::function<void()> &draw_shortcuts);
    void render_text_entry(const PageRenderContext &context,
                           const TextEntryViewModel &model);
    void render_search(const PageRenderContext &context,
                       const SearchPageViewModel &model);
private:
    HorizontalMarquee title_marquee_;
    HorizontalMarquee author_marquee_;
    HorizontalMarquee search_marquee_;
};

class AppDetailPage : public AppStoreUiPage
{
public:
    void render(const PageRenderContext &context,
                const AppDetailViewModel &model,
                DetailMediaState &media, const std::vector<std::string> &screenshots,
                const std::function<bool(lv_obj_t *, const std::string &, int, int)> &draw_thumbnail,
                const std::function<void(const appstore::StoreApp &)> &draw_shortcuts);
    // Glides the drawn page to the media's scroll target without rebuilding
    // it. False when no detail page is drawn.
    bool scroll(DetailMediaState &media);
    void render_confirmation(const PageRenderContext &context,
                             const ConfirmationViewModel &model);
    void render_progress(const PageRenderContext &context,
                         const PackageProgressViewModel &model);
    void render_error(const PageRenderContext &context,
                      const ErrorDialogViewModel &model);
    void render_screenshot_overlay(const PageRenderContext &context,
                                   const ScreenshotOverlayViewModel &model,
                                   const std::function<void()> &draw_background);
private:
    void show_page_edges(const DetailMediaState &media);

    HorizontalMarquee title_marquee_;
    HorizontalMarquee author_marquee_;
    // The drawn page, cleared when the page content is deleted.
    lv_obj_t *document_ = nullptr;
    lv_obj_t *top_fade_ = nullptr;
    lv_obj_t *bottom_fade_ = nullptr;
};

class StoreSettingsPage : public AppStoreUiPage
{
public:
    void render(const PageRenderContext &context,
                const StoreSettingsViewModel &model);
    void render_editor(const PageRenderContext &context,
                       const RegistryEditorViewModel &model);
private:
    HorizontalMarquee source_marquee_;
    HorizontalMarquee url_marquee_;
};

} // namespace appstore_ui
