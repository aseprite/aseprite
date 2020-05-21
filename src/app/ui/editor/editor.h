// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_H_INCLUDED
#define APP_UI_EDITOR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/doc.h"
#include "app/doc_observer.h"
#include "app/pref/preferences.h"
#include "app/tools/active_tool_observer.h"
#include "app/tools/tool_loop_modifiers.h"
#include "app/ui/color_source.h"
#include "app/ui/editor/brush_preview.h"
#include "app/ui/editor/editor_hit.h"
#include "app/ui/editor/editor_observers.h"
#include "app/ui/editor/editor_state.h"
#include "app/ui/editor/editor_states_history.h"
#include "doc/algorithm/flip_type.h"
#include "doc/frame.h"
#include "doc/image_buffer.h"
#include "doc/selected_objects.h"
#include "filters/tiled_mode.h"
#include "gfx/fwd.h"
#include "obs/connection.h"
#include "os/color_space.h"
#include "render/projection.h"
#include "render/zoom.h"
#include "ui/base.h"
#include "ui/cursor_type.h"
#include "ui/pointer_type.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <set>

namespace doc {
  class Layer;
  class Sprite;
}
namespace gfx {
  class Region;
}
namespace ui {
  class Cursor;
  class Graphics;
  class View;
}

namespace app {
  class Context;
  class DocView;
  class EditorCustomizationDelegate;
  class EditorRender;
  class PixelsMovement;
  class Site;

  namespace tools {
    class Ink;
    class Tool;
  }

  enum class AutoScroll {
    MouseDir,
    ScrollDir,
  };

