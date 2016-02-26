// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_CLIPBOARD_IMPL_H_INCLUDED
#define SHE_CLIPBOARD_IMPL_H_INCLUDED
#pragma once

#include "she/clipboard.h"

namespace she {

  class ClipboardImpl : public Clipboard {
  public:
    ~ClipboardImpl() {
    }

    void dispose() override {
      delete this;
    }

    std::string getText(DisplayHandle) override {
      return m_text;
    }

    void setText(DisplayHandle, const std::string& text) override {
      m_text = text;
    }

  private:
    std::string m_text;
  };

} // namespace she

#endif
