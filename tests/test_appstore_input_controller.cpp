/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "appstore_input_controller.hpp"
#include "appstore_task_service.hpp"
#include "detached_worker_launcher.hpp"
#include "input_keys.h"
#include "keyboard_input.h"

#include <cassert>
#include <iostream>
#include <vector>

namespace appstore {

std::string one_line(std::string value, size_t max_len)
{
    if (value.size() > max_len) value.resize(max_len);
    return value;
}

std::string match_key(std::string value)
{
    for (char &ch : value)
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
    return value;
}

std::vector<std::string> wrap_display_text(std::string text, int)
{
    return {std::move(text)};
}

bool has_blocking_missing(const std::string &missing) { return !missing.empty(); }
std::string missing_install_message(const std::string &) { return "Missing dependencies"; }
bool can_install_app(const StoreApp &app) { return app.installable; }
bool can_reinstall_app(const StoreApp &app) { return app.installable && app.installed; }
bool can_upgrade_app(const StoreApp &app)
{
    return app.installable && app.installed && app.version != app.installed_version;
}
std::string backend_error_message(const std::string &output) { return output; }
std::string sync_status_message(const std::string &) { return "Catalog synced"; }
void sort_apps(std::vector<StoreApp> &, SortRule) {}

} // namespace appstore

std::vector<std::string> detail_screenshot_paths(const std::string &,
                                                  const appstore::StoreApp &app)
{
    return app.images.empty() ? std::vector<std::string>{} : std::vector<std::string>{"one.png", "two.png"};
}

