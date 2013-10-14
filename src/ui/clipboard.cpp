// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/clipboard.h"

#include "base/string.h"

#include <algorithm>

#ifdef WIN32
#include <allegro.h>
#include <winalleg.h>
#endif

#pragma warning(disable:4996)   // To void MSVC warning about std::copy() with unsafe arguments

static base::string clipboard_text;

static void lowlevel_set_clipboard_text(const char *text)
{
  clipboard_text = text ? text: "";
}

const char* ui::clipboard::get_text()
{
#ifdef WIN32
  if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
    if (OpenClipboard(win_get_window())) {
      HGLOBAL hglobal = GetClipboardData(CF_UNICODETEXT);
      if (hglobal != NULL) {
        LPWSTR lpstr = static_cast<LPWSTR>(GlobalLock(hglobal));
        if (lpstr != NULL) {
          lowlevel_set_clipboard_text(base::to_utf8(lpstr).c_str());
          GlobalUnlock(hglobal);
        }
      }
      CloseClipboard();
    }
  }
#endif

  return clipboard_text.c_str();
}

void ui::clipboard::set_text(const char *text)
{
  lowlevel_set_clipboard_text(text);

#ifdef WIN32
  if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
    if (OpenClipboard(win_get_window())) {
      EmptyClipboard();

      if (!clipboard_text.empty()) {
        std::wstring wstr = base::from_utf8(clipboard_text);
        int len = wstr.size();

        HGLOBAL hglobal = GlobalAlloc(GMEM_MOVEABLE |
                                      GMEM_ZEROINIT, sizeof(WCHAR)*(len+1));

        LPWSTR lpstr = static_cast<LPWSTR>(GlobalLock(hglobal));
        std::copy(wstr.begin(), wstr.end(), lpstr);
        GlobalUnlock(hglobal);

        SetClipboardData(CF_UNICODETEXT, hglobal);
      }
      CloseClipboard();
    }
  }
#endif
}
