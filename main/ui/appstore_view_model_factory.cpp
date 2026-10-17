/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "appstore_view_model_factory.hpp"

#include "detail_action_controller.hpp"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace appstore_ui {

using namespace appstore;

namespace {

std::string human_size(const std::string &value)
{
    if (value.empty()) return "-";
    char *end = nullptr;
    const unsigned long long bytes = std::strtoull(value.c_str(), &end, 10);
    if (!end || *end != '\0') return value;

    const char *unit = "B";
    double amount = static_cast<double>(bytes);
    if (bytes >= 1024ULL * 1024ULL * 1024ULL) {
        amount /= 1024.0 * 1024.0 * 1024.0;
        unit = "G";
    } else if (bytes >= 1024ULL * 1024ULL) {
        amount /= 1024.0 * 1024.0;
        unit = "M";
    } else if (bytes >= 1024ULL) {
        amount /= 1024.0;
        unit = "K";
    }

    std::ostringstream out;
    out << std::fixed << std::setprecision(unit[0] == 'B' ? 0 : 1);
    out << amount << unit;
    std::string result = out.str();
    const std::string redundant_decimal = std::string(".0") + unit;
    const size_t decimal = result.rfind(redundant_decimal);
    if (decimal != std::string::npos && decimal + redundant_decimal.size() == result.size())
        result.erase(decimal, 2);
    result.insert(result.size() - 1, " ");
    if (unit[0] != 'B') result += "B";
    return result;
}

// What the startup sync is doing, as the startup screen's title.
std::string startup_title(const SyncStatus &status)
{
    if (status.cancel_requested) return "Cancelling…";
    if (status.phase.empty() || status.phase == "network" || status.phase == "config")
        return "Connecting…";
    if (status.phase == "catalog") return "Preparing Apps…";
    if (status.phase == "assets") return "Loading Artwork…";
    return "Updating Catalog…";
}

// The registry's host name, or the address itself when it has none.
std::string registry_host(const std::string &url)
{
    const size_t scheme = url.find("://");
    if (scheme == std::string::npos) return url;
    const size_t start = scheme + 3;
    const std::string host = url.substr(start, url.find('/', start) - start);
    return host.empty() ? url : host;
}

std::string sentence(std::string text)
{
    if (!text.empty() && std::string(".!?").find(text.back()) == std::string::npos) text += '.';
    return text;
}

std::string friendly_updated_at(std::string value)
{
    if (value.empty()) return "-";
    if (value.size() > 10 && value[10] == 'T') value[10] = ' ';
    if (value.size() > 16) value.resize(16);
    return value;
}

std::string single_line(std::string value)
{
    for (char &ch : value) {
        if (ch == '\n' || ch == '\r' || ch == '\t') ch = ' ';
    }
    return value;
}

} // namespace

AppStoreViewModelFactory::AppStoreViewModelFactory(AppStoreSessionState &session,
                                                   PackageJobState &package_job,
                                                   ShareCodeState &share_code)
    : session_(session), package_job_(package_job), share_code_(share_code)
{
}

CatalogDisplayViewModel AppStoreViewModelFactory::catalog(
    uint32_t now, uint32_t status_visible_ms) const
{
    CatalogDisplayViewModel model;
    model.selected_index = session_.catalog.selected_index();
    model.app_count = static_cast<int>(session_.catalog.visible().size());
    model.show_empty = session_.catalog.visible().empty();
    model.show_status = session_.status.visible(now, status_visible_ms);
    model.status = session_.status.value();
    model.total_count = static_cast<int>(session_.catalog.apps().size());
    model.empty_message = session_.catalog.current_category_name() == "Installed"
        ? "No installed apps" : "No apps";
    if (const auto *app = session_.catalog.selected_app()) {
        model.name = single_line(app->name);
        model.version = single_line(app->version.empty() ? "-" : app->version);
        model.author = single_line(app->author.empty() ? "-" : app->author);
        model.size = human_size(app->size);
        model.updated = app->updated_at.empty() ? "-" : app->updated_at.substr(0, 10);
        model.permissions = app->permissions;
    }
    return model;
}

AppDetailViewModel AppStoreViewModelFactory::detail(StoreApp *selected, uint32_t now,
                                                    uint32_t status_visible_ms)
{
    AppDetailViewModel model;
    model.has_app = selected != nullptr;
    if (!selected) return model;
    model.app = *selected;
    model.app.size = human_size(selected->size);
    model.title = single_line(selected->name) + "  " + single_line(selected->version);
    model.version = single_line(selected->version.empty() ? "-" : selected->version);
    model.author = single_line(selected->author.empty() ? "-" : selected->author);
    model.state = selected->installed ? "Installed" : "Not installed";
    if (selected->installed && !selected->installed_version.empty())
        model.state += " " + one_line(selected->installed_version, 10);
    model.updated = friendly_updated_at(selected->updated_at);
    model.installable = can_install_app(*selected);
    model.job_running = package_job_.running;
    model.job_progress = package_job_.progress;
    model.status = session_.status.value();
    model.show_status = !package_job_.running &&
        session_.status.visible(now, status_visible_ms);
    model.description = selected->description.empty() ? "-" : selected->description;
    for (const auto &category : selected->categories) {
        if (!model.categories.empty()) model.categories += "; ";
        model.categories += single_line(category);
    }
    if (model.categories.empty()) model.categories = selected->category.empty() ? "-" : selected->category;
    return model;
}

