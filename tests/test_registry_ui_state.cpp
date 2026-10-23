/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "registry_ui_state.hpp"

#include <cassert>
#include <iostream>

int main()
{
    using appstore::SharedRegistryStore;
    appstore_ui::RegistryUiState state;
    assert(state.region_code() == "auto");
    assert(state.registry_url() == SharedRegistryStore::kDefaultRegistryUrl);

    appstore::RegistryConfig legacy;
    legacy.region = "auto";
    legacy.active_region = "CN";
    legacy.entries.push_back({"https://custom.test", "Custom", "", "", "", "", "", true, false});
    legacy.entries.push_back({SharedRegistryStore::kDefaultRegistryUrl, "Old builtin",
                              "", "", "", "", "default", true, true});
    state.apply_config_hint(legacy);
    assert(state.active_region() == "CN");
    assert(state.registry_url() == SharedRegistryStore::kChinaRegistryUrl);
    legacy.region = "invalid";
    legacy.active_region = "invalid";
    state.apply_config_hint(legacy);
    assert(state.region_code() == "auto" && state.active_region() == "default");
    assert(state.registry_url() == SharedRegistryStore::kDefaultRegistryUrl);

    assert(state.schedule_region("CN", 100));
    assert(state.region_label() == "China");
    assert(state.registry_url() == SharedRegistryStore::kChinaRegistryUrl);
    // A stale refresh must not replace the user's debounced selection.
    state.apply(appstore::RegistryData{});
    assert(state.region_code() == "CN");
    assert(!state.take_due_region(2099, 2000, false));
    assert(!state.take_due_region(2100, 2000, true));
    auto due = state.take_due_region(2100, 2000, false);
    assert(due && *due == "CN");
    assert(!state.region_commit_pending());
    assert(state.adjacent_region(1) == "auto");
    assert(state.schedule_region("auto", 2200));
    assert(state.active_region() == "default");
    assert(state.registry_url() == SharedRegistryStore::kDefaultRegistryUrl);
    assert(state.take_due_region(4200, 2000, false));

    appstore::RegistryData data;
    data.region = {"auto", "Auto", "https://untrusted.test", "CN"};
    data.entries.push_back({"https://custom.test", "Custom", "", "", "", "Custom error", "", true, false});
    data.entries.push_back({SharedRegistryStore::kChinaRegistryUrl, "Hub", "", "", "",
                            "Mirror unavailable", "CN", true, true});
    state.apply(data);
    assert(state.registry_url() == SharedRegistryStore::kChinaRegistryUrl);
    assert(state.error() == "Mirror unavailable");
    data.region = {"default", "Wrong label", "https://untrusted.test", "CN"};
    state.apply(data);
    assert(state.active_region() == "default" && state.region_label() == "Default");
    assert(state.registry_url() == SharedRegistryStore::kDefaultRegistryUrl);
    assert(state.error().empty());

    std::cout << "registry UI state tests passed\n";
}
