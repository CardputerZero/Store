/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "horizontal_marquee.hpp"
#include <cassert>
int main() {
 appstore_ui::HorizontalMarquee m;
 m.select("Long title",100);
 assert(m.offset(2099,300)==0);
 assert(m.offset(3100,300)==-20);
 m.select("Long title",5100); // A status refresh must not restart reading.
 assert(m.offset(5100,300)==-60);
 assert(m.offset(5100,0)==0);
 assert(m.offset(17100,300)==-300);
 assert(m.offset(18000,300)==-300);
 assert(m.offset(18600,300)==0);
 m.select("Other app",19000);
 assert(m.offset(20000,300)==0);
 m.select("Wrap",0xfffffff0U);
 assert(m.offset(0x20U,300)==0);
}
