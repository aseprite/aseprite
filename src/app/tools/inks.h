// Aseprite
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/tools/ink_processing.h"

#include "app/app.h"                // TODO avoid to include this file
#include "app/color_utils.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/doc_undo.h"
#include "app/tools/pick_ink.h"
#include "doc/mask.h"
#include "gfx/region.h"

namespace app {
namespace tools {

class BaseInk : public Ink {
public:
  BaseInk() { }
  BaseInk(const BaseInk& other) {
    m_forcedCopyInk = other.wasForcedCopyInk();
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop) override {
    ASSERT(m_proc);
    m_proc->processScanline(x1, y, x2, loop);
  }

  void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) override {
    ASSERT(m_proc);
    m_proc->prepareForPointShape(loop, firstPoint, x, y);
  }

  void prepareVForPointShape(ToolLoop* loop, int y) override {
    ASSERT(m_proc);
    m_proc->prepareVForPointShape(loop, y);
  }

  void prepareUForPointShapeWholeScanline(ToolLoop* loop, int x1) override {
    ASSERT(m_proc);
    m_proc->prepareUForPointShapeWholeScanline(loop, x1);
  }

  void prepareUForPointShapeSlicedScanline(ToolLoop* loop, bool leftSlice, int x1) override {
    ASSERT(m_proc);
    m_proc->prepareUForPointShapeSlicedScanline(loop, leftSlice, x1);
  }

  bool wasForcedCopyInk() const override { return m_forcedCopyInk; }
  void setForcedCopyInk(const bool value) override { m_forcedCopyInk = value; }

protected:
  void setProc(BaseInkProcessing* proc) {
    m_proc.reset(proc);
  }

  BaseInkProcessing* proc() {
    return m_proc.get();
  }

private:
  InkProcessingPtr m_proc;
  bool m_forcedCopyInk = false;
};

// Ink used for tools which paint with primary/secondary
// (or foreground/background colors)
class PaintInk : public BaseInk {
public:
  enum Type { Simple, WithFg, WithBg, AlphaCompositing, Copy, LockAlpha};

private:
  Type m_type;

public:
  PaintInk(Type type) : m_type(type) { }

  Ink* clone() override { return new PaintInk(*this); }

  bool isPaint() const override { return true; }