  class Editor : public ui::Widget,
                 public app::DocObserver,
                 public IColorSource,
                 public tools::ActiveToolObserver {
  public:
    enum EditorFlags {
      kNoneFlag = 0,
      kShowGrid = 1,
      kShowMask = 2,
      kShowOnionskin = 4,
      kShowOutside = 8,
      kShowDecorators = 16,
      kShowSymmetryLine = 32,
      kShowSlices = 64,
      kUseNonactiveLayersOpacityWhenEnabled = 128,
      kDefaultEditorFlags = (kShowGrid |
                             kShowMask |
                             kShowOnionskin |
                             kShowOutside |
                             kShowDecorators |
                             kShowSymmetryLine |
                             kShowSlices |
                             kUseNonactiveLayersOpacityWhenEnabled)
    };

    enum class ZoomBehavior {
      CENTER,                   // Zoom from center (don't change center of the editor)
      MOUSE,                    // Zoom from cursor
    };

    static ui::WidgetType Type();

    Editor(Doc* document, EditorFlags flags = kDefaultEditorFlags);
    ~Editor();

    static void destroyEditorSharedInternals();

    bool isActive() const;

    DocView* getDocView() { return m_docView; }
    void setDocView(DocView* docView) { m_docView = docView; }

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

    bool isExtraCelLocked() const {
      return m_flashing != Flashing::None;
    }

    Doc* document() { return m_document; }
    Sprite* sprite() { return m_sprite; }
    Layer* layer() { return m_layer; }
    frame_t frame() { return m_frame; }
    DocumentPreferences& docPref() { return m_docPref; }

    void getSite(Site* site) const;
    Site getSite() const;

    void setLayer(const Layer* layer);
    void setFrame(frame_t frame);

    const render::Projection& projection() const { return m_proj; }
    const render::Zoom& zoom() const { return m_proj.zoom(); }
    const gfx::Point& padding() const { return m_padding; }

    void setZoom(const render::Zoom& zoom);
    void setDefaultScroll();
    void setScrollToCenter();
    void setScrollAndZoomToFitScreen();
    void setEditorScroll(const gfx::Point& scroll);
    void setEditorZoom(const render::Zoom& zoom);

    // Updates the Editor's view.
    void updateEditor(const bool restoreScrollPos);

    // Draws the sprite taking care of the whole clipping region.
    void drawSpriteClipped(const gfx::Region& updateRegion);

    void flashCurrentLayer();

    gfx::Point screenToEditor(const gfx::Point& pt);
    gfx::PointF screenToEditorF(const gfx::Point& pt);
    gfx::Point editorToScreen(const gfx::Point& pt);
    gfx::PointF editorToScreenF(const gfx::PointF& pt);
    gfx::Rect screenToEditor(const gfx::Rect& rc);
    gfx::Rect editorToScreen(const gfx::Rect& rc);
    gfx::RectF editorToScreenF(const gfx::RectF& rc);

    void add_observer(EditorObserver* observer);
    void remove_observer(EditorObserver* observer);

    void setCustomizationDelegate(EditorCustomizationDelegate* delegate);

    EditorCustomizationDelegate* getCustomizationDelegate() {
      return m_customizationDelegate;
    }

    // Returns the visible area of the viewport in sprite coordinates.
    gfx::Rect getViewportBounds();

    // Returns the visible area of the active sprite.
    gfx::Rect getVisibleSpriteBounds();

    gfx::Size canvasSize() const;
    gfx::Point mainTilePosition() const;
    void expandRegionByTiledMode(gfx::Region& rgn,
                                 const bool withProj) const;
    void collapseRegionByTiledMode(gfx::Region& rgn) const;

    // Changes the scroll to see the given point as the center of the editor.
    void centerInSpritePoint(const gfx::Point& spritePos);

    void updateStatusBar();

    // Control scroll when cursor goes out of the editor viewport.
    gfx::Point autoScroll(ui::MouseMessage* msg, AutoScroll dir);

    tools::Tool* getCurrentEditorTool() const;
    tools::Ink* getCurrentEditorInk() const;

    tools::ToolLoopModifiers getToolLoopModifiers() const { return m_toolLoopModifiers; }
    bool isAutoSelectLayer();

    // Returns true if we are able to draw in the current doc/sprite/layer/cel.
    bool canDraw();

    // Returns true if the cursor is inside the active mask/selection.
    bool isInsideSelection();

    // Returns true if the cursor is inside the selection and the
    // selection mode is the default one which prioritizes and easy
    // way to move the selection.
    bool canStartMovingSelectionPixels();

    // Returns true if the range selected in the timeline should be
    // kept. E.g. When we are moving/transforming pixels on multiple
    // cels, the MovingPixelsState can handle previous/next frame
    // commands, so it's nice to keep the timeline range intact while
    // we are in the MovingPixelsState.
    bool keepTimelineRange();

    // Returns the element that will be modified if the mouse is used
    // in the given position.
    EditorHit calcHit(const gfx::Point& mouseScreenPos);

    void setZoomAndCenterInMouse(const render::Zoom& zoom,
      const gfx::Point& mousePos, ZoomBehavior zoomBehavior);

    void pasteImage(const Image* image, const Mask* mask = nullptr);

    void startSelectionTransformation(const gfx::Point& move, double angle);

    void startFlipTransformation(doc::algorithm::FlipType flipType);

    // Used by EditorView to notify changes in the view's scroll
    // position.
    void notifyScrollChanged();
    void notifyZoomChanged();

    // Returns true and changes to ScrollingState when "msg" says "the
    // user wants to scroll". Same for zoom.
    bool checkForScroll(ui::MouseMessage* msg);
    bool checkForZoom(ui::MouseMessage* msg);

    // Start Scrolling/ZoomingState
    void startScrollingState(ui::MouseMessage* msg);
    void startZoomingState(ui::MouseMessage* msg);

    // Animation control
    void play(const bool playOnce,
              const bool playAll);
    void stop();
    bool isPlaying() const;

    // Shows a popup menu to change the editor animation speed.
    void showAnimationSpeedMultiplierPopup(Option<bool>& playOnce,
                                           Option<bool>& playAll,
                                           const bool withStopBehaviorOptions);
    double getAnimationSpeedMultiplier() const;
    void setAnimationSpeedMultiplier(double speed);

    // Functions to be used in EditorState::onSetCursor()
    void showMouseCursor(ui::CursorType cursorType,
                         const ui::Cursor* cursor = nullptr);
    void showBrushPreview(const gfx::Point& pos);

    // Gets the brush preview controller.
    BrushPreview& brushPreview() { return m_brushPreview; }

    static EditorRender& renderEngine() { return *m_renderEngine; }

    // IColorSource
    app::Color getColorByPosition(const gfx::Point& pos) override;

    void setTagFocusBand(int value) { m_tagFocusBand = value; }
    int tagFocusBand() const { return m_tagFocusBand; }

    // Returns true if the Shift key to draw straight lines with a
    // freehand tool is pressed.
    bool startStraightLineWithFreehandTool(const ui::MouseMessage* msg);

    // Functions to handle the set of selected slices.
    bool isSliceSelected(const doc::Slice* slice) const;
    void clearSlicesSelection();
    void selectSlice(const doc::Slice* slice);
    bool selectSliceBox(const gfx::Rect& box);
    void selectAllSlices();
    bool hasSelectedSlices() const { return !m_selectedSlices.empty(); }

    // Called by DocView's InputChainElement::onCancel() impl when Esc
    // key is pressed to cancel the active selection.
    void cancelSelections();

    static void registerCommands();

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    void onInvalidateRegion(const gfx::Region& region) override;
    void onFgColorChange();
    void onContextBarBrushChange();
    void onTiledModeBeforeChange();
    void onTiledModeChange();
    void onShowExtrasChange();

    // DocObserver impl
    void onColorSpaceChanged(DocEvent& ev) override;
    void onExposeSpritePixels(DocEvent& ev) override;
    void onSpritePixelRatioChanged(DocEvent& ev) override;
    void onBeforeRemoveLayer(DocEvent& ev) override;
    void onRemoveCel(DocEvent& ev) override;
    void onAddTag(DocEvent& ev) override;
    void onRemoveTag(DocEvent& ev) override;
    void onRemoveSlice(DocEvent& ev) override;

    // ActiveToolObserver impl
    void onActiveToolChange(tools::Tool* tool) override;

  private:
    enum class Flashing { None, WithFlashExtraCel, WaitingDeferedPaint };

    void setStateInternal(const EditorStatePtr& newState);
    void updateQuicktool();
    void updateToolByTipProximity(ui::PointerType pointerType);
    void updateToolLoopModifiersIndicators();

    void drawBackground(ui::Graphics* g);
    void drawSpriteUnclippedRect(ui::Graphics* g, const gfx::Rect& rc);
    void drawMaskSafe();
    void drawMask(ui::Graphics* g);
    void drawGrid(ui::Graphics* g, const gfx::Rect& spriteBounds, const gfx::Rect& gridBounds,
                  const app::Color& color, int alpha);
    void drawSlices(ui::Graphics* g);
    void drawCelBounds(ui::Graphics* g, const Cel* cel, const gfx::Color color);
    void drawCelGuides(ui::Graphics* g, const Cel* cel, const Cel* mouseCel);
    void drawCelHGuide(ui::Graphics* g,
                       const int sprX1, const int sprX2,
                       const int scrX1, const int scrX2, const int scrY,
                       const gfx::Rect& scrCelBounds, const gfx::Rect& scrCmpBounds,
                       const int dottedX);
    void drawCelVGuide(ui::Graphics* g,
                       const int sprY1, const int sprY2,
                       const int scrY1, const int scrY2, const int scrX,
                       const gfx::Rect& scrCelBounds, const gfx::Rect& scrCmpBounds,
                       const int dottedY);
    gfx::Rect getCelScreenBounds(const Cel* cel);

    void setCursor(const gfx::Point& mouseScreenPos);

    // Draws the specified portion of sprite in the editor.  Warning:
    // You should setup the clip of the screen before calling this
    // routine.
    void drawOneSpriteUnclippedRect(ui::Graphics* g, const gfx::Rect& rc, int dx, int dy);

    gfx::Point calcExtraPadding(const render::Projection& proj);

    void invalidateIfActive();
    bool showAutoCelGuides();
    void updateAutoCelGuides(ui::Message* msg);

    // Stack of states. The top element in the stack is the current state (m_state).
    EditorStatesHistory m_statesHistory;
    EditorStatesHistory m_deletedStates;

    // Current editor state (it can be shared between several editors to
    // the same document). This member cannot be NULL.
    EditorStatePtr m_state;

    // Current decorator (to draw extra UI elements).
    EditorDecorator* m_decorator;

    Doc* m_document;              // Active document in the editor
    Sprite* m_sprite;             // Active sprite in the editor
    Layer* m_layer;               // Active layer in the editor
    frame_t m_frame;              // Active frame in the editor
    render::Projection m_proj;    // Zoom/pixel ratio in the editor
    DocumentPreferences& m_docPref;

    // Brush preview
    BrushPreview m_brushPreview;

    tools::ToolLoopModifiers m_toolLoopModifiers;

    // Extra space around the sprite.
    gfx::Point m_padding;

    // Marching ants stuff
    ui::Timer m_antsTimer;
    int m_antsOffset;

    obs::scoped_connection m_fgColorChangeConn;
    obs::scoped_connection m_contextBarBrushChangeConn;
    obs::scoped_connection m_showExtrasConn;

    // Slots listeing document preferences.
    obs::scoped_connection m_tiledConnBefore;
    obs::scoped_connection m_tiledConn;
    obs::scoped_connection m_gridConn;
    obs::scoped_connection m_pixelGridConn;
    obs::scoped_connection m_bgConn;
    obs::scoped_connection m_onionskinConn;
    obs::scoped_connection m_symmetryModeConn;

    EditorObservers m_observers;

    EditorCustomizationDelegate* m_customizationDelegate;

    // TODO This field shouldn't be here. It should be removed when
    // editors.cpp are finally replaced with a fully funtional Workspace
    // widget.
    DocView* m_docView;

    gfx::Point m_oldPos;

    EditorFlags m_flags;

    bool m_secondaryButton;
    Flashing m_flashing;

    // Animation speed multiplier.
    double m_aniSpeed;
    bool m_isPlaying;

    // The Cel that is above the mouse if the Ctrl (or Cmd) key is
    // pressed (move key).
    Cel* m_showGuidesThisCel;

    // Focused tag band. Used by the Timeline to save/restore the
    // focused tag band for each sprite/editor.
    int m_tagFocusBand;

    // Used to restore scroll when the tiled mode is changed.
    // TODO could we avoid one extra field just to do this?
    gfx::Point m_oldMainTilePos;

#if ENABLE_DEVMODE
    gfx::Rect m_perfInfoBounds;
#endif

    // For slices
    doc::SelectedObjects m_selectedSlices;

    // The render engine must be shared between all editors so when a
    // DrawingState is being used in one editor, other editors for the
    // same document can show the same preview image/stroke being drawn
    // (search for Render::setPreviewImage()).
    static EditorRender* m_renderEngine;
  };

} // namespace app

#endif
