// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/clipboard.h"

#ifdef WIN32
#include "ui/clipboard_win.h"
#else
#include "ui/clipboard_none.h"
#endif

static std::string clipboard_text;

const char* ui::clipboard::get_text()
{
  get_system_clipboard_text(clipboard_text);
  return clipboard_text.c_str();
}

void ui::clipboard::set_text(const char* text)
{
  clipboard_text = text ? text: "";
  set_system_clipboard_text(clipboard_text);
}
