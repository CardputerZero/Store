/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "appstore_client.hpp"
#include "shared_registry_store.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace appstore_ui {

class RegistryUiState
{
public:
    static constexpr const char *kDefaultRegistryUrl =
        appstore::SharedRegistryStore::kDefaultRegistryUrl;

    void apply(const appstore::RegistryData &data);
    void apply_config_hint(const appstore::RegistryConfig &config);
    void apply_region(const appstore::RegionData &region);
    void apply_local_region(const std::string &region);
    std::string adjacent_region(int delta) const;

    bool schedule_region(const std::string &region, uint32_t now);
    std::optional<std::string> take_due_region(uint32_t now, uint32_t delay_ms,
                                               bool operation_running);

    const std::string &region_code() const { return region_code_; }
    const std::string &region_label() const { return region_label_; }
    const std::string &registry_url() const { return registry_url_; }
    const std::string &active_region() const { return active_region_; }
    const std::string &error() const { return error_; }
    bool region_commit_pending() const { return region_commit_pending_; }

private:
    std::string region_code_ = "auto";
    std::string region_label_ = "Auto";
    std::string registry_url_ = kDefaultRegistryUrl;
    std::string active_region_ = "default";
    std::string error_;
    bool region_commit_pending_ = false;
    std::string pending_region_;
    uint32_t region_change_tick_ = 0;
};

} // namespace appstore_ui
