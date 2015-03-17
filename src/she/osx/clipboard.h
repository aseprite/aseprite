// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_OSX_CLIPBOARD_H_INCLUDED
#define SHE_OSX_CLIPBOARD_H_INCLUDED
#pragma once

#include "base/string.h"
#include "she/clipboard.h"

namespace she {

  class ClipboardOSX : public Clipboard {
  public:
    void dispose() override;
    std::string getText(DisplayHandle display) override;
    void setText(DisplayHandle display, const std::string& text) override;

  private:
    std::string m_text;
  };

} // namespace she

#endif
