/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#ifdef ALLEGRO_WINDOWS
#include <allegro.h>
#include <winalleg.h>
#endif

#include "jinete/jbase.h"

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
