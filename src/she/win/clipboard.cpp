// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/string.h"
#include "she/win/clipboard.h"

#include <windows.h>

namespace {

bool open_clipboard(HWND hwnd)
{
  for (int i=0; i<5; ++i) {
    if (OpenClipboard(hwnd))
      return true;

    Sleep(100);
  }
  return false;
}

}

namespace she {

void ClipboardWin32::dispose()
{
  delete this;
}

std::string ClipboardWin32::getText(DisplayHandle hwnd)
{
  std::string text;

  if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
    if (open_clipboard((HWND)hwnd)) {
      HGLOBAL hglobal = GetClipboardData(CF_UNICODETEXT);
      if (hglobal != NULL) {
        LPWSTR lpstr = static_cast<LPWSTR>(GlobalLock(hglobal));
        if (lpstr != NULL) {
          text = base::to_utf8(lpstr).c_str();
          GlobalUnlock(hglobal);
        }
      }
      CloseClipboard();
    }
  }

  return text;
}

void ClipboardWin32::setText(DisplayHandle hwnd, const std::string& text)
{
  if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
    if (open_clipboard((HWND)hwnd)) {
      EmptyClipboard();

      if (!text.empty()) {
        std::wstring wstr = base::from_utf8(text);
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
}

} // namespace she
