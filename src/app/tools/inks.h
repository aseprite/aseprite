// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/tools/ink_processing.h"

#include "app/app.h"                // TODO avoid to include this file
#include "app/color_utils.h"
#include "app/context.h"
#include "app/document.h"
#include "app/document_undo.h"
#include "app/tools/pick_ink.h"
#include "doc/mask.h"
#include "gfx/region.h"

namespace app {
namespace tools {

// Ink used for tools which paint with primary/secondary
// (or foreground/background colors)
class PaintInk : public Ink {
public:
  enum Type { Merge, WithFg, WithBg, Copy, LockAlpha };

private:
  Type m_type;
  AlgoHLine m_proc;

public:
  PaintInk(Type type) : m_type(type), m_proc(nullptr) { }

  Ink* clone() override { return new PaintInk(*this); }

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
          int color = (m_type == WithFg ? loop->getFgColor():
                                          loop->getBgColor());
          loop->setPrimaryColor(color);
          loop->setSecondaryColor(color);
        }
        break;
    }

    int depth = MID(0, loop->sprite()->pixelFormat(), 2);

    if (loop->getBrush()->type() == doc::kImageBrushType)
      m_proc = ink_processing[INK_BRUSH][depth];
    else {
      switch (m_type) {
        case Merge: {
          bool opaque = false;

          if (loop->getOpacity() == 255) {
            color_t color = loop->getPrimaryColor();

            switch (loop->sprite()->pixelFormat()) {
              case IMAGE_RGB:
                opaque = (rgba_geta(color) == 255);
                break;
              case IMAGE_GRAYSCALE:
                opaque = (graya_geta(color) == 255);
                break;
              case IMAGE_INDEXED:
                color = get_current_palette()->getEntry(color);
                opaque = (rgba_geta(color) == 255);
                break;
            }
          }

          // Use a faster ink, direct copy
          if (opaque)
            m_proc = ink_processing[INK_COPY][depth];
          else
            m_proc = ink_processing[INK_TRANSPARENT][depth];
          break;
        }
        case Copy:
          m_proc = ink_processing[INK_COPY][depth];
          break;
        case LockAlpha:
          m_proc = ink_processing[INK_LOCKALPHA][depth];
          break;
        default:
          m_proc = ink_processing[INK_TRANSPARENT][depth];
          break;
      }
    }
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop)
  {
    ASSERT(m_proc);
    (*m_proc)(x1, y, x2, loop);
  }

};


class ShadingInk : public Ink {
private:
  AlgoHLine m_proc;

public:
  ShadingInk() { }

  Ink* clone() override { return new ShadingInk(*this); }

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
  Ink* clone() override { return new ScrollInk(*this); }

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
  Ink* clone() override { return new ZoomInk(*this); }

  bool isZoom() const { return true; }
  void prepareInk(ToolLoop* loop) { }
  void inkHline(int x1, int y, int x2, ToolLoop* loop) { }
};


class MoveInk : public Ink {
public:
  Ink* clone() override { return new MoveInk(*this); }

  bool isCelMovement() const { return true; }
  void prepareInk(ToolLoop* loop) { }
  void inkHline(int x1, int y, int x2, ToolLoop* loop) { }
};


class SliceInk : public Ink {
public:
  Ink* clone() override { return new SliceInk(*this); }

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

  Ink* clone() override { return new EraserInk(*this); }

  bool isPaint() const { return true; }
  bool isEffect() const { return true; }
  bool isEraser() const { return true; }