  void prepareInk(ToolLoop* loop) override {
    switch (m_type) {

      case Simple:
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

    if (loop->getBrush()->type() == doc::kImageBrushType) {
      switch (m_type) {
        case Simple:
          setProc(get_ink_proc<BrushSimpleInkProcessing>(loop));
          break;
        case LockAlpha:
          setProc(get_ink_proc<BrushLockAlphaInkProcessing>(loop));
          break;
        case Copy:
          setProc(get_ink_proc<BrushCopyInkProcessing>(loop));
          break;
        default:
          setProc(get_ink_proc<BrushSimpleInkProcessing>(loop));
          break;
      }
    }
    else {
      // When Dynamics + Gradient + Simple Ink (modified to 'Copy' because
      // alpha(fg) == 0) is a special case that will be arranged to proceed as follows:
      // - alpha(fg) == alpha(bg) == 0   -->  Copy Ink
      // - alpha(fg) == alpha(bg) == 255 -->  Copy Ink
      // - Otherwise  -->  Transparent Ink
      if (loop->getDynamics().gradient != DynamicSensor::Static) {
        switch (m_type) {
          case Copy: {
            if (wasForcedCopyInk()) {
              bool copyInk = false;
              color_t fgColor = loop->getPrimaryColor();
              color_t bgColor = loop->getSecondaryColor();
              switch (loop->sprite()->pixelFormat()) {
                case IMAGE_RGB:
                  copyInk = (rgba_geta(fgColor) == 0 && rgba_geta(bgColor) == 0) ||
                            (rgba_geta(fgColor) == 255 && rgba_geta(bgColor) == 255);
                  break;
                case IMAGE_GRAYSCALE:
                  copyInk = (graya_geta(fgColor) == 0 && graya_geta(bgColor) == 0) ||
                            (graya_geta(fgColor) == 255 && graya_geta(bgColor) == 255);
                  break;
                case IMAGE_INDEXED:
                  int transparentColor = loop->sprite()->transparentColor();
                  copyInk = (fgColor == transparentColor && bgColor == transparentColor) ||
                            (rgba_geta(loop->getPalette()->getEntry(fgColor)) == 255 &&
                              rgba_geta(loop->getPalette()->getEntry(bgColor)) == 255);
                  break;
              }
              if (copyInk)
                setProc(get_ink_proc<CopyInkProcessing>(loop));
              else
                setProc(get_ink_proc<TransparentInkProcessing>(loop));
            }
            else
              setProc(get_ink_proc<CopyInkProcessing>(loop));
            break;
          }
          case LockAlpha:
            setProc(get_ink_proc<LockAlphaInkProcessing>(loop));
            break;
          default: // Simple or AlphaCompositing
            bool copyInk = false;
            if (loop->sprite()->pixelFormat() == IMAGE_INDEXED && m_type == Simple) {
              int transparentColor = loop->sprite()->transparentColor();
              copyInk = loop->getPrimaryColor() == transparentColor &&
                        loop->getSecondaryColor() == transparentColor;
            }
            if (copyInk)
              setProc(get_ink_proc<CopyInkProcessing>(loop));
            else
              setProc(get_ink_proc<TransparentInkProcessing>(loop));
            break;
        }
      }
      else {
        // Inks without gradient dynamics
        switch (m_type) {
          case Simple:
          case AlphaCompositing: {
            bool opaque = false;

            // Opacity is set to 255 when InkType=Simple in ToolLoopBase()
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
                  // Simple ink for indexed is better to use always
                  // opaque if opacity == 255.
                  if (m_type == Simple)
                    opaque = true;
                  else if (color == loop->sprite()->transparentColor())
                    opaque = false;
                  else {
                    color = loop->getPalette()->getEntry(color);
                    opaque = (rgba_geta(color) == 255);
                  }
                  break;
              }
            }

            // Use a faster ink, direct copy
            if (opaque)
              setProc(get_ink_proc<CopyInkProcessing>(loop));
            else
              setProc(get_ink_proc<TransparentInkProcessing>(loop));
            break;
          }
          case Copy:
            setProc(get_ink_proc<CopyInkProcessing>(loop));
            break;
          case LockAlpha:
            setProc(get_ink_proc<LockAlphaInkProcessing>(loop));
            break;
          default:
            setProc(get_ink_proc<TransparentInkProcessing>(loop));
            break;
        }
      }
    }
  }

};


class ShadingInk : public PaintInk {
public:
  ShadingInk() : PaintInk(PaintInk::Simple) { }

  Ink* clone() override { return new ShadingInk(*this); }

  bool isPaint() const override { return true; }
  bool isShading() const override { return true; }

  void prepareInk(ToolLoop* loop) override {
    if (loop->getShadingRemap()) {
      if (loop->getBrush()->type() == doc::kImageBrushType) {
        setProc(get_ink_proc<BrushShadingInkProcessing>(loop));
      }
      else {
        setProc(get_ink_proc<ShadingInkProcessing>(loop));
      }
    }
    else {
      PaintInk::prepareInk(loop);
    }
  }

};


class GradientInk : public BaseInk {
public:
  Ink* clone() override { return new GradientInk(*this); }

  bool isPaint() const override { return true; }
  bool isEffect() const override { return true; }
  bool withDitheringOptions() const override { return true; }

  void prepareInk(ToolLoop* loop) override {
    setProc(get_ink_proc<GradientInkProcessing>(loop));
  }

  void prepareForStrokes(ToolLoop* loop, Strokes& strokes) override {
    proc()->prepareForStrokes(loop, strokes);
  }

};


class ScrollInk : public Ink {
public:
  Ink* clone() override { return new ScrollInk(*this); }

  bool isScrollMovement() const override { return true; }

  void prepareInk(ToolLoop* loop) override {
    // Do nothing
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop) override {
    // Do nothing
  }

  void setForcedCopyInk(const bool value) override {
    // Do nothing
  }

};


class ZoomInk : public Ink {
public:
  Ink* clone() override { return new ZoomInk(*this); }

