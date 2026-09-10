/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

namespace appstore {

// Handles private backend process modes. Returns false for a normal UI launch.
bool run_backend_process_mode(int argc, char **argv, int *exit_code);

} // namespace appstore
