// SHE library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_DRAW_TEXT_H_INCLUDED
#define SHE_DRAW_TEXT_H_INCLUDED
#pragma once

#include "base/string.h"
#include "gfx/color.h"
#include "gfx/fwd.h"

namespace she {

  class Font;
  class Surface;
  class SurfaceLock;

  class DrawTextDelegate {
  public:
    virtual ~DrawTextDelegate() { }

    // This is called before drawing the character.
    virtual void preProcessChar(const int index,
                                const int codepoint,
                                gfx::Color& fg, gfx::Color& bg) {
      // Do nothing
    }

    virtual bool preDrawChar(const gfx::Rect& charBounds) {
      // Returns false if the process should stop here.
      return true;
    }

    virtual void postDrawChar(const gfx::Rect& charBounds) {
      // Do nothing
    }
  };

  // The surface can be nullptr just to process the string
  // (e.g. measure how much space will use the text without drawing
  // it).
  gfx::Rect draw_text(Surface* surface, Font* font,
                      const base::utf8_const_iterator& begin,
                      const base::utf8_const_iterator& end,
                      gfx::Color fg, gfx::Color bg,
                      int x, int y,
                      DrawTextDelegate* delegate);

} // namespace she

#endif
