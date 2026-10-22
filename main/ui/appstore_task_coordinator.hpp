/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "appstore_client.hpp"
#include "async_job_slot.hpp"

#include <string>
#include <vector>

namespace appstore_ui {

enum class Screen {
    StartupSync,
    Home,
    Detail,
    Confirm,
    Progress,
    ErrorDialog,
    Registry,
    ShareCode,
    Search,
    Screenshots,
};

enum class RegistryOpKind {
    None,
    SetRegion,
};

struct RegistryOpRequest {
    RegistryOpKind kind = RegistryOpKind::None;
    std::string region;
};

struct RegistryOpResult {
    std::string output;
    int rc = -1;
};

struct RegistryRefreshRequest { std::string fallback; };
struct SyncRequest { bool refresh_registries_after = false; };
enum class SummaryPurpose { Regular, StartupCatalog };
struct SummaryRequest {
    appstore::SortRule rule = appstore::SortRule::Default;
    SummaryPurpose purpose = SummaryPurpose::Regular;
};

struct PlanRequest {
    std::string action;
    std::string app_id;
    Screen origin_screen = Screen::Detail;
};

struct PlanResult {
    std::string output;
    int rc = -1;
};

struct ScreenshotRequest { std::string app_id; };
struct ScreenshotResult { std::string output; int rc = -1; };

class AppStoreTaskCoordinator
{
public:
    static std::vector<std::string> registry_arguments(const RegistryOpRequest &request)
    {
        switch (request.kind) {
            case RegistryOpKind::SetRegion: return {"--set-region", request.region};
            case RegistryOpKind::None: return {};
        }
        return {};
    }

    appstore::AsyncJobSlot<SyncRequest, std::string> &sync() { return sync_; }
    appstore::AsyncJobSlot<SummaryRequest, appstore::SummaryData> &summary() { return summary_; }
    appstore::AsyncJobSlot<RegistryRefreshRequest, appstore::RegistryData> &registry_refresh()
    {
        return registry_refresh_;
    }
    appstore::AsyncJobSlot<RegistryOpRequest, RegistryOpResult> &registry_operation()
    {
        return registry_operation_;
    }
    appstore::AsyncJobSlot<PlanRequest, PlanResult> &plan() { return plan_; }
    appstore::AsyncJobSlot<ScreenshotRequest, ScreenshotResult> &screenshots()
    {
        return screenshots_;
    }

    void cancel_catalog_work()
    {
        registry_refresh_.cancel();
        sync_.cancel();
        summary_.cancel();
    }

private:
    appstore::AsyncJobSlot<SyncRequest, std::string> sync_;
    appstore::AsyncJobSlot<SummaryRequest, appstore::SummaryData> summary_;
    appstore::AsyncJobSlot<RegistryRefreshRequest, appstore::RegistryData> registry_refresh_;
    appstore::AsyncJobSlot<RegistryOpRequest, RegistryOpResult> registry_operation_;
    appstore::AsyncJobSlot<PlanRequest, PlanResult> plan_;
    appstore::AsyncJobSlot<ScreenshotRequest, ScreenshotResult> screenshots_;
};

} // namespace appstore_ui
