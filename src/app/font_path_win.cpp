// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/font_path.h"

#include "base/fs.h"
#include "base/string.h"

#include <cctype>
#include <windows.h>
#include <shlobj.h>

namespace app {

void get_font_dirs(base::paths& fontDirs)
{
  std::vector<wchar_t> buf(MAX_PATH+1);

  // Fonts in system fonts directory
  HRESULT hr = SHGetFolderPath(
    nullptr, CSIDL_FONTS, nullptr,
    SHGFP_TYPE_DEFAULT, &buf[0]);
  if (hr == S_OK)
    fontDirs.push_back(base::to_utf8(&buf[0]));

  // Fonts in ...\AppData\Local\Microsoft\Windows\Fonts
  hr = SHGetFolderPath(
    nullptr, CSIDL_LOCAL_APPDATA, nullptr,
    SHGFP_TYPE_CURRENT, &buf[0]);
  if (hr == S_OK) {
    std::string userPath = base::to_utf8(&buf[0]);
    userPath = base::join_path(userPath, "Microsoft\\Windows\\Fonts");
    if (base::is_directory(userPath) &&
        (fontDirs.empty() || userPath != fontDirs.back())) {
      fontDirs.push_back(userPath);
    }
  }
}

} // namespace app
