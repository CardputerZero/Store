/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "registry_controller.hpp"

#include <sstream>
#include <utility>

namespace appstore_ui {

using namespace appstore;

RegistryController::RegistryController(AppStoreSessionState &session,
                                       AppStoreTaskService &tasks,
                                       PackageJobState &package_job,
                                       Dependencies dependencies)
    : session_(session), tasks_(tasks), package_job_(package_job),
      dependencies_(std::move(dependencies))
{
}

void RegistryController::open()
{
    session_.screen = Screen::Registry;
    session_.status.value() = "Loading registries...";
    request_refresh();
}

void RegistryController::request_refresh()
{
    if (dependencies_.exit_requested && dependencies_.exit_requested()) return;
    const TaskStartResult started = tasks_.request_registry_refresh(
        session_.registry.registry_url(), dependencies_.load_registries);
    session_.registry_loading = true;
    if (started == TaskStartResult::Failed) {
        session_.registry_loading = false;
        session_.status.value() = "Unable to refresh registries";
    }
}

bool RegistryController::poll_refresh()
{
    RegistryRefreshRequest request;
    RegistryData registries;
    std::optional<RegistryRefreshRequest> deferred;
    if (!tasks_.take_registry_refresh(request, registries, &deferred)) return false;
    session_.registry_loading = false;
    if (deferred) {
        request_refresh();
        return true;
    }
    session_.registry.apply(registries);
    if (dependencies_.save_config) dependencies_.save_config();
    return true;
}

bool RegistryController::refresh_running()
{
    return tasks_.registry_refresh_running() || session_.registry_loading;
}

bool RegistryController::operation_available()
{
    if (tasks_.registry_operation_running()) {
        session_.status.value() = "Registry operation running";
        return false;
    }
    if (refresh_running()) {
        session_.status.value() = "Loading registries...";
        return false;
    }
    if (tasks_.sync_running()) {
        session_.status.value() = "Syncing catalog...";
        return false;
    }
    return true;
}

void RegistryController::cancel_online_work()
{
    tasks_.cancel_catalog_work();
    session_.registry_loading = false;
    session_.sync.animation_phase = -1;
    const auto active = tasks_.active_registry_operation();
    if (active && active->kind == RegistryOpKind::SetRegion)
        tasks_.cancel_registry_operation();
    if (!package_job_.running && !package_job_.pending_start && dependencies_.stop_backend)
        dependencies_.stop_backend();
}

bool RegistryController::start_operation(const RegistryOpRequest &request,
                                         const std::string &status)
{
    if (dependencies_.exit_requested && dependencies_.exit_requested()) return false;
    const TaskStartResult started = tasks_.start_registry_operation(request);
    if (started == TaskStartResult::Busy) {
        session_.status.value() = "Registry operation running";
        return false;
    }
    session_.status.value() = status;
    if (started == TaskStartResult::Failed) {
        session_.status.value() = "Unable to start registry operation";
        return false;
    }
    return true;
}

void RegistryController::apply_region_output(const std::string &output)
{
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        const auto fields = split_tab(line);
        if (fields.size() < 4 || fields[0] != "REGION") continue;
        RegionData region;
        region.code = fields[1];
        region.label = fields[2];
        region.registry_url = fields[3];
        if (fields.size() >= 5) region.active = fields[4];
        session_.registry.apply_region(region);
        return;
    }
}

bool RegistryController::poll_operation()
{
    RegistryOpRequest request;
    RegistryOpResult result;
    if (!tasks_.take_registry_operation(request, result)) return false;
    const bool ok = result.rc == 0 && result.output.find("ERROR") == std::string::npos;
    if (!ok) {
        session_.status.value() = one_line(backend_error_message(result.output), 54);
        request_refresh();
        return true;
    }
    apply_region_output(result.output);
    if (dependencies_.save_config) dependencies_.save_config();
    session_.status.value() = "Region: " + session_.registry.region_label();
    if (!session_.registry.region_commit_pending()) {
        if (dependencies_.request_summary) dependencies_.request_summary();
        if (dependencies_.sync_catalog) dependencies_.sync_catalog(true);
    }
    return true;
}

void RegistryController::select_region(const std::string &region, uint32_t now)
{
    if (region == session_.registry.region_code()) return;
    cancel_online_work();
    session_.registry.schedule_region(region, now);
    session_.status.value() = "Region: " + session_.registry.region_label() + " (waiting...)";
}

std::string RegistryController::adjacent_region(int delta) const
{
    return session_.registry.adjacent_region(delta);
}

bool RegistryController::poll_region_debounce(uint32_t now, uint32_t delay_ms)
{
    if (!session_.registry.region_commit_pending()) return false;
    auto region = session_.registry.take_due_region(
        now, delay_ms, tasks_.registry_operation_running());
    if (!region) return false;
    RegistryOpRequest request;
    request.kind = RegistryOpKind::SetRegion;
    request.region = *region;
    start_operation(request, "Changing region...");
    return true;
}

} // namespace appstore_ui
