// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_CLIPBOARD_H_INCLUDED
#define SHE_CLIPBOARD_H_INCLUDED
#pragma once

#include <string>

namespace she {

  class Clipboard {
  public:
    virtual ~Clipboard() { }
    virtual void dispose() = 0;
    virtual std::string getText() = 0;
    virtual void setText(const std::string& text) = 0;
  };

} // namespace she

#endif
