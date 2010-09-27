// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#ifdef ALLEGRO_WINDOWS
#include <allegro.h>
#include <winalleg.h>
#endif

#include "gui/jbase.h"

static char *clipboard_text = NULL;

static void lowlevel_set_clipboard_text(const char *text)
{
  if (clipboard_text)
    jfree(clipboard_text);

  clipboard_text = text ? jstrdup(text) : NULL;
}

const char* jclipboard_get_text()
{
#ifdef ALLEGRO_WINDOWS
  if (IsClipboardFormatAvailable(CF_TEXT)) {
    if (OpenClipboard(win_get_window())) {
      HGLOBAL hglobal = GetClipboardData(CF_TEXT);
      if (hglobal != NULL) {
	LPSTR lpstr = static_cast<LPSTR>(GlobalLock(hglobal));
	if (lpstr != NULL) {
	  lowlevel_set_clipboard_text(lpstr);
	  GlobalUnlock(hglobal);
	}
      }
      CloseClipboard();
    }
  }
#endif

  return clipboard_text;
}

void jclipboard_set_text(const char *text)
{
  lowlevel_set_clipboard_text(text);

#ifdef ALLEGRO_WINDOWS
  if (IsClipboardFormatAvailable(CF_TEXT)) {
    if (OpenClipboard(win_get_window())) {
      EmptyClipboard();

      if (clipboard_text) {
	int len = ustrlen(clipboard_text);

	HGLOBAL hglobal = GlobalAlloc(GMEM_MOVEABLE, sizeof(char)*(len+1));
	LPSTR lpstr = static_cast<LPSTR>(GlobalLock(hglobal));
	memcpy(lpstr, clipboard_text, len);
	GlobalUnlock(hglobal);
 
	SetClipboardData(CF_TEXT, hglobal);
      }
      CloseClipboard();
    }
  }
#endif
}