int main()
{
    assert(appstore_ui::appstore_input_context(appstore_ui::Screen::Home) ==
           KBD_INPUT_CONTEXT_NAVIGATION);
    assert(appstore_ui::appstore_input_context(appstore_ui::Screen::Search) ==
           KBD_INPUT_CONTEXT_TEXT);
    assert(appstore_ui::appstore_input_context(appstore_ui::Screen::RegistryEdit) ==
           KBD_INPUT_CONTEXT_TEXT);
    assert(appstore_ui::appstore_input_context(appstore_ui::Screen::Detail) ==
           KBD_INPUT_CONTEXT_NAVIGATION);
    using namespace appstore_ui;

    // Moving keys tick on every repeat; other keys sound once per press.
    constexpr auto nav = KBD_INPUT_CONTEXT_NAVIGATION;
    constexpr auto text = KBD_INPUT_CONTEXT_TEXT;
    assert(key_sound({KEY_DOWN}, nav) == KeySound::Move);
    assert(key_sound({KEY_X, 0, 'x', false, true}, nav) == KeySound::Move);
    assert(key_sound({KEY_X, 0, 'x'}, text) == KeySound::None);
    assert(key_sound({KEY_UP}, text) == KeySound::Move);
    assert(key_sound({KEY_ENTER}, text) == KeySound::Select);
    assert(key_sound({KEY_8, 0, '8'}, nav) == KeySound::Select);
    assert(key_sound({KEY_8, 0, '8', false, true}, nav) == KeySound::None);
    assert(key_sound({KEY_LEFTSHIFT}, nav) == KeySound::None);
    assert(key_sound({KEY_DOWN, 0, 0, true}, nav) == KeySound::None);

    AppStoreSessionState session;
    PackageJobState package_job;
    ExitController exit;
    ShareCodeState share_code;
    AppStoreTaskCoordinator coordinator;
    appstore::DetachedWorkerLauncher workers({}, {});
    AppStoreTaskService tasks(coordinator, workers,
        [](const std::vector<std::string> &, int *rc) { *rc = 0; return std::string{}; });
    CatalogController catalog(session, share_code, []() {});
    DetailActionController detail(session, package_job, [](const std::string &, const std::string &) {},
                                  []() { return false; });
    RegistryController registry(
        session, tasks, package_job,
        {[](const std::string &) { return appstore::RegistryData{}; },
         []() { return false; }, []() {}, []() {}, [](bool) {}, []() {}});
    SyncController sync(
        session, tasks,
        {[]() { return appstore::SyncStatus{}; }, []() { return true; },
         []() { return false; }, [](SummaryPurpose) {}, []() {}, []() {},
         []() { return true; }});

    int renders = 0;
    int quits = 0;
    int backs = 0;
    int repairs = 0;
    int registry_opens = 0;
    int registry_adds = 0;
    int confirmations = 0;
    int page_moves = 0;
    bool page_drawn = true;
    std::vector<KeySound> sounds;
    AppStoreInputController input(
        session, package_job, exit, share_code, catalog, detail, registry, sync,
        {[&]() { ++quits; },
         [&]() {
             ++backs;
             session.screen = session.sync.startup_active ? Screen::StartupSync : Screen::Home;
         },
         [&]() { ++registry_opens; session.screen = Screen::Registry; },
         [&]() { ++registry_adds; session.screen = Screen::RegistryEdit; },
         [&]() { ++confirmations; }, [&]() { ++repairs; }, [&]() { ++renders; },
         [&]() { ++page_moves; return page_drawn; },
         [&](KeySound sound) { sounds.push_back(sound); }});

    session.catalog.categories() = {"All"};
    appstore::StoreApp first;
    first.id = "one";
    first.name = "One";
    appstore::StoreApp second;
    second.id = "two";
    second.name = "Two";
    session.catalog.apps() = {first, second};
    session.catalog.visible() = {0, 1};
    session.screen = Screen::Home;
    input.handle({KEY_DOWN}, 100);
    assert(session.catalog.selected_index() == 1 && renders == 1);
    assert(sounds == std::vector<KeySound>{KeySound::Move});

    input.handle({KEY_5, 0, '5'}, 110);
    assert(session.screen == Screen::ShareCode && sounds.back() == KeySound::Select);
    input.handle({0, 0, 'A'}, 120);
    assert(share_code.input() == "A" && sounds.size() == 2);
    input.handle({KEY_BACKSPACE}, 130);
    assert(share_code.input().empty());

    session.screen = Screen::Home;
    input.handle({KEY_6}, 140);
    assert(session.screen == Screen::Search);

    session.catalog.apps()[0].name = "Firefox";
    session.catalog.apps()[1].name = "Firefly";
    session.search.input() = "fire";
    session.search.submit(session.catalog.apps());
    assert(session.search.results_active());
    input.handle({KEY_F, 0, 'f'}, 141);
    assert(session.search.input() == "firef");
    assert(!session.search.results_active());
    session.search.submit(session.catalog.apps());
    input.handle({KEY_X, 0, 'x'}, 142);
    assert(session.search.input() == "firefx");
    assert(!session.search.results_active());
    session.search.input() = "fire";
    session.search.submit(session.catalog.apps());
    assert(session.search.results_active() && session.search.selected_index() == 0);
    input.handle({KEY_DOWN}, 143);
    assert(session.search.selected_index() == 1);
    assert(session.search.input() == "fire");
    input.handle({KEY_UP}, 144);
    assert(session.search.selected_index() == 0);

    session.screen = Screen::Confirm;
    session.confirmation.focus() = 0;
    input.handle({KEY_TAB}, 150);
    assert(session.confirmation.focus() == 1);
    input.handle({KEY_7}, 151);
    assert(session.screen == Screen::Confirm);
    input.handle({KEY_ENTER}, 152);
    assert(session.screen == Screen::Detail);
    session.screen = Screen::Confirm;
    session.confirmation.focus() = 0;
    input.handle({KEY_5}, 153);
    assert(confirmations == 0);
    input.handle({KEY_ENTER}, 154);
    assert(confirmations == 1);

    session.screen = Screen::RegistryEdit;
    session.registry.edit_focus() = 0;
    session.registry.name_input().clear();
    input.handle({0, 0, 'X'}, 160);
    assert(session.registry.name_input() == "X");
    input.handle({KEY_TAB}, 161);
    assert(session.registry.edit_focus() == 1);
    const std::string url_before_x = session.registry.input_url();
    input.handle({KEY_X, 0, 'x'}, 162);
    assert(session.registry.edit_focus() == 1);
    assert(session.registry.input_url() == url_before_x + "x");
    // Fn+Z/C moves the cursor; typing and Backspace happen at the cursor.
    input.handle({KEY_Z, KBD_MOD_FN, 'z'}, 163);
    input.handle({KEY_Y, 0, 'y'}, 164);
    assert(session.registry.input_url() == url_before_x + "yx");
    input.handle({KEY_C, KBD_MOD_FN, 'c'}, 165);
    input.handle({KEY_BACKSPACE}, 166);
    assert(session.registry.input_url() == url_before_x + "y");
    // While a field is open, 8 types. Enter steps Name, then URL, then confirms.
    input.handle({KEY_8, 0, '8'}, 167);
    assert(session.registry.input_url() == url_before_x + "y8");
    assert(session.registry.editing_field());
    session.registry.edit_focus() = 0;
    input.handle({KEY_ENTER}, 168);
    assert(session.registry.editing_field() && session.registry.edit_focus() == 1);
    input.handle({KEY_ENTER}, 168);
    assert(!session.registry.editing_field());
    input.handle({KEY_ENTER}, 168);
    assert(session.registry.editing_field() && session.registry.edit_focus() == 0);
    session.registry.name_input().clear();
    input.handle({KEY_ENTER}, 168);
    input.handle({KEY_ENTER}, 168);
    input.handle({KEY_8, 0, '8'}, 169);
    assert(session.screen == Screen::RegistryEdit);
    assert(session.status.value() == "Name required");
    assert(session.registry.input_url() == url_before_x + "y8");

    appstore::RegistryData registry_data;
    registry_data.entries.push_back(
        {"https://example.invalid/registry.json", "Example", "ok", "1",
         "now", "", "global", true, false});
    session.registry.apply(registry_data);
    session.screen = Screen::Registry;
    input.handle({KEY_5}, 170);
    assert(session.screen == Screen::RegistryEdit);
    assert(session.registry.name_input() == "Example");
    session.screen = Screen::Registry;
    input.handle({KEY_4}, 180);
    assert(session.screen == Screen::RegistryEdit && registry_adds == 1);
    session.screen = Screen::Registry;
    input.handle({KEY_A, 0, 'a'}, 181);
    assert(session.screen == Screen::RegistryEdit && registry_adds == 2);
    session.screen = Screen::Registry;
    session.status.value().clear();
    input.handle({KEY_6}, 182);
    input.handle({KEY_T, 0, 't'}, 183);
    assert(session.screen == Screen::Registry && session.status.value().empty());

    appstore::RegistryData builtin_registry_data;
    builtin_registry_data.entries.push_back(
        {"https://example.invalid/builtin.json", "Built in", "ok", "1",
         "now", "", "global", true, true});
    session.registry.apply(builtin_registry_data);
    session.status.value().clear();
    session.screen = Screen::Registry;
    input.handle({KEY_5}, 184);
    assert(session.screen == Screen::Registry && session.status.value().empty());
    input.handle({KEY_7}, 185);
    assert(session.screen == Screen::Registry && session.status.value().empty());

    session.screen = Screen::Detail;
    const std::string detail_id = session.catalog.selected_app()->id;
    session.detail_media.normalize_page(detail_id, 400, 129);
    const int renders_before_scroll = renders;
    input.handle({KEY_X, 0, 'x'}, 186);
    assert(session.detail_media.page_scroll() == 24);
    assert(session.detail_media.page_scroll_at(186) == 0);
    // Scrolling moves the drawn page instead of rebuilding it.
    assert(page_moves == 1 && renders == renders_before_scroll);
    input.handle({KEY_F, 0, 'f', false, true}, 187);
    assert(session.detail_media.page_scroll() == 0 && sounds.back() == KeySound::Move);
    // Clearing a status toast takes a full render.
    session.status.value() = "Installed";
    input.handle({KEY_X, 0, 'x'}, 188);
    assert(session.status.value().empty() && page_moves == 2);
    assert(renders == renders_before_scroll + 1);
    // So does a page that is not drawn yet.
    page_drawn = false;
    input.handle({KEY_F, 0, 'f'}, 189);
    assert(page_moves == 3 && renders == renders_before_scroll + 2);
    page_drawn = true;
    session.catalog.selected_app()->images = "icon,one,two";
    input.handle({KEY_C, 0, 'c'}, 188);
    assert(session.detail_media.image_index() == 1 && session.screen == Screen::Detail);
    input.handle({KEY_Z, 0, 'z'}, 189);
    assert(session.detail_media.image_index() == 0);
    input.handle({KEY_5}, 190);
    assert(session.screen == Screen::Screenshots);
    session.screen = Screen::Detail;
    const size_t sounds_before_back = sounds.size();
    input.handle({KEY_ESC}, 200);
    input.handle({KEY_ESC, 0, 0, true}, 300);
    assert(backs == 1 && session.screen == Screen::Home);
    assert(sounds.size() == sounds_before_back + 1 && sounds.back() == KeySound::Select);

    session.screen = Screen::StartupSync;
    session.sync.startup_failed = false;
    input.handle({KEY_TAB}, 390);
    assert(registry_opens == 1 && session.screen == Screen::Registry);

    session.screen = Screen::StartupSync;
    session.sync.startup_failed = true;
    input.handle({KEY_ENTER}, 400);
    assert(quits == 1);
    session.screen = Screen::StartupSync;
    input.handle({KEY_RIGHT}, 410);
    assert(session.sync.failure_focus == 1);
    input.handle({KEY_ENTER}, 420);
    assert(registry_opens == 2 && session.screen == Screen::Registry);
    session.screen = Screen::StartupSync;
    input.handle({KEY_TAB}, 425);
    assert(registry_opens == 3 && session.screen == Screen::Registry);
    session.sync.startup_active = true;
    input.handle({KEY_B}, 430);
    assert(session.screen == Screen::StartupSync && session.sync.startup_failed);
    session.sync.startup_active = false;

    session.screen = Screen::ErrorDialog;
    session.error.repairable = true;
    input.handle({KEY_ENTER}, 500);
    assert(repairs == 1);
    input.handle({KEY_B}, 510);
    assert(backs == 3);

    // Root Esc opens a modal; dismissing it must not trigger the key's action.
    session.screen = Screen::Home;
    input.handle({KEY_ESC}, 1000);
    input.handle({KEY_ESC, 0, 0, true}, 1100);
    assert(session.exit_hint_visible && quits == 1 && backs == 3);
    input.handle({KEY_ENTER}, 1200);
    assert(!session.exit_hint_visible && session.screen == Screen::Home);
    input.handle({KEY_ENTER, 0, 0, false, true}, 1250);
    assert(session.screen == Screen::Home);
    input.handle({KEY_ENTER, 0, 0, true}, 1300);
    input.handle({KEY_ENTER}, 1400);
    assert(session.screen == Screen::Detail);

    session.screen = Screen::Home;
    input.handle({KEY_ESC}, 2000);
    input.handle({KEY_ESC, 0, 0, true}, 2100);
    assert(session.exit_hint_visible);
    input.handle({KEY_ESC}, 2200);
    input.handle({KEY_ESC, 0, 0, true}, 2300);
    assert(!session.exit_hint_visible && backs == 3);

    // Startup also offers the hint, while retaining the current sync screen.
    session.screen = Screen::StartupSync;
    input.handle({KEY_ESC}, 3000);
    input.handle({KEY_ESC, 0, 0, true}, 3100);
    assert(session.exit_hint_visible && session.screen == Screen::StartupSync);
    input.handle({KEY_TAB}, 3200);
    assert(!session.exit_hint_visible && session.screen == Screen::StartupSync);

    // Holding Esc exits at 3s, never at the previous 1.2s threshold.
    input.handle({KEY_ESC}, 4000);
    input.handle({KEY_ESC, 0, 0, false, true}, 5200);
    assert(quits == 1);
    input.handle({KEY_ESC, 0, 0, false, true}, 6999);
    assert(quits == 1);
    input.handle({KEY_ESC, 0, 0, false, true}, 7000);
    assert(quits == 2);
    input.handle({KEY_ESC, 0, 0, true}, 7100);
    assert(quits == 2 && !session.exit_hint_visible);

    // No-repeat keyboards must still exit when release reaches 3s.
    input.handle({KEY_ESC}, 8000);
    input.handle({KEY_ESC, 0, 0, true}, 11000);
    assert(quits == 3 && !session.exit_hint_visible);

    std::cout << "appstore input controller tests passed\n";
}