  void prepareInk(ToolLoop* loop)
  {
    switch (m_type) {

      case Eraser:
        m_proc = ink_processing[INK_COPY][MID(0, loop->sprite()->pixelFormat(), 2)];

        // TODO app_get_color_to_clear_layer should receive the context as parameter
        loop->setPrimaryColor(app_get_color_to_clear_layer(loop->getLayer()));
        loop->setSecondaryColor(app_get_color_to_clear_layer(loop->getLayer()));
        break;

      case ReplaceFgWithBg:
        m_proc = ink_processing[INK_REPLACE][MID(0, loop->sprite()->pixelFormat(), 2)];

        loop->setPrimaryColor(loop->getFgColor());
        loop->setSecondaryColor(loop->getBgColor());
        break;

      case ReplaceBgWithFg:
        m_proc = ink_processing[INK_REPLACE][MID(0, loop->sprite()->pixelFormat(), 2)];

        loop->setPrimaryColor(loop->getBgColor());
        loop->setSecondaryColor(loop->getFgColor());
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
  Ink* clone() override { return new BlurInk(*this); }

  bool isPaint() const { return true; }
  bool isEffect() const { return true; }
  bool needsSpecialSourceArea() const { return true; }

  void prepareInk(ToolLoop* loop)
  {
    m_proc = ink_processing[INK_BLUR][MID(0, loop->sprite()->pixelFormat(), 2)];
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop)
  {
    (*m_proc)(x1, y, x2, loop);
  }

  void createSpecialSourceArea(const gfx::Region& dirtyArea, gfx::Region& sourceArea) const {
    // We need one pixel more for each side, to use a 3x3 convolution matrix.
    for (const auto& rc : dirtyArea) {
      sourceArea.createUnion(sourceArea,
        gfx::Region(gfx::Rect(rc).enlarge(1)));
    }
  }
};


class JumbleInk : public Ink {
  AlgoHLine m_proc;

public:
  Ink* clone() override { return new JumbleInk(*this); }

  bool isPaint() const { return true; }
  bool isEffect() const { return true; }
  bool needsSpecialSourceArea() const { return true; }

  void prepareInk(ToolLoop* loop)
  {
    m_proc = ink_processing[INK_JUMBLE][MID(0, loop->sprite()->pixelFormat(), 2)];
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop)
  {
    (*m_proc)(x1, y, x2, loop);
  }

  void createSpecialSourceArea(const gfx::Region& dirtyArea, gfx::Region& sourceArea) const {
    // We need one pixel more for each side.
    for (const auto& rc : dirtyArea) {
      sourceArea.createUnion(sourceArea,
        gfx::Region(gfx::Rect(rc).enlarge(1)));
    }
  }
};


// Ink used for selection tools (like Rectangle Marquee, Lasso, Magic Wand, etc.)
class SelectionInk : public Ink {
  bool m_modify_selection;
  Mask m_mask;
  Rect m_maxBounds;

public:
  SelectionInk() { m_modify_selection = false; }

  Ink* clone() override { return new SelectionInk(*this); }

  bool isSelection() const { return true; }

  void inkHline(int x1, int y, int x2, ToolLoop* loop)
  {
    if (m_modify_selection) {
      Point offset = loop->getOffset();

      switch (loop->getSelectionMode()) {
        case SelectionMode::DEFAULT:
        case SelectionMode::ADD:
          m_mask.add(gfx::Rect(x1-offset.x, y-offset.y, x2-x1+1, 1));
          break;
        case SelectionMode::SUBTRACT:
          m_mask.subtract(gfx::Rect(x1-offset.x, y-offset.y, x2-x1+1, 1));
          break;
      }

      m_maxBounds |= gfx::Rect(x1-offset.x, y-offset.y, x2-x1+1, 1);
    }
    // TODO show the selection-preview with a XOR color or something like that
    else {
      ink_processing[INK_XOR][MID(0, loop->sprite()->pixelFormat(), 2)]
        (x1, y, x2, loop);
    }
  }

  void setFinalStep(ToolLoop* loop, bool state)
  {
    m_modify_selection = state;

    if (state) {
      m_maxBounds = loop->getMask()->bounds();

      m_mask.copyFrom(loop->getMask());
      m_mask.freeze();
      m_mask.reserve(loop->sprite()->bounds());
    }
    else {
      // We can intersect the used bounds in inkHline() calls to
      // reduce the shrink computation.
      m_mask.intersect(m_maxBounds);

      m_mask.unfreeze();

      loop->setMask(&m_mask);
      loop->getDocument()->setTransformation(Transformation(m_mask.bounds()));

      m_mask.clear();
    }
  }

};


} // namespace tools
} // namespace app
