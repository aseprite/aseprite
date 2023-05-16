// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TIMELINE_TIMELINE_H_INCLUDED
#define APP_UI_TIMELINE_TIMELINE_H_INCLUDED
#pragma once

#include "app/doc_observer.h"
#include "app/doc_range.h"
#include "app/docs_observer.h"
#include "app/loop_tag.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor_observer.h"
#include "app/ui/input_chain_element.h"
#include "app/ui/timeline/ani_controls.h"
#include "app/ui/timeline/timeline_observer.h"
#include "base/debug.h"
#include "doc/frame.h"
#include "doc/layer.h"
#include "doc/object_version.h"
#include "doc/selected_frames.h"
#include "doc/selected_layers.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "gfx/color.h"
#include "obs/connection.h"
#include "obs/observable.h"
#include "ui/scroll_bar.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <memory>
#include <vector>

namespace doc {
  class Cel;
  class Layer;
  class LayerImage;
  class Sprite;
}

namespace ui {
  class Graphics;
  class TooltipManager;
}

namespace app {

  namespace skin {
    class SkinTheme;
  }

  using namespace doc;

  class CommandExecutionEvent;
  class ConfigureTimelinePopup;
  class Context;
  class Doc;
  class Editor;

  class Timeline : public ui::Widget,
                   public ui::ScrollableViewDelegate,
                   public obs::observable<TimelineObserver>,
                   public ContextObserver,
                   public DocsObserver,
                   public DocObserver,
                   public EditorObserver,
                   public InputChainElement,
                   public TagProvider {
  public:
    typedef DocRange Range;

    enum State {
      STATE_STANDBY,
      STATE_SCROLLING,
      STATE_SELECTING_LAYERS,
      STATE_SELECTING_FRAMES,
      STATE_SELECTING_CELS,
      STATE_MOVING_SEPARATOR,
      STATE_MOVING_RANGE,
      STATE_MOVING_ONIONSKIN_RANGE_LEFT,
      STATE_MOVING_ONIONSKIN_RANGE_RIGHT,
      STATE_MOVING_TAG,
      STATE_RESIZING_TAG_LEFT,
      STATE_RESIZING_TAG_RIGHT,
      // Changing layers flags states
      STATE_SHOWING_LAYERS,
      STATE_HIDING_LAYERS,
      STATE_LOCKING_LAYERS,
      STATE_UNLOCKING_LAYERS,
      STATE_ENABLING_CONTINUOUS_LAYERS,
      STATE_DISABLING_CONTINUOUS_LAYERS,
      STATE_EXPANDING_LAYERS,
      STATE_COLLAPSING_LAYERS,
    };

    enum DropOp { kMove, kCopy };

    Timeline(ui::TooltipManager* tooltipManager);
    ~Timeline();

    void updateUsingEditor(Editor* editor);

    Sprite* sprite() { return m_sprite; }

    bool isMovingCel() const;

    Range range() const { return m_range; }
    const SelectedLayers& selectedLayers() const { return m_range.selectedLayers(); }
    const SelectedFrames& selectedFrames() const { return m_range.selectedFrames(); }

    void prepareToMoveRange();
    void moveRange(const Range& range);
    void setRange(const Range& range);

    void activateClipboardRange();

    // Drag-and-drop operations. These actions are used by commands
    // called from popup menus.
    void dropRange(DropOp op);

    // TagProvider impl
    // Returns the active frame tag depending on the timeline status
    // E.g. if other frame tags are collapsed, the focused band has
    // priority and tags in other bands are ignored.
    Tag* getTagByFrame(const frame_t frame,
                       const bool getLoopTagIfNone) override;

    // ScrollableViewDelegate impl
    gfx::Size visibleSize() const override;
    gfx::Point viewScroll() const override;
    void setViewScroll(const gfx::Point& pt) override;

    void lockRange();
    void unlockRange();

    void clearAndInvalidateRange();

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onInitTheme(ui::InitThemeEvent& ev) override;
    void onInvalidateRegion(const gfx::Region& region) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;

    // DocObserver impl.
    void onGeneralUpdate(DocEvent& ev) override;
    void onAddLayer(DocEvent& ev) override;
    void onBeforeRemoveLayer(DocEvent& ev) override;
    void onAfterRemoveLayer(DocEvent& ev) override;
    void onAddFrame(DocEvent& ev) override;
    void onRemoveFrame(DocEvent& ev) override;
    void onAddCel(DocEvent& ev) override;
    void onAfterRemoveCel(DocEvent& ev) override;
    void onLayerNameChange(DocEvent& ev) override;
    void onAddTag(DocEvent& ev) override;
    void onRemoveTag(DocEvent& ev) override;
    void onTagChange(DocEvent& ev) override;
    void onTagRename(DocEvent& ev) override;
    void onLayerCollapsedChanged(DocEvent& ev) override;

    // app::Context slots.
    void onBeforeCommandExecution(CommandExecutionEvent& ev);
    void onAfterCommandExecution(CommandExecutionEvent& ev);

    // ContextObserver impl
    void onActiveSiteChange(const Site& site) override;

    // DocsObserver impl.
    void onRemoveDocument(Doc* document) override;

    // EditorObserver impl.
    void onStateChanged(Editor* editor) override;
    void onAfterFrameChanged(Editor* editor) override;
    void onAfterLayerChanged(Editor* editor) override;
    void onDestroyEditor(Editor* editor) override;

    // InputChainElement impl
    void onNewInputPriority(InputChainElement* element,
                            const ui::Message* msg) override;
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
      layer_t layer;
      frame_t frame;
      ObjectId tag;
      bool veryBottom;
      int band;

      Hit(int part = 0,
          layer_t layer = -1,
          frame_t frame = 0,
          ObjectId tag = NullId,
          int band = -1);
      bool operator!=(const Hit& other) const;
      Tag* getTag() const;
    };

