/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "app/tools/ink_processing.h"

#include "app/app.h"                // TODO avoid to include this file
#include "app/color_utils.h"
#include "app/context.h"
#include "app/document.h"
#include "app/document_undo.h"
#include "app/settings/settings.h"
#include "app/tools/pick_ink.h"
#include "app/undoers/set_mask.h"
#include "raster/mask.h"

namespace app {
namespace tools {

// Ink used for tools which paint with primary/secondary
// (or foreground/background colors)
class PaintInk : public Ink {
public:
  enum Type { Merge, WithFg, WithBg, Opaque, SetAlpha, LockAlpha };

private:
  AlgoHLine m_proc;
  Type m_type;

public:
  PaintInk(Type type) : m_type(type) { }

  bool isPaint() const { return true; }

  void prepareInk(ToolLoop* loop)
  {
    switch (m_type) {

      case Merge:
        // Do nothing, use the default colors
        break;

      case WithFg:
      case WithBg:
        {
          int color = color_utils::color_for_layer(m_type == WithFg ?
                                                   loop->settings()->getFgColor():
                                                   loop->settings()->getBgColor(),
                                                   loop->getLayer());
          loop->setPrimaryColor(color);
          loop->setSecondaryColor(color);
        }
        break;
    }

    int depth = MID(0, loop->sprite()->pixelFormat(), 2);

    switch (m_type) {
      case Opaque:
        m_proc = ink_processing[INK_OPAQUE][depth];
        break;
      case SetAlpha:
        m_proc = ink_processing[INK_SETALPHA][depth];
        break;
      case LockAlpha:
        m_proc = ink_processing[INK_LOCKALPHA][depth];
        break;
      default:
        m_proc = (loop->getOpacity() == 255 ?
                  ink_processing[INK_OPAQUE][depth]:
                  ink_processing[INK_TRANSPARENT][depth]);
        break;
    }
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop)
  {
    (*m_proc)(x1, y, x2, loop);
  }
};


class ShadingInk : public Ink {
private:
  AlgoHLine m_proc;

public:
  ShadingInk() { }

  bool isPaint() const { return true; }

  void prepareInk(ToolLoop* loop)
  {
    m_proc = ink_processing[INK_SHADING][MID(0, loop->sprite()->pixelFormat(), 2)];
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop)
  {
    (*m_proc)(x1, y, x2, loop);
  }
};


class ScrollInk : public Ink {
public:

  bool isScrollMovement() const { return true; }

  void prepareInk(ToolLoop* loop)
  {
    // Do nothing
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop)
  {
    // Do nothing
  }

};


class ZoomInk : public Ink {
public:
  bool isZoom() const { return true; }
  void prepareInk(ToolLoop* loop) { }
  void inkHline(int x1, int y, int x2, ToolLoop* loop) { }
};


class MoveInk : public Ink {
public:
  bool isCelMovement() const { return true; }
  void prepareInk(ToolLoop* loop) { }
  void inkHline(int x1, int y, int x2, ToolLoop* loop) { }
};


class SliceInk : public Ink {
public:
  bool isSlice() const { return true; }
  void prepareInk(ToolLoop* loop) { }
  void inkHline(int x1, int y, int x2, ToolLoop* loop) {
    // TODO show the selection-preview with a XOR color or something like that
    draw_hline(loop->getDstImage(), x1, y, x2, loop->getPrimaryColor());
  }
};


class EraserInk : public Ink {
public:
  enum Type { Eraser, ReplaceFgWithBg, ReplaceBgWithFg };

private:
  AlgoHLine m_proc;
  Type m_type;

public:
  EraserInk(Type type) : m_type(type) { }

  bool isPaint() const { return true; }
  bool isEffect() const { return true; }
  bool isEraser() const { return true; }

  void prepareInk(ToolLoop* loop)
  {
    switch (m_type) {

      case Eraser:
        m_proc = ink_processing[INK_OPAQUE][MID(0, loop->sprite()->pixelFormat(), 2)];

        // TODO app_get_color_to_clear_layer should receive the context as parameter
        loop->setPrimaryColor(app_get_color_to_clear_layer(loop->getLayer()));
        loop->setSecondaryColor(app_get_color_to_clear_layer(loop->getLayer()));
        break;

      case ReplaceFgWithBg:
        m_proc = ink_processing[INK_REPLACE][MID(0, loop->sprite()->pixelFormat(), 2)];

        loop->setPrimaryColor(color_utils::color_for_layer(loop->settings()->getFgColor(),
                                                           loop->getLayer()));
        loop->setSecondaryColor(color_utils::color_for_layer(loop->settings()->getBgColor(),
                                                             loop->getLayer()));
        break;

      case ReplaceBgWithFg:
        m_proc = ink_processing[INK_REPLACE][MID(0, loop->sprite()->pixelFormat(), 2)];

        loop->setPrimaryColor(color_utils::color_for_layer(loop->settings()->getBgColor(),
                                                           loop->getLayer()));
        loop->setSecondaryColor(color_utils::color_for_layer(loop->settings()->getFgColor(),
                                                             loop->getLayer()));
        break;
    }
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop)
  {
    (*m_proc)(x1, y, x2, loop);
  }
};


class BlurInk : public Ink {
  AlgoHLine m_proc;

public:
  bool isPaint() const { return true; }
  bool isEffect() const { return true; }

  void prepareInk(ToolLoop* loop)
  {
    m_proc = ink_processing[INK_BLUR][MID(0, loop->sprite()->pixelFormat(), 2)];
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop)
  {
    (*m_proc)(x1, y, x2, loop);
  }
};


class JumbleInk : public Ink {
  AlgoHLine m_proc;

public:
  bool isPaint() const { return true; }
  bool isEffect() const { return true; }

  void prepareInk(ToolLoop* loop)
  {
    m_proc = ink_processing[INK_JUMBLE][MID(0, loop->sprite()->pixelFormat(), 2)];
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop)
  {
    (*m_proc)(x1, y, x2, loop);
  }
};


// Ink used for selection tools (like Rectangle Marquee, Lasso, Magic Wand, etc.)
class SelectionInk : public Ink {
  bool m_modify_selection;

public:
  SelectionInk() { m_modify_selection = false; }

  bool isSelection() const { return true; }

  void inkHline(int x1, int y, int x2, ToolLoop* loop)
  {
    if (m_modify_selection) {
      Point offset = loop->getOffset();

      switch (loop->getSelectionMode()) {
        case kDefaultSelectionMode:
        case kAddSelectionMode:
          loop->getMask()->add(x1-offset.x, y-offset.y, x2-x1+1, 1);
          break;
        case kSubtractSelectionMode:
          loop->getMask()->subtract(x1-offset.x, y-offset.y, x2-x1+1, 1);
          break;
      }
    }
    // TODO show the selection-preview with a XOR color or something like that
    else {
      draw_hline(loop->getDstImage(), x1, y, x2, loop->getPrimaryColor());
    }
  }

  void setFinalStep(ToolLoop* loop, bool state)
  {
    m_modify_selection = state;

    if (state) {
      DocumentUndo* undo = loop->getDocument()->getUndo();
      if (undo->isEnabled())
        undo->pushUndoer(new undoers::SetMask(undo->getObjects(), loop->getDocument()));

      loop->getMask()->freeze();
      loop->getMask()->reserve(0, 0, loop->sprite()->width(), loop->sprite()->height());
    }
    else {
      loop->getMask()->unfreeze();
      loop->getDocument()->setTransformation(Transformation(loop->getMask()->bounds()));
      loop->getDocument()->setMaskVisible(true);
    }
  }

};


} // namespace tools
} // namespace app