  bool isZoom() const override { return true; }
  void prepareInk(ToolLoop* loop) override { }
  void inkHline(int x1, int y, int x2, ToolLoop* loop) override { }
  void setForcedCopyInk(const bool value) override { }
};


class MoveInk : public Ink {
  bool m_autoSelect;
public:
  MoveInk(bool autoSelect) : m_autoSelect(autoSelect) { }

  Ink* clone() override { return new MoveInk(*this); }

  bool isCelMovement() const override { return true; }
  bool isAutoSelectLayer() const override { return m_autoSelect; }
  void prepareInk(ToolLoop* loop) override { }
  void inkHline(int x1, int y, int x2, ToolLoop* loop) override { }
  void setForcedCopyInk(const bool value) override { }
};


class SelectLayerInk : public Ink {
public:
  Ink* clone() override { return new SelectLayerInk(*this); }

  bool isCelMovement() const override { return true; }
  void prepareInk(ToolLoop* loop) override { }
  void inkHline(int x1, int y, int x2, ToolLoop* loop) override { }
  void setForcedCopyInk(const bool value) override { }
};


class SliceInk : public BaseInk {
  bool m_createSlice;
  gfx::Rect m_maxBounds;

public:
  SliceInk() : m_createSlice(false) { }

  Ink* clone() override { return new SliceInk(*this); }

  bool isSlice() const override { return true; }
  bool needsCelCoordinates() const override {
    return (m_createSlice ? false: true);
  }

  void prepareInk(ToolLoop* loop) override {
    setProc(get_ink_proc<XorInkProcessing>(loop));
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop) override {
    if (m_createSlice)
      m_maxBounds |= gfx::Rect(x1, y, x2-x1+1, 1);
    else
      BaseInk::inkHline(x1, y, x2, loop);
  }

  void setFinalStep(ToolLoop* loop, bool state) override {
    m_createSlice = state;
    if (state) {
      m_maxBounds = gfx::Rect(0, 0, 0, 0);
    }
    else {
      loop->onSliceRect(m_maxBounds);
    }
  }
};


class EraserInk : public BaseInk {
public:
  enum Type { Eraser, ReplaceFgWithBg, ReplaceBgWithFg };

private:
  Type m_type;

public:
  EraserInk(Type type) : m_type(type) { }

  Ink* clone() override { return new EraserInk(*this); }

  bool isPaint() const override { return true; }
  bool isEffect() const override { return true; }
  bool isEraser() const override { return true; }

  void prepareInk(ToolLoop* loop) override {
    switch (m_type) {
      case Eraser: {
        if (loop->getBrush()->type() == doc::kImageBrushType) {
          setProc(get_ink_proc<BrushEraserInkProcessing>(loop));
        }
        else {
          // TODO app_get_color_to_clear_layer should receive the context as parameter
          color_t clearColor = app_get_color_to_clear_layer(loop->getLayer());
          loop->setPrimaryColor(clearColor);
          loop->setSecondaryColor(clearColor);

          if (loop->getOpacity() == 255) {
            setProc(get_ink_proc<CopyInkProcessing>(loop));
          }
          else {
            // For opaque layers
            if (loop->getLayer()->isBackground()) {
              setProc(get_ink_proc<TransparentInkProcessing>(loop));
            }
            // For transparent layers
            else {
              if (loop->sprite()->pixelFormat() == IMAGE_INDEXED)
                loop->setPrimaryColor(loop->sprite()->transparentColor());

              setProc(get_ink_proc<MergeInkProcessing>(loop));
            }
          }
        }
        break;
      }
      case ReplaceFgWithBg:
        loop->setPrimaryColor(loop->getFgColor());
        loop->setSecondaryColor(loop->getBgColor());
        setProc(get_ink_proc<ReplaceInkProcessing>(loop));
        break;

      case ReplaceBgWithFg:
        loop->setPrimaryColor(loop->getBgColor());
        loop->setSecondaryColor(loop->getFgColor());
        setProc(get_ink_proc<ReplaceInkProcessing>(loop));
        break;
    }
  }

};


class BlurInk : public BaseInk {
public:
  Ink* clone() override { return new BlurInk(*this); }

