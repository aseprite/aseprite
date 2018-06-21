// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_EDITOR_DECORATOR_H_INCLUDED
#define APP_UI_EDITOR_EDITOR_DECORATOR_H_INCLUDED
#pragma once

#include "gfx/color.h"
#include "gfx/rect.h"

namespace gfx {
  class Region;
}

namespace doc {
  class Image;
}

namespace ui {
  class Graphics;
}

namespace app {
  class Editor;
  class EditorDecorator;

  using namespace doc;

  // EditorPostRender is an interface used to draw elements in the
  // editor's area. It's implemented by the editor and used by a
  // EditorDecorator.
  class EditorPostRender {
  public:
    virtual ~EditorPostRender() { }
    virtual Editor* getEditor() = 0;
    virtual void drawLine(gfx::Color color, int x1, int y1, int x2, int y2) = 0;
    virtual void drawRectXor(const gfx::Rect& rc) = 0;
    virtual void fillRect(gfx::Color color, const gfx::Rect& rc) = 0;
  };

  // Used by editor's states to pre- and post-render customized
  // decorations depending of the state (e.g. SelectBoxState draws the
  // selected bounds/canvas area).
  class EditorDecorator {
  public:
    virtual ~EditorDecorator() { }
    virtual void postRenderDecorator(EditorPostRender* render) = 0;
    virtual void getInvalidDecoratoredRegion(Editor* editor, gfx::Region& region) = 0;
  };

} // namespace app

#endif  // APP_UI_EDITOR_EDITOR_DECORATOR_H_INCLUDED
