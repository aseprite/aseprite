// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_WIN_THUMBNAILS_H_INCLUDED
#define APP_WIN_THUMBNAILS_H_INCLUDED
#pragma once

#if !LAF_WINDOWS
  #error Only for Windows
#endif

#include <string>

namespace app { namespace win {

struct ThumbnailsOption {
  bool enabled = false;
  bool overlay = false;

  bool operator==(const ThumbnailsOption& other) const
  {
    return (enabled == other.enabled && overlay == other.overlay);
  }
  bool operator!=(const ThumbnailsOption& other) const { return !operator==(other); }
};

extern const char* kAsepriteThumbnailerDllName;

std::string get_thumbnailer_dll();

ThumbnailsOption get_thumbnail_options(const std::string& extension);

// Returns true if settings were changed.
bool set_thumbnail_options(const std::string& extension, const ThumbnailsOption& options);

}} // namespace app::win

#endif
