/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "../main/interface/shared_registry_store.hpp"

#include <cassert>
#include <map>

int main()
{
    using appstore::SharedRegistryStore;
    std::map<std::string, std::string> values;
    bool saved = false;
    SharedRegistryStore store(
        [&](const std::string &key, const std::string &fallback) {
            auto found = values.find(key);
            return found == values.end() ? fallback : found->second;
        },
        [&](const std::string &key, const std::string &value) { values[key] = value; },
        [&]() { saved = true; });

    appstore::RegistryConfig loaded;
    assert(!store.load(loaded));
    values["appstore.registry.count"] = "2";
    values["appstore.registry.region"] = "auto";
    values["appstore.registry.active_region"] = "CN";
    values["appstore.registry.0.entry"] = "Custom\thttps://custom.test\t1\t0\tCN";
    values["appstore.registry.1.entry"] = "Spoofed\thttps://spoof.test\t0\t1\tdefault";
    assert(store.load(loaded));
    assert(loaded.region == "auto" && loaded.active_region == "CN");
    assert(loaded.entries.size() == 1);
    assert(loaded.entries[0].name == "CardputerZero Hub CN");
    assert(loaded.entries[0].url == SharedRegistryStore::kChinaRegistryUrl);
    assert(loaded.entries[0].enabled && loaded.entries[0].builtin);
    assert(store.save(loaded) && saved);
    assert(values["appstore.registry.count"] == "1");
    assert(values["appstore.registry.1.entry"].empty());

    for (const auto &mode : {"auto", "default", "CN", "US", ""}) {
        for (const auto &active : {"default", "CN", "custom", ""}) {
            appstore::RegistryConfig config;
            config.region = mode;
            config.active_region = active;
            assert(store.save(config));
            assert(store.load(loaded));
            const std::string expected_mode = std::string(mode) == "CN" ||
                std::string(mode) == "default" ? mode : "auto";
            const std::string expected_active = expected_mode == "auto"
                ? (std::string(active) == "CN" ? "CN" : "default") : expected_mode;
            assert(loaded.region == expected_mode && loaded.active_region == expected_active);
            assert(loaded.entries.size() == 1 && loaded.entries[0].region == expected_active);
            assert(loaded.entries[0].url == (expected_active == "CN"
                ? SharedRegistryStore::kChinaRegistryUrl : SharedRegistryStore::kDefaultRegistryUrl));
        }
    }
    values["appstore.registry.count"] = "0";
    values["appstore.registry.region"] = "CN";
    values["appstore.registry.active_region"] = "default";
    assert(store.load(loaded) && loaded.active_region == "CN");
    return 0;
}
