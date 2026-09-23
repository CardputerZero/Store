/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "appstore_fonts.hpp"
#include "cp0_font_service.hpp"

#include <cstdlib>

namespace {

// Keep these as APPLaunch resource names instead of filesystem paths.  The
// launcher font service resolves them against the active platform's font
// resource directory (device or SDL), so Store does not depend on an image's
// package layout.
constexpr const char *kLatinSansFont = "DejaVuSans.ttf";
constexpr const char *kCjkSansFont = "NotoSansCJK-Regular.ttc";
constexpr const char *kLegacyCjkSansFont = "AlibabaPuHuiTi-3-55-Regular.ttf";
constexpr const char *kLatinSerifFont = "DejaVuSerif.ttf";
constexpr const char *kCjkSerifFont = "NotoSerifCJK-Regular.ttc";

#if LV_USE_FREETYPE
const char *g_latin_sans_path = kLatinSansFont;
const char *g_cjk_sans_path = kCjkSansFont;
const char *g_latin_serif_path = kLatinSerifFont;
const char *g_cjk_serif_path = kCjkSerifFont;
std::string g_latin_sans_path_storage;
std::string g_cjk_sans_path_storage;
std::string g_latin_serif_path_storage;
std::string g_cjk_serif_path_storage;
lv_font_t *g_latin_fonts[4] = {};
lv_font_t *g_cjk_fonts[4] = {};
lv_font_t *g_latin_serif_fonts[4] = {};
lv_font_t *g_cjk_serif_fonts[4] = {};

int font_slot(const lv_font_t *font)
{
    if (font == &lv_font_montserrat_20) return 3;
    if (font == &lv_font_montserrat_14) return 2;
    if (font == &lv_font_montserrat_12) return 1;
    for (int slot = 0; slot < 4; ++slot) {
        if (font == g_latin_fonts[slot] || font == g_cjk_fonts[slot] ||
            font == g_latin_serif_fonts[slot] || font == g_cjk_serif_fonts[slot]) {
            return slot;
        }
    }
    return -1;
}

uint16_t font_size(int slot)
{
    static constexpr uint16_t sizes[] = {10, 12, 14, 20};
    return sizes[slot];
}

lv_font_t *latin_font_for_size(uint16_t size)
{
    const int slot = font_slot(cp0_fonts().fallback(size));
    return slot >= 0 ? g_latin_fonts[slot] : nullptr;
}

bool contains_cjk(const std::string &text)
{
    // CJK code points used by Store encode with a leading UTF-8 byte >= E3.
    // Latin, Greek, Cyrillic and the UI symbols used here stay below it.
    for (unsigned char ch : text) {
        if (ch >= 0xE3) return true;
    }
    return false;
}

lv_font_t *load_cjk_font(const char *font_name, uint16_t size,
                         lv_freetype_font_style_t style)
{
    lv_font_t *font = cp0_fonts().get(font_name, size, style);
    if (font == cp0_fonts().fallback(size) &&
        std::string(font_name) == kCjkSansFont) {
        font = cp0_fonts().get(kLegacyCjkSansFont, size, style);
    }
    return font;
}
#endif

}  // namespace

const lv_font_t *store_font(const std::string &text, uint16_t size, bool bold)
{
#if LV_USE_FREETYPE
    const bool cjk = contains_cjk(text);
    const char *override = std::getenv(cjk ? "M5APPSTORE_CJK_FONT" :
                                     (bold ? "M5APPSTORE_BOLD_FONT" : "M5APPSTORE_FONT"));

    // The Store reference UI is laid out against LVGL's built-in Montserrat
    // metrics.  Keep the default Latin path deterministic on both SDL and the
    // device; only use FreeType when an explicit override is requested.
    if (!cjk && !(override && override[0])) {
        return cp0_fonts().fallback(size);
    }

    const char *font_name = override && override[0]
        ? override
        : (cjk ? g_cjk_sans_path : g_latin_sans_path);
    const auto style = LV_FREETYPE_FONT_STYLE_NORMAL;

    // Keep Latin as the primary font for mixed names.  CJK fonts commonly
    // use different Latin glyph metrics, which makes the English part of a
    // name appear smaller even though both fonts were requested at the same
    // point size.  The CJK font is installed as the fallback during runtime
    // font initialization, so missing CJK glyphs still render correctly.
    if (cjk) {
        lv_font_t *latin = latin_font_for_size(size);
        if (latin && latin != cp0_fonts().fallback(size)) {
            if (override && override[0]) {
                lv_font_t *fallback = load_cjk_font(
                    font_name, size, LV_FREETYPE_FONT_STYLE_NORMAL);
                if (fallback != cp0_fonts().fallback(size)) latin->fallback = fallback;
            }
            return latin;
        }
    }

    lv_font_t *selected = cjk
        ? load_cjk_font(font_name, size, style)
        : cp0_fonts().get(font_name, size, style);
    if (!cjk && selected != cp0_fonts().fallback(size)) {
        lv_font_t *fallback = load_cjk_font(
            g_cjk_sans_path, size, LV_FREETYPE_FONT_STYLE_NORMAL);
        if (fallback != cp0_fonts().fallback(size)) selected->fallback = fallback;
    }
    return selected;
#else
    (void)bold;
    return font_for_text(text, cp0_fonts().fallback(size));
#endif
}

