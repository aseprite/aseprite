// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_LOSPEC_H_INCLUDED
#define APP_LOSPEC_H_INCLUDED
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace app {
namespace lospec {

// Information about a single palette available on the Lospec
// palette-list (https://lospec.com/palette-list).
struct PaletteInfo {
  std::string title; // Human readable palette name, e.g. "Resurrect 64".
  std::string slug;  // URL slug used to download the palette, e.g. "resurrect-64".
  std::vector<uint32_t> colors; // RGBA colors for the on-screen preview.
};

// How the colorNumber value is applied when filtering the
// palette-list by number of colors.
enum class ColorFilter {
  Any,   // No filtering (colorNumber is ignored).
  Exact, // Palettes with exactly colorNumber colors.
  Min,   // Palettes with at least colorNumber colors.
  Max,   // Palettes with at most colorNumber colors.
};

// Returns the URL used to query the Lospec palette-list for the given
// search term and (1-based) page number. The returned URL produces a
// JSON document. The result can optionally be filtered by number of
// colors (filter + colorNumber).
std::string make_search_url(const std::string& query,
                            int page,
                            ColorFilter filter = ColorFilter::Any,
                            int colorNumber = 0);

// Returns the URL to download the given palette slug as a GIMP
// palette (.gpl) file, which Aseprite can load natively.
std::string make_download_url(const std::string& slug);

// Parses the JSON returned by make_search_url() into a list of
// palettes. Returns false if the JSON could not be parsed. If
// totalCount is not null, it is set to the total number of palettes
// available for the query (across all pages).
bool parse_search_result(const std::string& json,
                         std::vector<PaletteInfo>& result,
                         int* totalCount = nullptr);

} // namespace lospec
} // namespace app

#endif
