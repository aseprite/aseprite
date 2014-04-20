// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/clipboard.h"

#include "she/clipboard.h"
#include "ui/manager.h"

#include <string>

static std::string clipboard_text;

namespace ui {
namespace clipboard {

const char* get_text()
{
  clipboard_text = Manager::getDefault()->getClipboard()->getText();
  return clipboard_text.c_str();
}

void set_text(const char* text)
{
  clipboard_text = text ? text: "";
  Manager::getDefault()->getClipboard()->setText(clipboard_text);
}

} // namespace clipboard
} // namespace ui
