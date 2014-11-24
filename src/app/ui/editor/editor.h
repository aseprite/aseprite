/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#ifndef APP_UI_EDITOR_H_INCLUDED
#define APP_UI_EDITOR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/document.h"
#include "app/settings/selection_mode.h"
#include "app/settings/settings_observers.h"
#include "app/ui/editor/editor_observers.h"
#include "app/ui/editor/editor_state.h"
#include "app/ui/editor/editor_states_history.h"
#include "app/zoom.h"
#include "base/connection.h"
#include "doc/frame_number.h"
#include "gfx/fwd.h"
#include "ui/base.h"
#include "ui/timer.h"
#include "ui/widget.h"

namespace doc {
  class Sprite;
  class Layer;
}
namespace gfx {
  class Region;
}
namespace ui {
  class Graphics;
  class View;
}

namespace app {
  class Context;
  class DocumentLocation;
  class DocumentView;
  class EditorCustomizationDelegate;
  class PixelsMovement;

  namespace tools {
    class Ink;
    class Tool;
  }

  enum class AutoScroll {
    MouseDir,
    ScrollDir,
  };

  class Editor : public ui::Widget,
                 public DocumentSettingsObserver {
  public:
    typedef void (*PixelDelegate)(ui::Graphics*, const gfx::Point&, gfx::Color);

    enum EditorFlags {
      kNoneFlag = 0,
      kShowGrid = 1,
      kShowMask = 2,
      kShowOnionskin = 4,
      kShowOutside = 8,
      kShowDecorators = 16,
      kDefaultEditorFlags = (kShowGrid | kShowMask | 
        kShowOnionskin | kShowOutside | kShowDecorators),
    };

    enum ZoomBehavior {
      kCofiguredZoomBehavior,
      kCenterOnZoom,
      kDontCenterOnZoom,
    };

    Editor(Document* document, EditorFlags flags = kDefaultEditorFlags);
    ~Editor();

    DocumentView* getDocumentView() { return m_docView; }
    void setDocumentView(DocumentView* docView) { m_docView = docView; }

    // Returns the current state.
    EditorStatePtr getState() { return m_state; }

    // Changes the state of the editor.
    void setState(const EditorStatePtr& newState);

    // Backs to previous state.
    void backToPreviousState();

    // Gets/sets the current decorator. The decorator is not owned by
    // the Editor, so it must be deleted by the caller.
    EditorDecorator* decorator() { return m_decorator; }
    void setDecorator(EditorDecorator* decorator) { m_decorator = decorator; }

    EditorFlags editorFlags() const { return m_flags; }
    void setEditorFlags(EditorFlags flags) { m_flags = flags; }

    Document* document() { return m_document; }
    Sprite* sprite() { return m_sprite; }
    Layer* layer() { return m_layer; }
    FrameNumber frame() { return m_frame; }

    void getDocumentLocation(DocumentLocation* location) const;
    DocumentLocation getDocumentLocation() const;

    void setLayer(const Layer* layer);
    void setFrame(FrameNumber frame);

    const Zoom& zoom() const { return m_zoom; }
    int offsetX() const { return m_offset_x; }
    int offsetY() const { return m_offset_y; }
    int cursorThick() { return m_cursorThick; }

    void setZoom(Zoom zoom) { m_zoom = zoom; }
    void setOffsetX(int x) { m_offset_x = x; }
    void setOffsetY(int y) { m_offset_y = y; }

    void setDefaultScroll();
    void setEditorScroll(const gfx::Point& scroll, bool blit_valid_rgn);
    void setEditorZoom(Zoom zoom);

    // Updates the Editor's view.
    void updateEditor();

    // Draws the sprite taking care of the whole clipping region.
    void drawSpriteClipped(const gfx::Region& updateRegion);
    void drawSpriteUnclippedRect(ui::Graphics* g, const gfx::Rect& rc);

    void flashCurrentLayer();

    gfx::Point screenToEditor(const gfx::Point& pt);
    gfx::Point editorToScreen(const gfx::Point& pt);
    gfx::Rect screenToEditor(const gfx::Rect& rc);
    gfx::Rect editorToScreen(const gfx::Rect& rc);

    void showDrawingCursor();
    void hideDrawingCursor();
    void moveDrawingCursor();

    void addObserver(EditorObserver* observer);
    void removeObserver(EditorObserver* observer);

    void setCustomizationDelegate(EditorCustomizationDelegate* delegate);

    EditorCustomizationDelegate* getCustomizationDelegate() {
      return m_customizationDelegate;
    }

    // Returns the visible area of the active sprite.
    gfx::Rect getVisibleSpriteBounds();

    // Changes the scroll to see the given point as the center of the editor.
    void centerInSpritePoint(const gfx::Point& spritePos);

    void updateStatusBar();

    // Control scroll when cursor goes out of the editor viewport.
    gfx::Point autoScroll(ui::MouseMessage* msg, AutoScroll dir, bool blit_valid_rgn);

    tools::Tool* getCurrentEditorTool();
    tools::Ink* getCurrentEditorInk();

    SelectionMode getSelectionMode() const { return m_selectionMode; }
    bool isAutoSelectLayer() const { return m_autoSelectLayer; }

    bool isSecondaryButton() const { return m_secondaryButton; }

    // Returns true if we are able to draw in the current doc/sprite/layer/cel.
    bool canDraw();

    // Returns true if the cursor is inside the active mask/selection.
    bool isInsideSelection();

    void setZoomAndCenterInMouse(Zoom zoom,
      const gfx::Point& mousePos, ZoomBehavior zoomBehavior);

    void pasteImage(const Image* image, const gfx::Point& pos);

    void startSelectionTransformation(const gfx::Point& move);

    // Used by EditorView to notify changes in the view's scroll
    // position.
    void notifyScrollChanged();

    // in cursor.cpp

    static app::Color get_cursor_color();
    static void set_cursor_color(const app::Color& color);

    static void editor_cursor_init();
    static void editor_cursor_exit();

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onPreferredSize(ui::PreferredSizeEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    void onCurrentToolChange();
    void onFgColorChange();

    void onSetTiledMode(filters::TiledMode mode);
    void onSetGridVisible(bool state);
    void onSetGridBounds(const gfx::Rect& rect);
    void onSetGridColor(const app::Color& color);

  private:
    void setStateInternal(const EditorStatePtr& newState);
    void updateQuicktool();
    void updateContextBarFromModifiers();
    void drawBrushPreview(const gfx::Point& pos, bool refresh = true);
    void moveBrushPreview(const gfx::Point& pos, bool refresh = true);
    void clearBrushPreview(bool refresh = true);
    bool doesBrushPreviewNeedSubpixel();
    bool isCurrentToolAffectedByRightClickMode();

    void drawMaskSafe();
    void drawMask(ui::Graphics* g);
    void drawGrid(ui::Graphics* g, const gfx::Rect& spriteBounds, const gfx::Rect& gridBounds,
      const app::Color& color, int alpha);

    void editor_setcursor();

    void forEachBrushPixel(
      ui::Graphics* g,
      const gfx::Point& screenPos,
      const gfx::Point& spritePos,
      gfx::Color color,
      PixelDelegate pixelDelegate);

    // Draws the specified portion of sprite in the editor.  Warning:
    // You should setup the clip of the screen before calling this
    // routine.
    void drawOneSpriteUnclippedRect(ui::Graphics* g, const gfx::Rect& rc, int dx, int dy);

    // Stack of states. The top element in the stack is the current state (m_state).
    EditorStatesHistory m_statesHistory;

    // Current editor state (it can be shared between several editors to
    // the same document). This member cannot be NULL.
    EditorStatePtr m_state;

    // Current decorator (to draw extra UI elements).
    EditorDecorator* m_decorator;

    Document* m_document;         // Active document in the editor
    Sprite* m_sprite;             // Active sprite in the editor
    Layer* m_layer;               // Active layer in the editor
    FrameNumber m_frame;          // Active frame in the editor
    Zoom m_zoom;                  // Zoom in the editor

    // Drawing cursor
    int m_cursorThick;
    gfx::Point m_cursorScreen; // Position in the screen (view)
    gfx::Point m_cursorEditor; // Position in the editor (model)

    // Current selected quicktool (this genererally should be NULL if
    // the user is not pressing any keyboard key).
    tools::Tool* m_quicktool;

    SelectionMode m_selectionMode;
    bool m_autoSelectLayer;

    // Offset for the sprite
    int m_offset_x;
    int m_offset_y;

    // Marching ants stuff
    ui::Timer m_mask_timer;
    int m_offset_count;

    // This slot is used to disconnect the Editor from CurrentToolChange
    // signal (because the editor can be destroyed and the application
    // still continue running and generating CurrentToolChange
    // signals).
    ScopedConnection m_currentToolChangeConn;
    ScopedConnection m_fgColorChangeConn;

    EditorObservers m_observers;

    EditorCustomizationDelegate* m_customizationDelegate;

    // TODO This field shouldn't be here. It should be removed when
    // editors.cpp are finally replaced with a fully funtional Workspace
    // widget.
    DocumentView* m_docView;

    gfx::Point m_oldPos;

    EditorFlags m_flags;

    bool m_secondaryButton;
  };

  ui::WidgetType editor_type();

} // namespace app

#endif