ConfirmationViewModel AppStoreViewModelFactory::confirmation() const
{
    return {session_.confirmation.lines(), session_.confirmation.focus()};
}

PackageProgressViewModel AppStoreViewModelFactory::progress(uint32_t now) const
{
    PackageProgressViewModel model;
    model.action = job_action_label(package_job_.action);
    model.title = package_job_.title.empty() ? "Selected app" : package_job_.title;
    model.detail = package_job_.pending_start ? "Preparing package worker" :
        (package_job_.detail.empty() ? "Waiting for package output" : package_job_.detail);
    model.progress = package_job_.progress;
    if (package_job_.progress >= 0)
        model.detail += " " + std::to_string(package_job_.progress) + "%";
    model.elapsed_seconds = package_job_.start_tick
        ? static_cast<uint32_t>(now - package_job_.start_tick) / 1000 : 0;
    return model;
}

ErrorDialogViewModel AppStoreViewModelFactory::error() const
{
    ErrorDialogViewModel model;
    model.title = session_.error.title.empty() ? "OPERATION FAILED" : session_.error.title;
    model.message = session_.error.message.empty() ? "Package operation failed." : session_.error.message;
    const std::string detail = session_.error.detail.empty()
        ? "Please try again after checking the package state." : session_.error.detail;
    model.detail_lines = wrap_display_text(detail, 44);
    model.repairable = session_.error.repairable;
    return model;
}

InitializationProgressViewModel AppStoreViewModelFactory::initialization() const
{
    InitializationProgressViewModel model;
    const SyncStatus &status = session_.sync.status;
    model.title = startup_title(status);
    model.source = registry_host(status.url.empty() ? session_.registry.registry_url() : status.url);
    model.percent = status.percent;
    model.failed = session_.sync.startup_failed;
    model.failure_focus = session_.sync.failure_focus;
    model.error_title = session_.error.title.empty() ? "Cannot Connect to Store" : session_.error.title;
    // The reason, then what to do about it. The non-breaking hyphen keeps Wi-Fi on one line.
    model.error_message =
        sentence(session_.error.message.empty() ? "No registry could be reached" : session_.error.message) +
        " " + sentence(session_.error.detail.empty()
                           ? "Check Wi\u2011Fi or change registries in Settings" : session_.error.detail);
    return model;
}

StoreSettingsViewModel AppStoreViewModelFactory::settings(bool operation_running) const
{
    StoreSettingsViewModel model;
    model.region_code = session_.registry.region_code();
    model.active_region = session_.registry.active_region();
    model.registry_url = session_.registry.registry_url();
    model.focus = session_.registry.page_focus();
    model.loading = session_.registry_loading;
    model.selected_index = session_.registry.selected_index();
    model.entry_count = static_cast<int>(session_.registry.entries().size());
    model.has_entry = session_.registry.selected_entry() != nullptr;
    if (model.has_entry) model.entry = *session_.registry.selected_entry();
    model.operation_running = operation_running;
    model.region_commit_pending = session_.registry.region_commit_pending();
    if (model.region_commit_pending) {
        model.status = "Region update pending...";
    } else if (model.operation_running) {
        model.status = "Registry operation running...";
    } else if (model.has_entry && !model.entry.error.empty()) {
        model.status = single_line(model.entry.error);
        model.status_is_error = true;
    }
    return model;
}

RegistryEditorViewModel AppStoreViewModelFactory::registry_editor() const
{
    RegistryEditorViewModel model{!session_.registry.edit_url().empty(),
                                  session_.registry.name_input(), session_.registry.input_url(),
                                  session_.registry.edit_focus(), session_.registry.caret(),
                                  session_.registry.editing_field(), {}, false};
    model.status = session_.status.value();
    model.status_is_error = !model.status.empty();
    return model;
}

TextEntryViewModel AppStoreViewModelFactory::share_code() const
{
    return {"Share Code", "TYPE SHARE CODE", "share code", share_code_.input(),
            share_code_.message(), 0x58A6FF};
}

SearchPageViewModel AppStoreViewModelFactory::search()
{
    SearchPageViewModel model;
    model.input = session_.search.input();
    model.message = session_.search.message();
    if (!session_.search.results_active() || session_.search.results().empty()) return model;
    model.results_active = true;
    model.result_count = static_cast<int>(session_.search.results().size());
    const int total = model.result_count;
    session_.search.selected_index() = std::max(
        0, std::min(total - 1, session_.search.selected_index()));
    int first = std::max(0, session_.search.selected_index() - 1);
    int last = std::min(total - 1, first + 2);
    first = std::max(0, last - 2);
    for (int index = first; index <= last; ++index) {
        const StoreApp &app = session_.catalog.apps()[session_.search.results()[index]];
        model.rows.push_back({app.name, app.category.empty() ? app.author : app.category,
                              app.version, index == session_.search.selected_index()});
    }
    return model;
}

} // namespace appstore_ui
