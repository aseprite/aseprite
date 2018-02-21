// Aseprite
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/font_path.h"

#include "base/string.h"

#include <cctype>
#include <windows.h>
#include <shlobj.h>

namespace app {

void get_font_dirs(base::paths& fontDirs)
{
  std::vector<wchar_t> buf(MAX_PATH+1);
  HRESULT hr = SHGetFolderPath(
    nullptr, CSIDL_FONTS, nullptr,
    SHGFP_TYPE_DEFAULT, &buf[0]);
  if (hr == S_OK)
    fontDirs.push_back(base::to_utf8(&buf[0]));
}

} // namespace app
