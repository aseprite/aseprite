// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_H_INCLUDED
#define APP_UI_EDITOR_H_INCLUDED
#pragma once

#include "app/app_render.h"
#include "app/color.h"
#include "app/document.h"
#include "app/pref/preferences.h"
#include "app/tools/selection_mode.h"
#include "app/ui/color_source.h"
#include "app/ui/editor/brush_preview.h"
#include "app/ui/editor/editor_observers.h"
#include "app/ui/editor/editor_state.h"
#include "app/ui/editor/editor_states_history.h"
#include "base/connection.h"
#include "doc/document_observer.h"
#include "doc/frame.h"
#include "doc/image_buffer.h"
#include "filters/tiled_mode.h"
#include "gfx/fwd.h"
#include "render/zoom.h"
#include "ui/base.h"
#include "ui/cursor_type.h"
#include "ui/timer.h"
#include "ui/widget.h"

namespace doc {
  class Layer;
  class Site;
  class Sprite;
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
                 public doc::DocumentObserver
               , public IColorSource {
  public:
    enum EditorFlags {
      kNoneFlag = 0,
      kShowGrid = 1,
      kShowMask = 2,
      kShowOnionskin = 4,
      kShowOutside = 8,
      kShowDecorators = 16,
      kShowSymmetryLine = 32,
      kDefaultEditorFlags = (kShowGrid |
                             kShowMask |
                             kShowOnionskin |
                             kShowOutside |
                             kShowDecorators |
                             kShowSymmetryLine)
    };

    enum class ZoomBehavior {
      CENTER,                   // Zoom from center (don't change center of the editor)
      MOUSE,                    // Zoom from cursor
    };

    Editor(Document* document, EditorFlags flags = kDefaultEditorFlags);
    ~Editor();

    static void destroyEditorSharedInternals();

    bool isActive() const;

    DocumentView* getDocumentView() { return m_docView; }
    void setDocumentView(DocumentView* docView) { m_docView = docView; }

    // Returns the current state.
    EditorStatePtr getState() { return m_state; }

    bool isMovingPixels() const;
    void dropMovingPixels();

    // Changes the state of the editor.
    void setState(const EditorStatePtr& newState);

    // Backs to previous state.
    void backToPreviousState();

    // Gets/sets the current decorator. The decorator is not owned by
    // the Editor, so it must be deleted by the caller.
    EditorDecorator* decorator() { return m_decorator; }
    void setDecorator(EditorDecorator* decorator) { m_decorator = decorator; }
    void getInvalidDecoratoredRegion(gfx::Region& region);

    EditorFlags editorFlags() const { return m_flags; }
    void setEditorFlags(EditorFlags flags) { m_flags = flags; }

    Document* document() { return m_document; }
    Sprite* sprite() { return m_sprite; }
    Layer* layer() { return m_layer; }
    frame_t frame() { return m_frame; }

    void getSite(Site* site) const;
    Site getSite() const;

    void setLayer(const Layer* layer);
    void setFrame(frame_t frame);

    const render::Zoom& zoom() const { return m_zoom; }
    const gfx::Point& padding() const { return m_padding; }

    void setZoom(const render::Zoom& zoom);
    void setDefaultScroll();
    void setEditorScroll(const gfx::Point& scroll);
    void setEditorZoom(const render::Zoom& zoom);

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
    gfx::Point autoScroll(ui::MouseMessage* msg, AutoScroll dir);

    tools::Tool* getCurrentEditorTool();
    tools::Ink* getCurrentEditorInk();

    tools::SelectionMode getSelectionMode() const { return m_selectionMode; }
    bool isAutoSelectLayer() const { return m_autoSelectLayer; }

    bool isSecondaryButton() const { return m_secondaryButton; }

    gfx::Point lastDrawingPosition() const { return m_lastDrawingPosition; }
    void setLastDrawingPosition(const gfx::Point& pos);

    // Returns true if we are able to draw in the current doc/sprite/layer/cel.
    bool canDraw();

    // Returns true if the cursor is inside the active mask/selection.
    bool isInsideSelection();

    void setZoomAndCenterInMouse(const render::Zoom& zoom,
      const gfx::Point& mousePos, ZoomBehavior zoomBehavior);

    void pasteImage(const Image* image, const Mask* mask = nullptr);

    void startSelectionTransformation(const gfx::Point& move, double angle);

    // Used by EditorView to notify changes in the view's scroll
    // position.
    void notifyScrollChanged();
    void notifyZoomChanged();

    // Animation control
    void play(bool playOnce);
    void stop();
    bool isPlaying() const;

    // Shows a popup menu to change the editor animation speed.
    void showAnimationSpeedMultiplierPopup(Option<bool>& playOnce,
                                           bool withStopBehaviorOptions);
    double getAnimationSpeedMultiplier() const;
    void setAnimationSpeedMultiplier(double speed);

    // Functions to be used in EditorState::onSetCursor()
    void showMouseCursor(ui::CursorType cursorType);
    void showBrushPreview(const gfx::Point& pos);

    // Gets the brush preview controller.
    BrushPreview& brushPreview() { return m_brushPreview; }

    // Returns the buffer used to render editor viewports.
    // E.g. It can be re-used by PreviewCommand
    static ImageBufferPtr getRenderImageBuffer();

    AppRender& renderEngine() { return m_renderEngine; }

    // IColorSource
    app::Color getColorByPosition(const gfx::Point& pos) override;

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    void onInvalidateRegion(const gfx::Region& region) override;
    void onCurrentToolChange();
    void onFgColorChange();
    void onContextBarBrushChange();
    void onShowExtrasChange();
    void onExposeSpritePixels(doc::DocumentEvent& ev) override;

  private:
    void setStateInternal(const EditorStatePtr& newState);
    void updateQuicktool();
    void updateContextBarFromModifiers();
    bool isCurrentToolAffectedByRightClickMode();

    void drawMaskSafe();
    void drawMask(ui::Graphics* g);
    void drawGrid(ui::Graphics* g, const gfx::Rect& spriteBounds, const gfx::Rect& gridBounds,
      const app::Color& color, int alpha);

    void setCursor(const gfx::Point& mouseScreenPos);

    // Draws the specified portion of sprite in the editor.  Warning:
    // You should setup the clip of the screen before calling this
    // routine.
    void drawOneSpriteUnclippedRect(ui::Graphics* g, const gfx::Rect& rc, int dx, int dy);

    gfx::Point calcExtraPadding(const render::Zoom& zoom);

    void invalidateIfActive();

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
    frame_t m_frame;              // Active frame in the editor
    render::Zoom m_zoom;          // Zoom in the editor
    DocumentPreferences& m_docPref;

    // Brush preview
    BrushPreview m_brushPreview;

    // Position used to draw straight lines using freehand tools + Shift key
    // (EditorCustomizationDelegate::isStraightLineFromLastPoint() modifier)
    gfx::Point m_lastDrawingPosition;

    // Current selected quicktool (this genererally should be NULL if
    // the user is not pressing any keyboard key).
    tools::Tool* m_quicktool;

    tools::SelectionMode m_selectionMode;
    bool m_autoSelectLayer;

    // Extra space around the sprite.
    gfx::Point m_padding;

    // Marching ants stuff
    ui::Timer m_antsTimer;
    int m_antsOffset;

    // This slot is used to disconnect the Editor from CurrentToolChange
    // signal (because the editor can be destroyed and the application
    // still continue running and generating CurrentToolChange
    // signals).
    base::ScopedConnection m_currentToolChangeConn;
    base::ScopedConnection m_fgColorChangeConn;
    base::ScopedConnection m_contextBarBrushChangeConn;
    base::ScopedConnection m_showExtrasConn;

    // Slots listeing document preferences.
    base::ScopedConnection m_tiledConn;
    base::ScopedConnection m_gridConn;
    base::ScopedConnection m_pixelGridConn;
    base::ScopedConnection m_bgConn;
    base::ScopedConnection m_onionskinConn;
    base::ScopedConnection m_symmetryModeConn;

    EditorObservers m_observers;

    EditorCustomizationDelegate* m_customizationDelegate;

    // TODO This field shouldn't be here. It should be removed when
    // editors.cpp are finally replaced with a fully funtional Workspace
    // widget.
    DocumentView* m_docView;

    gfx::Point m_oldPos;

    EditorFlags m_flags;

    bool m_secondaryButton;

    // Animation speed multiplier.
    double m_aniSpeed;

    static doc::ImageBufferPtr m_renderBuffer;

    // The render engine must be shared between all editors so when a
    // DrawingState is being used in one editor, other editors for the
    // same document can show the same preview image/stroke being drawn
    // (search for Render::setPreviewImage()).
    static AppRender m_renderEngine;
  };

  ui::WidgetType editor_type();

} // namespace app

#endif