    struct DropTarget {
      enum HHit {
        HNone,
        Before,
        After
      };
      enum VHit {
        VNone,
        Bottom,
        Top,
        FirstChild,
        VeryBottom
      };
      DropTarget();
      DropTarget(const DropTarget& o);
      bool operator!=(const DropTarget& o) const {
        return (hhit != o.hhit ||
                vhit != o.vhit ||
                outside != o.outside);
      }
      HHit hhit;
      VHit vhit;
      bool outside;
    };

    struct Row {
      Row();
      Row(Layer* layer,
          const int level,
          const LayerFlags inheritedFlags);

      Layer* layer() const { return m_layer; }
      int level() const { return m_level; }

      bool parentVisible() const;
      bool parentEditable() const;

    private:
      Layer* m_layer;
      int m_level;
      LayerFlags m_inheritedFlags;
    };

    bool selectedLayersBounds(const SelectedLayers& layers,
                              layer_t* first, layer_t* last) const;

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
    void getDrawableLayers(layer_t* firstLayer, layer_t* lastLayer);
    void getDrawableFrames(frame_t* firstFrame, frame_t* lastFrame);
    void drawPart(ui::Graphics* g, const gfx::Rect& bounds,
                  const std::string* text,
                  ui::Style* style,
                  const bool is_active = false,
                  const bool is_hover = false,
                  const bool is_clicked = false,
                  const bool is_disabled = false);
    void drawTop(ui::Graphics* g);
    void drawHeader(ui::Graphics* g);
    void drawHeaderFrame(ui::Graphics* g, const frame_t frame);
    void drawLayer(ui::Graphics* g, const layer_t layerIdx);
    void drawCel(ui::Graphics* g,
                 const layer_t layerIdx, const frame_t frame,
                 const Cel* cel, const DrawCelData* data);
    void drawCelLinkDecorators(ui::Graphics* g, const gfx::Rect& bounds,
                               const Cel* cel, const frame_t frame,
                               const bool is_active, const bool is_hover,
                               const DrawCelData* data);
    void drawTags(ui::Graphics* g);
    void drawTagBraces(ui::Graphics* g,
                       gfx::Color tagColor,
                       const gfx::Rect& bounds,
                       const gfx::Rect& clipBounds);
    void drawRangeOutline(ui::Graphics* g);
    void drawPaddings(ui::Graphics* g);
    bool drawPart(ui::Graphics* g, int part, layer_t layer, frame_t frame);
    void drawClipboardRange(ui::Graphics* g);
    gfx::Rect getLayerHeadersBounds() const;
    gfx::Rect getFrameHeadersBounds() const;
    gfx::Rect getOnionskinFramesBounds() const;
    gfx::Rect getCelsBounds() const;
    gfx::Rect getPartBounds(const Hit& hit) const;
    gfx::Rect getRangeBounds(const Range& range) const;
    gfx::Rect getRangeClipBounds(const Range& range) const;
    void invalidateHit(const Hit& hit);
    void invalidateLayer(const Layer* layer);
    void invalidateFrame(const frame_t frame);
    void invalidateRange();
    void regenerateRows();
    void regenerateTagBands();
    int visibleTagBands() const;
    void updateScrollBars();
    void updateByMousePos(ui::Message* msg, const gfx::Point& mousePos);
    Hit hitTest(ui::Message* msg, const gfx::Point& mousePos);
    Hit hitTestCel(const gfx::Point& mousePos);
    void setHot(const Hit& hit);
    void showCel(layer_t layer, frame_t frame);
    void showCurrentCel();
    void focusTagBand(int band);
    void cleanClk();
    gfx::Size getScrollableSize() const;
    gfx::Point getMaxScrollablePos() const;
    doc::Layer* getLayer(int layerIndex) const;
    layer_t getLayerIndex(const Layer* layer) const;
    bool isLayerActive(const layer_t layerIdx) const;
    bool isFrameActive(const frame_t frame) const;
    bool isCelActive(const layer_t layerIdx, const frame_t frame) const;
    bool isCelLooselyActive(const layer_t layerIdx, const frame_t frame) const;
    void updateStatusBar(ui::Message* msg);
    void updateStatusBarForFrame(const frame_t frame,
                                 const Tag* tag,
                                 const Cel* cel);
    void updateDropRange(const gfx::Point& pt);
    void clearClipboardRange();

