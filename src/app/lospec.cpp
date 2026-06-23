// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/lospec.h"

#include "base/replace_string.h"

#include "json11.hpp"

#include <cstdlib>

namespace app {
namespace lospec {

namespace {

// URL-encodes a search term in a minimal way (enough for typical
// palette names: letters, digits, spaces and a few punctuation marks).
std::string url_encode(const std::string& str)
{
  std::string out;
  out.reserve(str.size() * 3);
  for (const unsigned char ch : str) {
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
        ch == '-' || ch == '_' || ch == '.' || ch == '~') {
      out.push_back(char(ch));
    }
    else if (ch == ' ') {
      out.push_back('+');
    }
    else {
      char buf[4];
      std::snprintf(buf, sizeof(buf), "%%%02X", int(ch));
      out += buf;
    }
  }
  return out;
}

// Parses a "rrggbb" (or "#rrggbb") hex string into an RGBA value.
// Returns true on success.
bool parse_hex_color(const std::string& hex, uint32_t& out)
{
  std::string h = hex;
  if (!h.empty() && h[0] == '#')
    h = h.substr(1);
  if (h.size() != 6)
    return false;

  char* end = nullptr;
  const unsigned long value = std::strtoul(h.c_str(), &end, 16);
  if (end != h.c_str() + 6)
    return false;

  const uint32_t r = (value >> 16) & 0xff;
  const uint32_t g = (value >> 8) & 0xff;
  const uint32_t b = (value) & 0xff;
  out = (uint32_t(0xff) << 24) | (b << 16) | (g << 8) | r; // doc::rgba layout
  return true;
}

// Removes icons/emoji from a palette title. Lospec titles are UTF-8 and
// some of them include emoji (e.g. "👌31" or "👍 31"); these multi-byte
// sequences render poorly in the list, so we drop all non-ASCII bytes
// and collapse the resulting whitespace. Plain ASCII titles (the vast
// majority) are left untouched.
std::string clean_title(const std::string& title)
{
  std::string out;
  out.reserve(title.size());
  for (const unsigned char ch : title) {
    // Keep printable ASCII only (0x20-0x7E); drop UTF-8 multi-byte
    // bytes (>= 0x80, i.e. emoji/icons) and control characters.
    if (ch >= 0x20 && ch < 0x7f)
      out.push_back(char(ch));
    else
      out.push_back(' '); // Replace removed glyphs with a space so words don't merge.
  }

  // Collapse consecutive spaces into one.
  std::string collapsed;
  collapsed.reserve(out.size());
  bool prevSpace = false;
  for (const char ch : out) {
    const bool isSpace = (ch == ' ');
    if (isSpace && prevSpace)
      continue;
    collapsed.push_back(ch);
    prevSpace = isSpace;
  }

  // Trim leading/trailing spaces.
  const auto first = collapsed.find_first_not_of(' ');
  if (first == std::string::npos)
    return std::string();
  const auto last = collapsed.find_last_not_of(' ');
  return collapsed.substr(first, last - first + 1);
}

} // anonymous namespace

std::string make_search_url(const std::string& query,
                            int page,
                            ColorFilter filter,
                            int colorNumber)
{
  if (page < 1)
    page = 1;

  const char* filterType = "any";
  switch (filter) {
    case ColorFilter::Exact: filterType = "exact"; break;
    case ColorFilter::Min:   filterType = "min"; break;
    case ColorFilter::Max:   filterType = "max"; break;
    case ColorFilter::Any:   filterType = "any"; break;
  }

  // colorNumber is only meaningful when an actual filter is selected.
  const int number = (filter == ColorFilter::Any ? 0 : colorNumber);

  // The Lospec palette-list "load" endpoint returns a JSON document
  // with a "palettes" array. When the tag is empty it returns the
  // most popular palettes.
  std::string url = "https://lospec.com/palette-list/load"
                    "?colorNumberFilterType=" +
                    std::string(filterType) + "&colorNumber=" + std::to_string(number) +
                    "&page=" + std::to_string(page) + "&tag=" + url_encode(query) +
                    "&sortingType=default";
  return url;
}

std::string make_download_url(const std::string& slug)
{
  return "https://lospec.com/palette-list/" + slug + ".gpl";
}

bool parse_search_result(const std::string& json,
                         std::vector<PaletteInfo>& result,
                         int* totalCount)
{
  std::string err;
  const json11::Json doc = json11::Json::parse(json, err);
  if (!err.empty() || !doc.is_object())
    return false;

  if (totalCount) {
    const json11::Json& tc = doc["totalCount"];
    if (tc.is_number())
      *totalCount = tc.int_value();
  }

  const json11::Json& palettes = doc["palettes"];
  if (!palettes.is_array())
    return false;

  for (const json11::Json& pal : palettes.array_items()) {
    if (!pal.is_object())
      continue;

    PaletteInfo info;
    info.title = clean_title(pal["title"].string_value());
    info.slug = pal["slug"].string_value();

    // Skip palettes that we wouldn't be able to download.
    if (info.slug.empty())
      continue;
    // Fall back to the slug if the title was empty or consisted only
    // of icons/emoji that were stripped out.
    if (info.title.empty())
      info.title = info.slug;

    // "colors" (and the equivalent "colorsArray") is an array of hex
    // strings without the leading '#'.
    const json11::Json& colors = pal["colors"];
    if (colors.is_array()) {
      for (const json11::Json& c : colors.array_items()) {
        uint32_t rgba;
        if (c.is_string() && parse_hex_color(c.string_value(), rgba))
          info.colors.push_back(rgba);
      }
    }

    result.push_back(std::move(info));
  }

  return true;
}

} // namespace lospec
} // namespace app
