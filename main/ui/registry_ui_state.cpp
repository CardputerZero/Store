/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "registry_ui_state.hpp"

#include <algorithm>
#include <vector>

namespace appstore_ui {

void RegistryUiState::apply(const appstore::RegistryData &data)
{
    if (region_commit_pending_) return;
    apply_region(data.region);
    error_.clear();
    for (const auto &entry : data.entries) {
        if (entry.builtin && entry.url == registry_url_) {
            error_ = entry.error;
            break;
        }
    }
}

void RegistryUiState::apply_config_hint(const appstore::RegistryConfig &config)
{
    if (region_commit_pending_) return;
    const auto canonical = appstore::SharedRegistryStore::normalize(config);
    region_code_ = canonical.region;
    region_label_ = region_code_ == "CN" ? "China" :
                    region_code_ == "default" ? "Default" : "Auto";
    active_region_ = canonical.active_region;
    if (registry_url_ != canonical.entries.front().url) error_.clear();
    registry_url_ = canonical.entries.front().url;
}

void RegistryUiState::apply_region(const appstore::RegionData &region)
{
    appstore::RegistryConfig config;
    config.region = region.code;
    config.active_region = region.active;
    apply_config_hint(config);
}

void RegistryUiState::apply_local_region(const std::string &region)
{
    appstore::RegistryConfig config;
    config.region = region;
    // Auto always starts with GitHub, regardless of the previously active mirror.
    config.active_region = region == "CN" ? "CN" : "default";
    region_commit_pending_ = false;
    apply_config_hint(config);
}

std::string RegistryUiState::adjacent_region(int delta) const
{
    static const std::vector<std::string> regions = {"auto", "default", "CN"};
    auto it = std::find(regions.begin(), regions.end(), region_code_);
    int index = it == regions.end() ? 0 : static_cast<int>(it - regions.begin());
    int next = (index + delta) % static_cast<int>(regions.size());
    if (next < 0) next += static_cast<int>(regions.size());
    return regions[next];
}

bool RegistryUiState::schedule_region(const std::string &region, uint32_t now)
{
    if (region == region_code_) return false;
    apply_local_region(region);
    pending_region_ = region_code_;
    region_commit_pending_ = true;
    region_change_tick_ = now;
    return true;
}

std::optional<std::string> RegistryUiState::take_due_region(uint32_t now, uint32_t delay_ms,
                                                            bool operation_running)
{
    if (!region_commit_pending_ || operation_running ||
        static_cast<uint32_t>(now - region_change_tick_) < delay_ms) return std::nullopt;
    region_commit_pending_ = false;
    return pending_region_;
}

} // namespace appstore_ui
