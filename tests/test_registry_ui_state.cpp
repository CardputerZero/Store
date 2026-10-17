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
    appstore_ui::RegistryUiState state;
    assert(state.region_code() == "auto");
    assert(state.registry_url() == appstore_ui::RegistryUiState::kDefaultRegistryUrl);

    appstore::RegistryData data;
    data.region = {"default", "Default", appstore_ui::RegistryUiState::kDefaultRegistryUrl,
                   "default"};
    data.entries.push_back({appstore_ui::RegistryUiState::kDefaultRegistryUrl,
                            "CardputerZero Hub", "", "", "", "", "default", true, true});
    data.entries.push_back({"https://example.test/registry.json", "Example", "", "", "", "",
                            "", true, false});
    state.apply(data);
    assert(state.selected_entry()->builtin);
    state.select_adjacent_entry(-1);
    assert(state.selected_index() == 1);
    assert(state.begin_edit_selected());
    assert(state.name_input() == "Example");
    state.select_adjacent_entry(1);
    assert(!state.begin_edit_selected());

    assert(state.schedule_region("CN", 100));
    assert(state.region_label() == "China");
    assert(state.registry_url().find("oss-cn-shenzhen") != std::string::npos);
    assert(!state.take_due_region(2099, 2000, false));
    assert(!state.take_due_region(2100, 2000, true));
    auto due = state.take_due_region(2100, 2000, false);
    assert(due && *due == "CN");
    assert(!state.region_commit_pending());

    state.begin_add();
    assert(state.edit_url().empty());
    assert(state.name_input().empty());
    assert(state.input_url() == appstore_ui::RegistryUiState::kDefaultRegistryUrl);
    assert(state.caret() == 0);

    // The caret edits in the middle of a field, not only at its end.
    state.name_input() = "ac";
    state.move_caret(1000);
    state.move_caret(-1);
    state.insert_at_caret('b', 48);
    assert(state.name_input() == "abc" && state.caret() == 2);
    state.erase_before_caret();
    assert(state.name_input() == "ac" && state.caret() == 1);
    state.input_url() = "héllo";
    state.edit_focus() = 1;
    state.move_caret(1000);
    // Three characters back steps over both bytes of é's neighbour, not into it.
    state.move_caret(-3);
    state.insert_at_caret('X', 180);
    assert(state.input_url() == "héXllo");

    std::cout << "registry UI state tests passed\n";
}