  bool isPaint() const override { return true; }
  bool isEffect() const override { return true; }
  bool needsSpecialSourceArea() const override { return true; }

  void prepareInk(ToolLoop* loop) override {
    setProc(get_ink_proc<BlurInkProcessing>(loop));
  }

  void createSpecialSourceArea(const gfx::Region& dirtyArea, gfx::Region& sourceArea) const override {
    // We need one pixel more for each side, to use a 3x3 convolution matrix.
    for (const auto& rc : dirtyArea) {
      sourceArea.createUnion(sourceArea,
        gfx::Region(gfx::Rect(rc).enlarge(1)));
    }
  }
};


class JumbleInk : public BaseInk {
public:
  Ink* clone() override { return new JumbleInk(*this); }

  bool isPaint() const override { return true; }
  bool isEffect() const override { return true; }
  bool needsSpecialSourceArea() const override { return true; }

  void prepareInk(ToolLoop* loop) override {
    setProc(get_ink_proc<JumbleInkProcessing>(loop));
  }

  void createSpecialSourceArea(const gfx::Region& dirtyArea, gfx::Region& sourceArea) const override {
    // We need one pixel more for each side.
    for (const auto& rc : dirtyArea) {
      sourceArea.createUnion(sourceArea,
        gfx::Region(gfx::Rect(rc).enlarge(1)));
    }
  }
};


// Ink used for selection tools (like Rectangle Marquee, Lasso, Magic Wand, etc.)
class SelectionInk : public BaseInk {
  bool m_modify_selection;
  Mask m_mask;
  Mask m_intersectMask;
  Rect m_maxBounds;

public:
  SelectionInk()
    : m_modify_selection(false) { }

  Ink* clone() override { return new SelectionInk(*this); }

  void prepareInk(ToolLoop* loop) override {
    setProc(get_ink_proc<XorInkProcessing>(loop));
  }

  bool isSelection() const override { return true; }
  bool needsCelCoordinates() const override {
    return (m_modify_selection ? false: true);
  }

  void inkHline(int x1, int y, int x2, ToolLoop* loop) override {
    if (m_modify_selection) {
      int modifiers = int(loop->getModifiers());

      if ((modifiers & (int(ToolLoopModifiers::kReplaceSelection) |
                        int(ToolLoopModifiers::kAddSelection))) != 0) {
        m_mask.add(gfx::Rect(x1, y, x2-x1+1, 1));
      }
      else if ((modifiers & int(ToolLoopModifiers::kSubtractSelection)) != 0) {
        m_mask.subtract(gfx::Rect(x1, y, x2-x1+1, 1));
      }
      else if ((modifiers & int(ToolLoopModifiers::kIntersectSelection)) != 0) {
        m_intersectMask.add(gfx::Rect(x1, y, x2-x1+1, 1));
      }

      m_maxBounds |= gfx::Rect(x1, y, x2-x1+1, 1);
    }
    else {
      BaseInk::inkHline(x1, y, x2, loop);
    }
  }

  void setFinalStep(ToolLoop* loop, bool state) override {
    m_modify_selection = state;
    int modifiers = int(loop->getModifiers());

    if (state) {
      m_maxBounds = loop->getMask()->bounds();

      m_mask.copyFrom(loop->getMask());
      m_mask.freeze();
      m_mask.reserve(loop->sprite()->bounds());

      if ((modifiers & int(ToolLoopModifiers::kIntersectSelection)) != 0) {
        m_intersectMask.clear();
        m_intersectMask.reserve(loop->sprite()->bounds());
      }
    }
    else {
      if ((modifiers & int(ToolLoopModifiers::kIntersectSelection)) != 0) {
        m_mask.intersect(m_intersectMask);
        m_intersectMask.clear();
      }

      // We can intersect the used bounds in inkHline() calls to
      // reduce the shrink computation.
      m_mask.intersect(m_maxBounds);

      m_mask.unfreeze();

      loop->setMask(&m_mask);
      loop->getDocument()->setTransformation(
        Transformation(RectF(m_mask.bounds())));

      m_mask.clear();
    }
  }

};


} // namespace tools
} // namespace app
