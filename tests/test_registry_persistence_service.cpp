/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "registry_persistence_service.hpp"

#include <cassert>
#include <map>

namespace {

appstore::RegistryConfig config(const std::string &region, const std::string &url)
{
    appstore::RegistryConfig value;
    value.region = region;
    value.active_region = region;
    appstore::RegistryEntry entry;
    entry.name = region;
    entry.url = url;
    entry.region = region;
    entry.enabled = true;
    value.entries.push_back(entry);
    return value;
}

appstore::SharedRegistryStore store_for(std::map<std::string, std::string> &values,
                                        int &save_count)
{
    return {
        [&](const std::string &key, const std::string &fallback) {
            const auto found = values.find(key);
            return found == values.end() ? fallback : found->second;
        },
        [&](const std::string &key, const std::string &value) { values[key] = value; },
        [&]() { ++save_count; }};
}

} // namespace

int main()
{
    using namespace appstore_ui;
    std::map<std::string, std::string> values;
    int shared_saves = 0;
    auto shared_store = store_for(values, shared_saves);
    shared_store.save(config("CN", "https://cn.example/registry.json"));

    AppStoreSessionState session;
    int backend_loads = 0;
    int backend_replaces = 0;
    RegistryPersistenceService service(
        session, shared_store,
        {[&]() { ++backend_loads; return config("US", "https://us.example/registry.json"); },
         [&](const appstore::RegistryConfig &value) {
             ++backend_replaces;
             return value.region == "CN";
         },
         [](const std::string &) {}});
    service.initialize();
    assert(backend_loads == 0 && backend_replaces == 1);
    assert(session.registry.region_code() == "CN");
    assert(session.registry.registry_url() == appstore::SharedRegistryStore::kChinaRegistryUrl);
    assert(shared_saves == 2);

    values.clear();
    AppStoreSessionState migrated_session;
    RegistryPersistenceService migration(
        migrated_session, shared_store,
        {[&]() { ++backend_loads; return config("US", "https://us.example/registry.json"); },
         [](const appstore::RegistryConfig &) { return true; },
         [](const std::string &) {}});
    migration.initialize();
    assert(backend_loads == 1);
    assert(migrated_session.registry.region_code() == "auto");
    assert(migrated_session.registry.registry_url() ==
           appstore::SharedRegistryStore::kDefaultRegistryUrl);
    appstore::RegistryConfig migrated;
    assert(shared_store.load(migrated) && migrated.region == "auto");

    RegistryPersistenceService empty_backend(
        migrated_session, shared_store,
        {[]() { return appstore::RegistryConfig{}; },
         [](const appstore::RegistryConfig &) { return true; },
         [](const std::string &) {}});
    assert(!empty_backend.save_backend());
    appstore::RegistryConfig unchanged;
    assert(shared_store.load(unchanged) && unchanged.region == "auto");

    RegistryPersistenceService fallback(
        migrated_session, shared_store,
        {[]() {
             auto value = config("auto", "https://stale.test");
             value.active_region = "CN";
             return value;
         }, {}, {}});
    assert(fallback.save_backend());
    assert(migrated_session.registry.region_code() == "auto");
    assert(migrated_session.registry.active_region() == "CN");
    assert(migrated_session.registry.registry_url() ==
           appstore::SharedRegistryStore::kChinaRegistryUrl);
    assert(migrated_session.sync.status.url == migrated_session.registry.registry_url());
    assert(shared_store.load(unchanged) && unchanged.active_region == "CN");
    assert(unchanged.entries.size() == 1 && unchanged.entries[0].builtin);
}