    // The layer of the bottom (e.g. Background layer)
    layer_t firstLayer() const { return 0; }
    // The layer of the top.
    layer_t lastLayer() const { return m_rows.size()-1; }

    frame_t firstFrame() const { return frame_t(0); }
    frame_t lastFrame() const { return m_sprite->lastFrame(); }

    bool validLayer(layer_t layer) const { return layer >= firstLayer() && layer <= lastLayer(); }
    bool validFrame(frame_t frame) const { return frame >= firstFrame() && frame <= lastFrame(); }

    int topHeight() const;

    DocumentPreferences& docPref() const;

    // Theme/dimensions
    skin::SkinTheme* skinTheme() const;
    int headerBoxWidth() const;
    int headerBoxHeight() const;
    int layerBoxHeight() const;
    int frameBoxWidth() const;
    int outlineWidth() const;
    int oneTagHeight() const;
    int calcTagVisibleToFrame(Tag* tag) const;

    void updateCelOverlayBounds(const Hit& hit);
    void drawCelOverlay(ui::Graphics* g);
    void onThumbnailsPrefChange();
    void setZoom(const double zoom);
    void setZoomAndUpdate(const double zoom,
                          const bool updatePref);

    double zoom() const;
    int tagFramesDuration(const Tag* tag) const;
    // Calculate the duration of the selected range of frames
    int selectedFramesDuration() const;

