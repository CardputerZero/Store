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
                const std::function<int(lv_obj_t *, const std::string &, int, int)> &draw_thumbnail,
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
private:
    HorizontalMarquee source_marquee_;
};

} // namespace appstore_ui
