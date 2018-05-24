// Aseprite UI Library
// Copyright (C) 2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_CLIPBOARD_DELEGATE_H_INCLUDED
#define UI_CLIPBOARD_DELEGATE_H_INCLUDED
#pragma once

#include <string>

namespace ui {

  class ClipboardDelegate {
  public:
    virtual ~ClipboardDelegate() { }
    virtual void setClipboardText(const std::string& text) = 0;
    virtual bool getClipboardText(std::string& text) = 0;
  };

} // namespace ui

#endif