    void setLayerVisibleFlag(const layer_t layer, const bool state);
    void setLayerEditableFlag(const layer_t layer, const bool state);
    void setLayerContinuousFlag(const layer_t layer, const bool state);
    void setLayerCollapsedFlag(const layer_t layer, const bool state);

    int separatorX() const;
    void setSeparatorX(int newValue);

    static gfx::Color highlightColor(const gfx::Color color);

    ui::ScrollBar m_hbar;
    ui::ScrollBar m_vbar;
    gfx::Rect m_viewportArea;
    double m_zoom;
    Context* m_context;
    Editor* m_editor;
    Doc* m_document;
    Sprite* m_sprite;
    Layer* m_layer;
    frame_t m_frame;
    int m_rangeLocks;
    Range m_range;
    Range m_startRange;
    Range m_dropRange;
    State m_state;

    // Version of the sprite before executing a command. Used to check
    // if the sprite was modified after executing a command to avoid
    // regenerating all rows if it's not necessary.
    doc::ObjectVersion m_savedVersion;

    // Data used to display each row in the timeline
    std::vector<Row> m_rows;

    // Data used to display frame tags
    int m_tagBands;
    int m_tagFocusBand;
    std::map<Tag*, int> m_tagBand;

    int m_separator_x;
    int m_separator_w;
    int m_origFrames;
    Hit m_hot;       // The 'hot' part is where the mouse is on top of
    DropTarget m_dropTarget;
    Hit m_clk; // The 'clk' part is where the mouse's button was pressed (maybe for a drag & drop operation)
    // Absolute mouse positions for scrolling.
    gfx::Point m_oldPos;
    // Configure timeline
    std::unique_ptr<ConfigureTimelinePopup> m_confPopup;
    obs::scoped_connection m_ctxConn1, m_ctxConn2;
    obs::connection m_firstFrameConn;
    obs::connection m_onionskinConn;

    // Marching ants stuff to show the range in the clipboard.
    // TODO merge this with the marching ants of the sprite editor (ui::Editor)
    ui::Timer m_clipboard_timer;
    int m_offset_count;
    bool m_redrawMarchingAntsOnly;

    bool m_scroll;   // True if the drag-and-drop operation is a scroll operation.
    bool m_copy;     // True if the drag-and-drop operation is a copy.
    bool m_fromTimeline;

    AniControls m_aniControls;

    // Data used for thumbnails.
    bool m_thumbnailsOverlayVisible;
    gfx::Rect m_thumbnailsOverlayBounds;
    Hit m_thumbnailsOverlayHit;
    gfx::Point m_thumbnailsOverlayDirection;
    obs::connection m_thumbnailsPrefConn;

    // Temporal data used to move the range.
    struct MoveRange {
      layer_t activeRelativeLayer;
      frame_t activeRelativeFrame;
    } m_moveRangeData;

    // Temporal data used to move tags.
    struct ResizeTag {
      doc::ObjectId tag = doc::NullId;
      doc::frame_t from, to;
      void reset() {
        tag = doc::NullId;
      }
      void reset(const doc::ObjectId tagId) {
        auto tag = doc::get<doc::Tag>(tagId);
        if (tag) {
          this->tag = tagId;
          this->from = tag->fromFrame();
          this->to = tag->toFrame();
        }
        else {
          this->tag = doc::NullId;
        }
      }
    } m_resizeTagData;
  };

#ifdef ENABLE_UI
  class LockTimelineRange {
  public:
    LockTimelineRange(Timeline* timeline)
      : m_timeline(timeline) {
      if (m_timeline)
        m_timeline->lockRange();
    }
    ~LockTimelineRange() {
      if (m_timeline)
        m_timeline->unlockRange();
    }
  private:
    Timeline* m_timeline;
  };
#else  // !ENABLE_UI
  class LockTimelineRange {
  public:
    LockTimelineRange(Timeline* timeline) { }
  };
#endif

} // namespace app

#endif
