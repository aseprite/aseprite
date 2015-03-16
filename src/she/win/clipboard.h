// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_WIN_CLIPBOARD_H_INCLUDED
#define SHE_WIN_CLIPBOARD_H_INCLUDED
#pragma once

#include "base/string.h"
#include "she/clipboard.h"

namespace she {

  class ClipboardWin32 : public Clipboard {
  public:
    void dispose() override;
    std::string getText(DisplayHandle hwnd) override;
    void setText(DisplayHandle hwnd, const std::string& text) override;

  private:
    std::string m_text;
  };

} // namespace she

#endif
