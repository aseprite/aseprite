// SHE library
// Copyright (C) 2012-2014  David Capello
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

    void dispose() OVERRIDE {
      delete this;
    }

    std::string getText() OVERRIDE {
      return m_text;
    }

    void setText(const std::string& text) {
      m_text = text;
    }

  private:
    std::string m_text;
  };

} // namespace she

#endif
