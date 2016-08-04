// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_TIMELINE_H_INCLUDED
#define APP_UI_TIMELINE_H_INCLUDED
#pragma once

#include "app/document_range.h"
#include "app/pref/preferences.h"
#include "app/ui/ani_controls.h"
#include "app/ui/editor/editor_observer.h"
#include "app/ui/input_chain_element.h"
#include "base/connection.h"
#include "doc/document_observer.h"
#include "doc/documents_observer.h"
#include "doc/frame.h"
#include "doc/layer_index.h"
#include "doc/sprite.h"
#include "ui/scroll_bar.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <vector>

namespace doc {
  class Cel;
  class Layer;
  class LayerImage;
  class Sprite;
}

namespace ui {
  class Graphics;
}

namespace app {

  namespace skin {
    class Style;
    class SkinTheme;
  }

  using namespace doc;

  class CommandExecutionEvent;
  class ConfigureTimelinePopup;
  class Context;
  class Document;
  class Editor;

  class Timeline : public ui::Widget
                 , public ui::ScrollableViewDelegate
                 , public doc::DocumentsObserver
                 , public doc::DocumentObserver
                 , public app::EditorObserver
                 , public app::InputChainElement {
  public:
    typedef DocumentRange Range;

    enum State {
      STATE_STANDBY,
      STATE_SCROLLING,
      STATE_SELECTING_LAYERS,
      STATE_SELECTING_FRAMES,
      STATE_SELECTING_CELS,
      STATE_MOVING_SEPARATOR,
      STATE_MOVING_RANGE,
      STATE_MOVING_ONIONSKIN_RANGE_LEFT,
      STATE_MOVING_ONIONSKIN_RANGE_RIGHT
    };

    enum DropOp { kMove, kCopy };

    Timeline();
    ~Timeline();

    void updateUsingEditor(Editor* editor);

    Sprite* sprite() { return m_sprite; }
    Layer* getLayer() { return m_layer; }
    frame_t getFrame() { return m_frame; }

    State getState() const { return m_state; }
    bool isMovingCel() const;

    Range range() const { return m_range; }

    void prepareToMoveRange();
    void moveRange(Range& range);

    void activateClipboardRange();

    // Drag-and-drop operations. These actions are used by commands
    // called from popup menus.
    void dropRange(DropOp op);

    // ScrollableViewDelegate impl
    gfx::Size visibleSize() const override;
    gfx::Point viewScroll() const override;
    void setViewScroll(const gfx::Point& pt) override;

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;

    // DocumentObserver impl.
    void onGeneralUpdate(DocumentEvent& ev) override;
    void onAddLayer(doc::DocumentEvent& ev) override;
    void onAfterRemoveLayer(doc::DocumentEvent& ev) override;
    void onAddFrame(doc::DocumentEvent& ev) override;
    void onRemoveFrame(doc::DocumentEvent& ev) override;
    void onSelectionChanged(doc::DocumentEvent& ev) override;
    void onLayerNameChange(doc::DocumentEvent& ev) override;

    // app::Context slots.
    void onAfterCommandExecution(CommandExecutionEvent& ev);

    // DocumentsObserver impl.
    void onRemoveDocument(doc::Document* document) override;

    // EditorObserver impl.
    void onStateChanged(Editor* editor) override;
    void onAfterFrameChanged(Editor* editor) override;
    void onAfterLayerChanged(Editor* editor) override;
    void onDestroyEditor(Editor* editor) override;

    // InputChainElement impl
    void onNewInputPriority(InputChainElement* element) override;
    bool onCanCut(Context* ctx) override;
    bool onCanCopy(Context* ctx) override;
    bool onCanPaste(Context* ctx) override;
    bool onCanClear(Context* ctx) override;
    bool onCut(Context* ctx) override;
    bool onCopy(Context* ctx) override;
    bool onPaste(Context* ctx) override;
    bool onClear(Context* ctx) override;
    void onCancel(Context* ctx) override;

  private:
    struct DrawCelData;

    struct Hit {
      int part;
      LayerIndex layer;
      frame_t frame;
      ObjectId frameTag;

      Hit(int part = 0, LayerIndex layer = LayerIndex(0), frame_t frame = 0, ObjectId frameTag = NullId)
        : part(part), layer(layer), frame(frame), frameTag(frameTag) {
      }

      bool operator!=(const Hit& other) const {
        return
          part != other.part ||
          layer != other.layer ||
          frame != other.frame ||
          frameTag != other.frameTag;
      }

      FrameTag* getFrameTag() const;
    };

    struct DropTarget {
      enum HHit { HNone, Before, After };
      enum VHit { VNone, Bottom, Top };

      DropTarget() {
        hhit = HNone;
        vhit = VNone;
      }

      HHit hhit;
      VHit vhit;
      Layer* layer;
      LayerIndex layerIdx;
      frame_t frame;
      int xpos, ypos;
    };

    void setLayer(Layer* layer);
    void setFrame(frame_t frame, bool byUser);
    bool allLayersVisible();
    bool allLayersInvisible();
    bool allLayersLocked();
    bool allLayersUnlocked();
    bool allLayersContinuous();
    bool allLayersDiscontinuous();
    void detachDocument();
    void setCursor(ui::Message* msg, const Hit& hit);
    void getDrawableLayers(ui::Graphics* g, LayerIndex* first_layer, LayerIndex* last_layer);
    void getDrawableFrames(ui::Graphics* g, frame_t* first_frame, frame_t* last_frame);
    void drawPart(ui::Graphics* g, const gfx::Rect& bounds,
      const char* text, skin::Style* style,
      bool is_active = false, bool is_hover = false, bool is_clicked = false);
    void drawTop(ui::Graphics* g);
    void drawHeader(ui::Graphics* g);
    void drawHeaderFrame(ui::Graphics* g, frame_t frame);
    void drawLayer(ui::Graphics* g, LayerIndex layerIdx);
    void drawCel(ui::Graphics* g, LayerIndex layerIdx, frame_t frame, Cel* cel, DrawCelData* data);
    void drawCelLinkDecorators(ui::Graphics* g, const gfx::Rect& bounds,
                               Cel* cel, frame_t frame, bool is_active, bool is_hover,
                               DrawCelData* data);
    void drawFrameTags(ui::Graphics* g);
    void drawRangeOutline(ui::Graphics* g);
    void drawPaddings(ui::Graphics* g);
    bool drawPart(ui::Graphics* g, int part, LayerIndex layer, frame_t frame);
    void drawClipboardRange(ui::Graphics* g);
    gfx::Rect getLayerHeadersBounds() const;
    gfx::Rect getFrameHeadersBounds() const;
    gfx::Rect getOnionskinFramesBounds() const;
    gfx::Rect getCelsBounds() const;
    gfx::Rect getPartBounds(const Hit& hit) const;
    gfx::Rect getRangeBounds(const Range& range) const;
    void invalidateHit(const Hit& hit);
    void regenerateLayers();
    void updateScrollBars();
    void updateByMousePos(ui::Message* msg, const gfx::Point& mousePos);
    Hit hitTest(ui::Message* msg, const gfx::Point& mousePos);
    Hit hitTestCel(const gfx::Point& mousePos);
    void setHot(const Hit& hit);
    void showCel(LayerIndex layer, frame_t frame);
    void showCurrentCel();
    void cleanClk();
    gfx::Size getScrollableSize() const;
    gfx::Point getMaxScrollablePos() const;
    LayerIndex getLayerIndex(const Layer* layer) const;
    bool isLayerActive(LayerIndex layerIdx) const;
    bool isFrameActive(frame_t frame) const;
    void updateStatusBar(ui::Message* msg);
    void updateDropRange(const gfx::Point& pt);
    void clearClipboardRange();

    bool isCopyKeyPressed(ui::Message* msg);

    // The layer of the bottom (e.g. Background layer)
    LayerIndex firstLayer() const { return LayerIndex(0); }

    // The layer of the top.
    LayerIndex lastLayer() const { return LayerIndex(m_layers.size()-1); }

    frame_t firstFrame() const { return frame_t(0); }
    frame_t lastFrame() const { return m_sprite->lastFrame(); }

    bool validLayer(LayerIndex layer) const { return layer >= firstLayer() && layer <= lastLayer(); }
    bool validFrame(frame_t frame) const { return frame >= firstFrame() && frame <= lastFrame(); }

    int topHeight() const;

    DocumentPreferences& docPref() const;

    // Theme/dimensions
    skin::SkinTheme* skinTheme() const;
    gfx::Size celBoxSize() const;
    int headerBoxHeight() const;
    int layerBoxHeight() const;
    int frameBoxWidth() const;
    int outlineWidth() const;

    void updateCelOverlayBounds(const Hit& hit);
    void drawCelOverlay(ui::Graphics* g);
    void onThumbnailsPrefChange();

    ui::ScrollBar m_hbar;
    ui::ScrollBar m_vbar;
    gfx::Rect m_viewportArea;
    Context* m_context;
    Editor* m_editor;
    Document* m_document;
    Sprite* m_sprite;
    Layer* m_layer;
    frame_t m_frame;
    Range m_range;
    Range m_dropRange;
    State m_state;
    std::vector<Layer*> m_layers;
    int m_separator_x;
    int m_separator_w;
    int m_origFrames;
    Hit m_hot;       // The 'hot' part is where the mouse is on top of
    DropTarget m_dropTarget;
    Hit m_clk; // The 'clk' part is where the mouse's button was pressed (maybe for a drag & drop operation)
    // Absolute mouse positions for scrolling.
    gfx::Point m_oldPos;
    // Configure timeline
    ConfigureTimelinePopup* m_confPopup;
    base::ScopedConnection m_ctxConn;

    // Marching ants stuff to show the range in the clipboard.
    // TODO merge this with the marching ants of the sprite editor (ui::Editor)
    ui::Timer m_clipboard_timer;
    int m_offset_count;

    bool m_scroll;   // True if the drag-and-drop operation is a scroll operation.
    bool m_copy;     // True if the drag-and-drop operation is a copy.
    bool m_fromTimeline;

    AniControls m_aniControls;

    // Data used for thumbnails.
    bool m_thumbnailsOverlayVisible;
    gfx::Rect m_thumbnailsOverlayInner;
    gfx::Rect m_thumbnailsOverlayOuter;
    Hit m_thumbnailsOverlayHit;
    gfx::Point m_thumbnailsOverlayDirection;
    base::Connection m_thumbnailsPrefConn;

    // Temporal data used to move the range.
    struct MoveRange {
      int activeRelativeLayer;
      frame_t activeRelativeFrame;
    } m_moveRangeData;
  };

} // namespace app

#endif
