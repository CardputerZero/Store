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
 assert(m.offset(3100,300)==-40);
 m.select("Long title",5100); // A status refresh must not restart reading.
 assert(m.offset(5100,300)==-120);
 assert(m.offset(5100,0)==0);
 assert(m.offset(9600,300)==-300); // Hold at the end...
 assert(m.offset(11599,300)==-300);
 assert(m.offset(15350,300)==-150); // ...then scroll back instead of jumping.
 assert(m.offset(19099,300)==-1);
 assert(m.offset(19100,300)==0);
 assert(m.offset(21100,300)==0);
 appstore_ui::HorizontalMarquee clamped;
 clamped.select("Short",0);
 assert(clamped.offset(2150,4)==-2); // A tiny overflow still takes 300 ms.
 assert(clamped.offset(2300,4)==-4);
 assert(clamped.offset(7000,1000)==-500); // A huge one is capped at 10 s.
 m.select("Other app",19000);
 assert(m.offset(20000,300)==0);
 m.select("Wrap",0xfffffff0U);
 assert(m.offset(0x20U,300)==0);
 assert(m.offset(2990U,300)==-40);
}
