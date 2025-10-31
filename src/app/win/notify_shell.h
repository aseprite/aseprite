// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_WIN_NOTIFY_SHELL_H_INCLUDED
#define APP_WIN_NOTIFY_SHELL_H_INCLUDED
#pragma once

#if !LAF_WINDOWS
  #error Only for Windows
#endif

#include <shlobj.h>

namespace app { namespace win {

inline void notify_shell_about_association_change_regen_thumbnails()
{
  // We have to notify the system that the file associations have changed.
  // https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shchangenotify#remarks
  // https://learn.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shchangenotify
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}

}} // namespace app::win

#endif