#if LV_USE_FREETYPE
void init_runtime_fonts(const std::string &app_dir)
{
    (void)app_dir;
    const char *latin_path = std::getenv("M5APPSTORE_LATIN_SANS_FONT");
    const char *cjk_path = std::getenv("M5APPSTORE_CJK_SANS_FONT");
    const char *latin_serif_path = std::getenv("M5APPSTORE_LATIN_SERIF_FONT");
    const char *cjk_serif_path = std::getenv("M5APPSTORE_CJK_SERIF_FONT");
    if (latin_path && latin_path[0]) {
        g_latin_sans_path_storage = latin_path;
        g_latin_sans_path = g_latin_sans_path_storage.c_str();
    }
    if (cjk_path && cjk_path[0]) {
        g_cjk_sans_path_storage = cjk_path;
        g_cjk_sans_path = g_cjk_sans_path_storage.c_str();
    }
    if (latin_serif_path && latin_serif_path[0]) {
        g_latin_serif_path_storage = latin_serif_path;
        g_latin_serif_path = g_latin_serif_path_storage.c_str();
    }
    if (cjk_serif_path && cjk_serif_path[0]) {
        g_cjk_serif_path_storage = cjk_serif_path;
        g_cjk_serif_path = g_cjk_serif_path_storage.c_str();
    }
    for (int slot = 0; slot < 4; ++slot) {
        const uint16_t size = font_size(slot);
        g_cjk_fonts[slot] = load_cjk_font(
            g_cjk_sans_path, size, LV_FREETYPE_FONT_STYLE_NORMAL);
        g_latin_fonts[slot] = cp0_fonts().get(g_latin_sans_path, size);
        g_cjk_serif_fonts[slot] = cp0_fonts().get(g_cjk_serif_path, size);
        g_latin_serif_fonts[slot] = cp0_fonts().get(g_latin_serif_path, size);
        if (g_latin_fonts[slot] != cp0_fonts().fallback(size) &&
            g_cjk_fonts[slot] != cp0_fonts().fallback(size)) {
            g_latin_fonts[slot]->fallback = g_cjk_fonts[slot];
        }
        if (g_latin_serif_fonts[slot] != cp0_fonts().fallback(size) &&
            g_cjk_serif_fonts[slot] != cp0_fonts().fallback(size)) {
            g_latin_serif_fonts[slot]->fallback = g_cjk_serif_fonts[slot];
        }
    }
}
#else
void init_runtime_fonts(const std::string &) {}
#endif

const lv_font_t *font_for_text(const std::string &text, const lv_font_t *latin)
{
#if LV_USE_FREETYPE
    if (!contains_cjk(text)) return latin;
    // Use the same Latin-primary/CJK-fallback pairing as store_font().
    // Selecting the CJK font as the primary font changes the apparent size of
    // Latin glyphs in mixed-language names.
    const int slot = font_slot(latin);
    if (slot < 0) return latin;
    lv_font_t *selected = g_latin_fonts[slot];
    return selected && selected != cp0_fonts().fallback(font_size(slot))
        ? selected
        : latin;
#else
    (void)text;
    return latin;
#endif
}

const lv_font_t *font_for_serif_text(const std::string &text, uint16_t size)
{
#if LV_USE_FREETYPE
    (void)text;
    int slot = 0;
    if (size >= 18) slot = 3;
    else if (size >= 14) slot = 2;
    else if (size >= 12) slot = 1;
    lv_font_t *selected = g_latin_serif_fonts[slot];
    return selected ? selected : cp0_fonts().fallback(size);
#else
    (void)text;
    (void)size;
    return LV_FONT_DEFAULT;
#endif
}
