/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "keyboard_input.h"

#include "appstore_session_state.hpp"
#include "catalog_controller.hpp"
#include "detail_action_controller.hpp"
#include "exit_controller.hpp"
#include "package_job_state.hpp"
#include "registry_controller.hpp"
#include "share_code_state.hpp"
#include "sync_controller.hpp"

#include <cstdint>
#include <functional>

namespace appstore_ui {

constexpr cp0_keyboard_input_context_t appstore_input_context(Screen screen)
{
    return screen == Screen::Search || screen == Screen::ShareCode ||
                   screen == Screen::RegistryEdit
               ? KBD_INPUT_CONTEXT_TEXT
               : KBD_INPUT_CONTEXT_NAVIGATION;
}

struct AppStoreKeyEvent {
    uint32_t code = 0;
    uint32_t mods = 0;
    char ch = 0;
    bool release = false;
    bool repeated = false;
};

// The launcher's key sounds: Move when a key moves through the screen, Select
// when it acts.
enum class KeySound { None, Move, Select };

KeySound key_sound(const AppStoreKeyEvent &key, cp0_keyboard_input_context_t context);

class AppStoreInputController
{
public:
    struct Actions {
        std::function<void()> request_quit;
        std::function<void()> navigate_back;
        std::function<void()> open_registry;
        std::function<void()> open_registry_add;
        std::function<void()> execute_confirmation;
        std::function<void()> repair_package;
        std::function<void()> render;
        // Moves the drawn detail page to its scroll target; false if it needs a render.
        std::function<bool()> scroll_detail;
        std::function<void(KeySound)> play_sound;
    };

    AppStoreInputController(AppStoreSessionState &session, PackageJobState &package_job,
                            ExitController &exit, ShareCodeState &share_code,
                            CatalogController &catalog, DetailActionController &detail,
                            RegistryController &registry, SyncController &sync,
                            Actions actions);

    void handle(const AppStoreKeyEvent &key, uint32_t now);
    // The click for a key, without acting on it. Used when a repeat arrives
    // before the previous one has been drawn.
    void play_feedback(const AppStoreKeyEvent &key);

private:
    static bool matches(const AppStoreKeyEvent &key, char ch, uint32_t code);
    void play(KeySound sound);

    AppStoreSessionState &session_;
    PackageJobState &package_job_;
    ExitController &exit_;
    ShareCodeState &share_code_;
    CatalogController &catalog_;
    DetailActionController &detail_;
    RegistryController &registry_;
    SyncController &sync_;
    Actions actions_;
    uint32_t dismiss_key_ = 0;
};

} // namespace appstore_ui
