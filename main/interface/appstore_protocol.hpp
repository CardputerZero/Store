/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <vector>

namespace appstore {

std::string tsv_unescape(const std::string &value);
std::vector<std::string> split_tab(const std::string &line);

} // namespace appstore
