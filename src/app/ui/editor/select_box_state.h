// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_SELECT_BOX_STATE_H_INCLUDED
#define APP_UI_EDITOR_SELECT_BOX_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/editor_decorator.h"
#include "app/ui/editor/ruler.h"
#include "app/ui/editor/standby_state.h"
#include "gfx/fwd.h"
#include "ui/cursor_type.h"
#include "ui/mouse_button.h"

#include <string>
#include <vector>

namespace app {

  // State used to select boxes or points. Used for multiple-purposes:
  // - To select a new canvas size in CanvasSizeCommand
  // - To select a tile bounds in ImportSpriteSheetCommand
  // - To select and create a brush using canvas pixels from NewBrushCommand
  // - To select a point with app.editor:askPoint() API
  class SelectBoxDelegate {
  public:
    virtual ~SelectBoxDelegate() { }

    // Called each time the selected box is modified (e.g. rulers are
    // moved).
    virtual void onChangeRectangle(const gfx::Rect& rect) { }
    virtual void onChangePadding(const gfx::Size& padding) { }

    // Called only in QUICKBOX mode, when the user released the mouse
    // button.
    virtual void onQuickboxEnd(Editor* editor, const gfx::Rect& rect, ui::MouseButton button) { }
    virtual void onQuickboxCancel(Editor* editor) { }

    // Help text to be shown in the ContextBar
    virtual std::string onGetContextBarHelp() { return ""; }
  };

  class SelectBoxState : public StandbyState
                       , public EditorDecorator {
    enum { H1, H2, V1, V2, PH, PV };

  public:
    enum class Flags {
      // Draw rulers at each edge of the current box
      Rulers = 1,

      // The outside of the current box must be darker (used in "Canvas Size" command)
      DarkOutside = 2,

      // Show a horizontal array of boxes starting from the current box
      HGrid = 4,

      // Show a vertical array of boxes starting from the current box
      VGrid = 8,

      // Show a grid starting from the current box
      Grid = (HGrid | VGrid),

      // Select the box as in selection tool, drawing a boxu
      QuickBox = 16,

      // Adding 2 rules more for padding. Used in Import Sprite Sheet render
      PaddingRulers = 32,

      // Include Partial Tiles at the end of the sprite? Used in Import Sprite Sheet render
      IncludePartialTiles = 64,

      // Select just a point quickly
      QuickPoint = 128,
    };

    SelectBoxState(SelectBoxDelegate* delegate,
                   const gfx::Rect& rc,
                   Flags flags);
    ~SelectBoxState();

    Flags getFlags();
    void setFlags(Flags flags);
    void setFlag(Flags flag);
    void clearFlag(Flags flag);

    // Returns the bounding box arranged by the rulers.
    gfx::Rect getBoxBounds() const;
    void setBoxBounds(const gfx::Rect& rc);

    // Get & Set the size of the padding rulers during Import Sprite Sheet Dialog
    gfx::Size getPaddingBounds() const;
    void setPaddingBounds(const gfx::Size& padding);

    // EditorState overrides
    virtual void onEnterState(Editor* editor) override;
    virtual void onBeforePopState(Editor* editor) override;
    virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;
    virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool acceptQuickTool(tools::Tool* tool) override;
    virtual bool requireBrushPreview() override;
    virtual tools::Ink* getStateInk() const override;

    // EditorDecorator overrides
    virtual void postRenderDecorator(EditorPostRender* render) override;
    virtual void getInvalidDecoratoredRegion(Editor* editor, gfx::Region& region) override;

    // Disable Shift+click to draw straight lines with the Pencil tool
    bool canCheckStartDrawingStraightLine() override { return false; }

  private:
    typedef std::vector<Ruler> Rulers;

    void updateContextBar();
    Ruler& oppositeRuler(const int rulerIndex);

    // This returns a ui align value (e.g. LEFT for the ruler)
    int hitTestRulers(Editor* editor,
                      const gfx::Point& mousePos,
                      const bool updateMovingRulers);

    // Returns 0 if the mouse position on screen (mousePos) is not
    // touching any ruler, or in other case returns the alignment of
    // the ruler being touch (useful to show a proper mouse cursor).
    int hitTestRuler(Editor* editor,
                     const Ruler& ruler,
                     const Ruler& oppRuler,
                     const gfx::Point& mousePos);

    ui::CursorType cursorFromAlign(const int align) const;

    bool hasFlag(Flags flag) const;

    SelectBoxDelegate* m_delegate;
    Rulers m_rulers;
    Rulers m_startRulers;
    int m_rulersDragAlign;      // Used to calculate the correct mouse cursor
    std::vector<int> m_movingRulers;
    bool m_selectingBox;
    ui::MouseButton m_selectingButton;
    gfx::Point m_startingPos;
    Flags m_flags;
  };

} // namespace app

#endif  // APP_UI_EDITOR_SELECT_BOX_STATE_H_INCLUDED
