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
    void render(const PageRenderContext &context,
                const InitializationProgressViewModel &model);
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
};

class AppDetailPage : public AppStoreUiPage
{
public:
    void render(const PageRenderContext &context,
                const AppDetailViewModel &model,
                DetailMediaState &media, const std::vector<std::string> &screenshots,
                const std::function<bool(lv_obj_t *, const std::string &, int, int)> &draw_thumbnail,
                const std::function<bool(lv_obj_t *, const std::string &, int, int)> &draw_packaged,
                const std::function<void(const appstore::StoreApp &)> &draw_shortcuts);
    void render_confirmation(const PageRenderContext &context,
                             const ConfirmationViewModel &model);
    void render_progress(const PageRenderContext &context,
                         const PackageProgressViewModel &model);
    void render_error(const PageRenderContext &context,
                      const ErrorDialogViewModel &model);
    void render_screenshot_overlay(const PageRenderContext &context,
                                   const ScreenshotOverlayViewModel &model,
                                   const std::function<void()> &draw_background);
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
