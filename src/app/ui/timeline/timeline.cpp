// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/timeline/timeline.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/cmd/set_tag_range.h"
#include "app/cmd_transaction.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/doc_api.h"
#include "app/doc_event.h"
#include "app/doc_range_ops.h"
#include "app/doc_undo.h"
#include "app/i18n/strings.h"
#include "app/loop_tag.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/thumbnails.h"
#include "app/transaction.h"
#include "app/tx.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/configure_timeline_popup.h"
#include "app/ui/doc_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/input_chain.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "app/util/layer_boundaries.h"
#include "app/util/layer_utils.h"
#include "app/util/readable_time.h"
#include "base/convert_to.h"
#include "base/memory.h"
#include "base/scoped_value.h"
#include "doc/doc.h"
#include "fmt/format.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "os/font.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/ui.h"

#include <algorithm>
#include <cstdio>
#include <vector>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace doc;
using namespace ui;

enum {
  PART_NOTHING = 0,
  PART_TOP,
  PART_SEPARATOR,
  PART_HEADER_EYE,
  PART_HEADER_PADLOCK,
  PART_HEADER_CONTINUOUS,
  PART_HEADER_GEAR,
  PART_HEADER_ONIONSKIN,
  PART_HEADER_ONIONSKIN_RANGE_LEFT,
  PART_HEADER_ONIONSKIN_RANGE_RIGHT,
  PART_HEADER_LAYER,
  PART_HEADER_FRAME,
  PART_ROW,
  PART_ROW_EYE_ICON,
  PART_ROW_PADLOCK_ICON,
  PART_ROW_CONTINUOUS_ICON,
  PART_ROW_TEXT,
  PART_CEL,
  PART_RANGE_OUTLINE,
  PART_TAG,
  PART_TAG_LEFT,
  PART_TAG_RIGHT,
  PART_TAGS,
  PART_TAG_BAND,
  PART_TAG_SWITCH_BUTTONS,
  PART_TAG_SWITCH_BAND_BUTTON,
};

struct Timeline::DrawCelData {
  CelIterator begin;
  CelIterator end;
  CelIterator it;
  CelIterator prevIt;           // Previous Cel to "it"
  CelIterator nextIt;           // Next Cel to "it"
  CelIterator activeIt;         // Active Cel iterator
  CelIterator firstLink;        // First link to the active cel
  CelIterator lastLink;         // Last link to the active cel
};

namespace {

  template<typename Pred>
  void for_each_expanded_layer(LayerGroup* group,
                               Pred&& pred,
                               int level = 0,
                               LayerFlags flags =
                                 LayerFlags(int(LayerFlags::Visible) |
                                            int(LayerFlags::Editable))) {
    if (!group->isVisible())
      flags = static_cast<LayerFlags>(int(flags) & ~int(LayerFlags::Visible));

    if (!group->isEditable())
      flags = static_cast<LayerFlags>(int(flags) & ~int(LayerFlags::Editable));

    for (Layer* child : group->layers()) {
      if (child->isGroup() && !child->isCollapsed())
        for_each_expanded_layer<Pred>(
          static_cast<LayerGroup*>(child),
          std::forward<Pred>(pred),
          level+1,
          flags);

      pred(child, level, flags);
    }
  }

  bool is_copy_key_pressed(ui::Message* msg) {
    return
      msg->ctrlPressed() ||  // Ctrl is common on Windows
      msg->altPressed();    // Alt is common on Mac OS X
  }

  bool is_select_layer_in_canvas_key_pressed(ui::Message* msg) {
#ifdef __APPLE__
    return msg->cmdPressed();
#else
    return msg->ctrlPressed();
#endif
  }

  SelectLayerBoundariesOp get_select_layer_in_canvas_op(ui::Message* msg) {
    if (msg->altPressed() && msg->shiftPressed())
      return SelectLayerBoundariesOp::INTERSECT;
    else if (msg->shiftPressed())
      return SelectLayerBoundariesOp::ADD;
    else if (msg->altPressed())
      return SelectLayerBoundariesOp::SUBTRACT;
    else
      return SelectLayerBoundariesOp::REPLACE;
  }

} // anonymous namespace

Timeline::Hit::Hit(int part,
                   layer_t layer,
                   frame_t frame,
                   ObjectId tag,
                   int band)
  : part(part),
    layer(layer),
    frame(frame),
    tag(tag),
    veryBottom(false),
    band(band)
{
}

bool Timeline::Hit::operator!=(const Hit& other) const
{
  return
    part != other.part ||
    layer != other.layer ||
    frame != other.frame ||
    tag != other.tag ||
    band != other.band;
}

Tag* Timeline::Hit::getTag() const
{
  return get<Tag>(tag);
}

Timeline::DropTarget::DropTarget()
{
  hhit = HNone;
  vhit = VNone;
  outside = false;
}

Timeline::DropTarget::DropTarget(const DropTarget& o)
  : hhit(o.hhit)
  , vhit(o.vhit)
  , outside(o.outside)
{
}

Timeline::Row::Row()
  : m_layer(nullptr),
    m_level(0),
    m_inheritedFlags(LayerFlags::None)
{
}

Timeline::Row::Row(Layer* layer,
                   const int level,
                   const LayerFlags inheritedFlags)
  : m_layer(layer),
    m_level(level),
    m_inheritedFlags(inheritedFlags)
{
}

bool Timeline::Row::parentVisible() const
{
  return ((int(m_inheritedFlags) & int(LayerFlags::Visible)) != 0);
}

bool Timeline::Row::parentEditable() const
{
  return ((int(m_inheritedFlags) & int(LayerFlags::Editable)) != 0);
}

Timeline::Timeline(TooltipManager* tooltipManager)
  : Widget(kGenericWidget)
  , m_hbar(HORIZONTAL, this)
  , m_vbar(VERTICAL, this)
  , m_zoom(1.0)
  , m_context(UIContext::instance())
  , m_editor(NULL)
  , m_document(NULL)
  , m_sprite(NULL)
  , m_rangeLocks(0)
  , m_state(STATE_STANDBY)
  , m_tagBands(0)
  , m_tagFocusBand(-1)
  , m_separator_x(
      Preferences::instance().general.timelineLayerPanelWidth() * guiscale())
  , m_separator_w(1)
  , m_confPopup(nullptr)
  , m_clipboard_timer(100, this)
  , m_offset_count(0)
  , m_redrawMarchingAntsOnly(false)
  , m_scroll(false)
  , m_fromTimeline(false)
  , m_aniControls(tooltipManager)
{
  enableFlags(CTRL_RIGHT_CLICK);

  m_ctxConn1 = m_context->BeforeCommandExecution.connect(
    &Timeline::onBeforeCommandExecution, this);
  m_ctxConn2 = m_context->AfterCommandExecution.connect(
    &Timeline::onAfterCommandExecution, this);
  m_context->documents().add_observer(this);
  m_context->add_observer(this);

  setDoubleBuffered(true);
  addChild(&m_aniControls);
  addChild(&m_hbar);
  addChild(&m_vbar);

  m_hbar.setTransparent(true);
  m_vbar.setTransparent(true);
  initTheme();
}

Timeline::~Timeline()
{
  Preferences::instance().general.timelineLayerPanelWidth(
    m_separator_x / guiscale());

  m_clipboard_timer.stop();

  detachDocument();
  m_context->documents().remove_observer(this);
  m_context->remove_observer(this);
  m_confPopup.reset();
}

void Timeline::setZoom(const double zoom)
{
  m_zoom = std::clamp(zoom, 1.0, 10.0);
  m_thumbnailsOverlayDirection = gfx::Point(int(frameBoxWidth()*1.0), int(frameBoxWidth()*0.5));
  m_thumbnailsOverlayVisible = false;
}

void Timeline::setZoomAndUpdate(const double zoom,
                                const bool updatePref)
{
  if (zoom != m_zoom) {
    setZoom(zoom);
    regenerateTagBands();
    updateScrollBars();
    invalidate();
  }
  if (updatePref && zoom != docPref().thumbnails.zoom()) {
    docPref().thumbnails.zoom(zoom);
    docPref().thumbnails.enabled(zoom > 1);
  }
}

void Timeline::onThumbnailsPrefChange()
{
  setZoomAndUpdate(
    docPref().thumbnails.enabled() ?
    docPref().thumbnails.zoom(): 1.0,
    false);
}

void Timeline::updateUsingEditor(Editor* editor)
{
  // TODO if editor == m_editor, avoid doing a lot of extra work here

  m_aniControls.updateUsingEditor(editor);

  DocRange oldRange;
  if (editor != m_editor) {
    // Save active m_tagFocusBand into the old focused editor
    if (m_editor)
      m_editor->setTagFocusBand(m_tagFocusBand);
    m_tagFocusBand = -1;
  }
  else {
    oldRange = m_range;
  }

  detachDocument();

  if (Preferences::instance().timeline.keepSelection())
    m_range = oldRange;
  else {
    // The range is reset in detachDocument()
    ASSERT(!m_range.enabled());
  }

  // We always update the editor. In this way the timeline keeps in
  // sync with the active editor.
  m_editor = editor;
  if (!m_editor)
    return;                // No editor specified.

  m_editor->add_observer(this);
  m_tagFocusBand = m_editor->tagFocusBand();

  Site site;
  DocView* view = m_editor->getDocView();
  view->getSite(&site);

  site.document()->add_observer(this);

  Doc* app_document = site.document();
  DocumentPreferences& docPref = Preferences::instance().document(app_document);

  m_thumbnailsPrefConn = docPref.thumbnails.AfterChange.connect(
    [this]{ onThumbnailsPrefChange(); });

  setZoom(
    docPref.thumbnails.enabled() ?
    docPref.thumbnails.zoom(): 1.0);

  // If we are already in the same position as the "editor", we don't
  // need to update the at all timeline.
  if (m_document == site.document() &&
      m_sprite == site.sprite() &&
      m_layer == site.layer() &&
      m_frame == site.frame())
    return;

  m_document = site.document();
  m_sprite = site.sprite();
  m_layer = site.layer();
  m_frame = site.frame();
  m_state = STATE_STANDBY;
  m_hot.part = PART_NOTHING;
  m_clk.part = PART_NOTHING;

  m_firstFrameConn = Preferences::instance().document(m_document)
    .timeline.firstFrame.AfterChange.connect([this]{ invalidate(); });

  setFocusStop(true);
  regenerateRows();
  setViewScroll(view->timelineScroll());
  showCurrentCel();
}

void Timeline::detachDocument()
{
  if (m_confPopup && m_confPopup->isVisible())
    m_confPopup->closeWindow(nullptr);

  m_firstFrameConn.disconnect();

  if (m_document) {
    m_thumbnailsPrefConn.disconnect();
    m_document->remove_observer(this);
    m_document = nullptr;
  }

  // Reset all pointers to this document, even DocRanges, we don't
  // want to store a pointer to a layer of a document that we are not
  // observing anymore (because the document might be deleted soon).
  m_sprite = nullptr;
  m_layer = nullptr;
  m_range.clearRange();
  m_startRange.clearRange();
  m_dropRange.clearRange();

  if (m_editor) {
    if (DocView* view = m_editor->getDocView())
      view->setTimelineScroll(viewScroll());

    m_editor->remove_observer(this);
    m_editor = nullptr;
  }

  invalidate();
}

bool Timeline::isMovingCel() const
{
  return (m_state == STATE_MOVING_RANGE &&
          m_range.type() == Range::kCels);
}

bool Timeline::selectedLayersBounds(const SelectedLayers& layers,
                                    layer_t* first, layer_t* last) const
{
  if (layers.empty())
    return false;

  *first = *last = getLayerIndex(*layers.begin());

  for (auto layer : layers) {
    layer_t i = getLayerIndex(layer);
    if (*first > i) *first = i;
    if (*last < i) *last = i;
  }

  return true;
}

void Timeline::setLayer(Layer* layer)
{
  ASSERT(m_editor != NULL);

  invalidateLayer(m_layer);
  invalidateLayer(layer);

  m_layer = layer;

  // Expand all parents
  if (m_layer) {
    LayerGroup* group = m_layer->parent();
    while (group != m_layer->sprite()->root()) {
      // Expand this group
      group->setCollapsed(false);
      group = group->parent();
    }
    regenerateRows();
    invalidate();
  }

  if (m_editor->layer() != layer)
    m_editor->setLayer(m_layer);
}

void Timeline::setFrame(frame_t frame, bool byUser)
{
  ASSERT(m_editor != NULL);
  // ASSERT(frame >= 0 && frame < m_sprite->totalFrames());

  if (frame < 0)
    frame = firstFrame();
  else if (frame >= m_sprite->totalFrames())
    frame = frame_t(m_sprite->totalFrames()-1);

  if (m_layer) {
    Cel* oldCel = m_layer->cel(m_frame);
    Cel* newCel = m_layer->cel(frame);
    std::size_t oldLinks = (oldCel ? oldCel->links(): 0);
    std::size_t newLinks = (newCel ? newCel->links(): 0);
    if ((oldLinks && !newCel) ||
        (newLinks && !oldCel) ||
        ((oldLinks || newLinks) && (oldCel->data() != newCel->data())))
      invalidateLayer(m_layer);
  }

  invalidateFrame(m_frame);
  invalidateFrame(frame);

  gfx::Rect onionRc = getOnionskinFramesBounds();

  m_frame = frame;

  // Invalidate the onionskin handles area
  onionRc |= getOnionskinFramesBounds();
  if (!onionRc.isEmpty())
    invalidateRect(onionRc.offset(origin()));

  if (m_editor->frame() != frame) {
    const bool isPlaying = m_editor->isPlaying();

    if (isPlaying)
      m_editor->stop();

    m_editor->setFrame(m_frame);

    if (isPlaying)
      m_editor->play(false,
                     Preferences::instance().editor.playAll(),
                     Preferences::instance().editor.playSubtags());
  }
}

void Timeline::prepareToMoveRange()
{
  ASSERT(m_range.enabled());

  layer_t i = 0;
  for (auto layer : m_range.selectedLayers().toBrowsableLayerList()) {
    if (layer == m_layer)
      break;
    ++i;
  }

  frame_t j = 0;
  for (auto frame : m_range.selectedFrames()) {
    if (frame == m_frame)
      break;
    ++j;
  }

  m_moveRangeData.activeRelativeLayer = i;
  m_moveRangeData.activeRelativeFrame = j;
}

void Timeline::moveRange(const Range& range)
{
  regenerateRows();

  // We have to change the range before we generate an
  // onActiveSiteChange() event so observers (like cel properties
  // dialog) know the new selected range.
  m_range = range;

  layer_t i = 0;
  for (auto layer : range.selectedLayers().toBrowsableLayerList()) {
    if (i == m_moveRangeData.activeRelativeLayer) {
      setLayer(layer);
      break;
    }
    ++i;
  }

  frame_t j = 0;
  for (auto frame : range.selectedFrames()) {
    if (j == m_moveRangeData.activeRelativeFrame) {
      setFrame(frame, true);
      break;
    }
    ++j;
  }

  // Select the range again (it might be lost between all the
  // setLayer()/setFrame() calls).
  m_range = range;
}

void Timeline::setRange(const Range& range)
{
  m_range = range;
  invalidate();
}

void Timeline::activateClipboardRange()
{
  m_clipboard_timer.start();
  invalidate();
}

Tag* Timeline::getTagByFrame(const frame_t frame,
                             const bool getLoopTagIfNone)
{
  if (!m_sprite)
    return nullptr;

  if (m_tagFocusBand < 0) {
    Tag* tag = get_animation_tag(m_sprite, frame);
    if (!tag && getLoopTagIfNone)
      tag = get_loop_tag(m_sprite);
    return tag;
  }

  for (Tag* tag : m_sprite->tags()) {
    if (frame >= tag->fromFrame() &&
        frame <= tag->toFrame() &&
        m_tagBand[tag] == m_tagFocusBand) {
      return tag;
    }
  }

  return nullptr;
}

bool Timeline::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kFocusEnterMessage:
      App::instance()->inputChain().prioritize(this, msg);
      break;

    case kTimerMessage:
      if (static_cast<TimerMessage*>(msg)->timer() == &m_clipboard_timer) {
        Doc* clipboard_document;
        DocRange clipboard_range;
        Clipboard::instance()->getDocumentRangeInfo(
          &clipboard_document,
          &clipboard_range);

        if (isVisible() &&
            m_document &&
            m_document == clipboard_document) {
          // Set offset to make selection-movement effect
          if (m_offset_count < 7)
            m_offset_count++;
          else
            m_offset_count = 0;

          bool redrawOnlyMarchingAnts = getUpdateRegion().isEmpty();
          invalidateRect(gfx::Rect(getRangeBounds(clipboard_range)).offset(origin()));
          if (redrawOnlyMarchingAnts)
            m_redrawMarchingAntsOnly = true;
        }
        else if (m_clipboard_timer.isRunning()) {
          m_clipboard_timer.stop();
        }
      }
      break;

    case kMouseDownMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

      if (!m_document)
        break;

      if (mouseMsg->middle() ||
          os::instance()->isKeyPressed(kKeySpace)) {
        captureMouse();
        m_state = STATE_SCROLLING;
        m_oldPos = static_cast<MouseMessage*>(msg)->position();
        return true;
      }

      // As we can ctrl+click color bar + timeline, now we have to
      // re-prioritize timeline on each click.
      App::instance()->inputChain().prioritize(this, msg);

      // Update hot part (as the user might have left clicked with
      // Ctrl on OS X, which it's converted to a right-click and it's
      // interpreted as other action by the Timeline::hitTest())
      setHot(hitTest(msg, mouseMsg->position() - bounds().origin()));

      // Clicked-part = hot-part.
      m_clk = m_hot;

      // With Ctrl+click (Win/Linux) or Shift+click (OS X) we can
      // select non-adjacents layer/frame ranges
      bool clearRange =
#if !defined(__APPLE__)
        !msg->ctrlPressed() &&
#endif
        !msg->shiftPressed();

      captureMouse();

      switch (m_hot.part) {

        case PART_SEPARATOR:
          m_state = STATE_MOVING_SEPARATOR;
          break;

        case PART_HEADER_EYE: {
          ASSERT(m_sprite);
          if (!m_sprite)
            break;

          bool regenRows = false;
          bool newVisibleState = !allLayersVisible();
          for (Layer* topLayer : m_sprite->root()->layers()) {
            if (topLayer->isVisible() != newVisibleState) {
              topLayer->setVisible(newVisibleState);
              if (topLayer->isGroup())
                regenRows = true;
            }
          }

          if (regenRows) {
            regenerateRows();
            invalidate();
          }

          // Redraw all views.
          m_document->notifyGeneralUpdate();
          break;
        }

        case PART_HEADER_PADLOCK: {
          ASSERT(m_sprite);
          if (!m_sprite)
            break;

          bool regenRows = false;
          bool newEditableState = !allLayersUnlocked();
          for (Layer* topLayer : m_sprite->root()->layers()) {
            if (topLayer->isEditable() != newEditableState) {
              topLayer->setEditable(newEditableState);
              if (topLayer->isGroup()) {
                regenRows = true;
              }
            }
          }

          if (regenRows) {
            regenerateRows();
            invalidate();
          }
          break;
        }

        case PART_HEADER_CONTINUOUS: {
          bool newContinuousState = !allLayersContinuous();
          for (size_t i=0; i<m_rows.size(); i++)
            m_rows[i].layer()->setContinuous(newContinuousState);
          invalidate();
          break;
        }

        case PART_HEADER_ONIONSKIN: {
          docPref().onionskin.active(!docPref().onionskin.active());
          invalidate();
          break;
        }
        case PART_HEADER_ONIONSKIN_RANGE_LEFT: {
          m_state = STATE_MOVING_ONIONSKIN_RANGE_LEFT;
          m_origFrames = docPref().onionskin.prevFrames();
          break;
        }
        case PART_HEADER_ONIONSKIN_RANGE_RIGHT: {
          m_state = STATE_MOVING_ONIONSKIN_RANGE_RIGHT;
          m_origFrames = docPref().onionskin.nextFrames();
          break;
        }
        case PART_HEADER_FRAME: {
          bool selectFrame = (mouseMsg->left() || !isFrameActive(m_clk.frame));

          if (selectFrame) {
            m_state = STATE_SELECTING_FRAMES;
            if (clearRange)
              clearAndInvalidateRange();
            m_range.startRange(m_layer, m_clk.frame, Range::kFrames);
            m_startRange = m_range;
            invalidateRange();

            setFrame(m_clk.frame, true);
          }
          break;
        }
        case PART_ROW_TEXT: {
          base::ScopedValue<bool> lock(m_fromTimeline, true, false);
          const layer_t old_layer = getLayerIndex(m_layer);
          const bool selectLayer = (mouseMsg->left() || !isLayerActive(m_clk.layer));
          const bool selectLayerInCanvas =
            (m_clk.layer != -1 &&
             mouseMsg->left() &&
             is_select_layer_in_canvas_key_pressed(mouseMsg));

          if (selectLayerInCanvas) {
            select_layer_boundaries(m_rows[m_clk.layer].layer(), m_frame,
                                    get_select_layer_in_canvas_op(mouseMsg));
          }
          else if (selectLayer) {
            m_state = STATE_SELECTING_LAYERS;
            if (clearRange)
              clearAndInvalidateRange();
            m_range.startRange(m_rows[m_clk.layer].layer(),
                               m_frame, Range::kLayers);
            m_startRange = m_range;
            invalidateRange();

            // Did the user select another layer?
            if (old_layer != m_clk.layer) {
              setLayer(m_rows[m_clk.layer].layer());
              invalidate();
            }
          }

          // Change the scroll to show the new selected layer/cel.
          showCel(m_clk.layer, m_frame);
          break;
        }

        case PART_ROW_EYE_ICON:
          if (validLayer(m_clk.layer)) {
            Row& row = m_rows[m_clk.layer];
            Layer* layer = row.layer();
            ASSERT(layer)

            // Hide everything or restore alternative state
            bool oneWithInternalState = false;
            if (msg->altPressed()) {
              for (const Row& row : m_rows) {
                const Layer* l = row.layer();
                if (l->hasFlags(LayerFlags::Internal_WasVisible)) {
                  oneWithInternalState = true;
                  break;
                }
              }

              // If there is one layer with the internal state, restore the previous visible state
              if (oneWithInternalState) {
                for (Row& row : m_rows) {
                  Layer* l = row.layer();
                  if (l->hasFlags(LayerFlags::Internal_WasVisible)) {
                    l->setVisible(true);
                    l->switchFlags(LayerFlags::Internal_WasVisible, false);
                  }
                  else {
                    l->setVisible(false);
                  }
                }
              }
              // In other case, hide everything
              else {
                for (Row& row : m_rows) {
                  Layer* l = row.layer();
                  l->switchFlags(LayerFlags::Internal_WasVisible, l->isVisible());
                  l->setVisible(false);
                }
              }

              regenerateRows();
              invalidate();

              m_document->notifyGeneralUpdate();
            }

            if (layer->isVisible() && !oneWithInternalState)
              m_state = STATE_HIDING_LAYERS;
            else
              m_state = STATE_SHOWING_LAYERS;

            setLayerVisibleFlag(m_clk.layer, m_state == STATE_SHOWING_LAYERS);
          }
          break;

        case PART_ROW_PADLOCK_ICON:
          if (validLayer(m_hot.layer)) {
            Row& row = m_rows[m_clk.layer];
            Layer* layer = row.layer();
            ASSERT(layer);
            if (layer->isEditable())
              m_state = STATE_LOCKING_LAYERS;
            else
              m_state = STATE_UNLOCKING_LAYERS;

            setLayerEditableFlag(m_clk.layer, m_state == STATE_UNLOCKING_LAYERS);
          }
          break;

        case PART_ROW_CONTINUOUS_ICON:
          if (validLayer(m_hot.layer)) {
            Row& row = m_rows[m_clk.layer];
            Layer* layer = row.layer();
            ASSERT(layer);

            if (layer->isImage()) {
              if (layer->isContinuous())
                m_state = STATE_DISABLING_CONTINUOUS_LAYERS;
              else
                m_state = STATE_ENABLING_CONTINUOUS_LAYERS;

              setLayerContinuousFlag(m_clk.layer, m_state == STATE_ENABLING_CONTINUOUS_LAYERS);
            }
            else if (layer->isGroup()) {
              if (layer->isCollapsed())
                m_state = STATE_EXPANDING_LAYERS;
              else
                m_state = STATE_COLLAPSING_LAYERS;

              setLayerCollapsedFlag(m_clk.layer, m_state == STATE_COLLAPSING_LAYERS);
              updateByMousePos(msg, mousePosInClientBounds());

              // The m_clk might have changed because we've
              // expanded/collapsed a group just right now (i.e. we've
              // called regenerateRows())
              m_clk = m_hot;

              ASSERT(m_rows[m_clk.layer].layer() == layer);
            }
          }
          break;

        case PART_CEL: {
          base::ScopedValue<bool> lock(m_fromTimeline, true, false);
          const layer_t old_layer = getLayerIndex(m_layer);
          const bool selectCel = (mouseMsg->left()
            || !isLayerActive(m_clk.layer)
            || !isFrameActive(m_clk.frame));
          const bool selectCelInCanvas =
            (m_clk.layer != -1 &&
             mouseMsg->left() &&
             is_select_layer_in_canvas_key_pressed(mouseMsg));
          const frame_t old_frame = m_frame;

          if (selectCelInCanvas) {
            select_layer_boundaries(m_rows[m_clk.layer].layer(),
                                    m_clk.frame,
                                    get_select_layer_in_canvas_op(mouseMsg));
          }
          else {
            if (selectCel) {
              m_state = STATE_SELECTING_CELS;
              if (clearRange)
                clearAndInvalidateRange();
              m_range.startRange(m_rows[m_clk.layer].layer(),
                                 m_clk.frame, Range::kCels);
              m_startRange = m_range;
              invalidateRange();
            }

            // Select the new clicked-part.
            if (old_layer != m_clk.layer
                || old_frame != m_clk.frame) {
              setLayer(m_rows[m_clk.layer].layer());
              setFrame(m_clk.frame, true);
              invalidate();
            }
          }

          // Change the scroll to show the new selected cel.
          showCel(m_clk.layer, m_frame);
          invalidate();
          break;
        }
        case PART_RANGE_OUTLINE:
          m_state = STATE_MOVING_RANGE;

          // If we select the outline of a cels range, we have to
          // recalculate the dragged cel (m_clk) using a special
          // hitTestCel() and limiting the clicked cel inside the
          // range bounds.
          if (m_range.type() == Range::kCels) {
            m_clk = hitTestCel(mouseMsg->position() - bounds().origin());

            if (m_range.layers() > 0) {
              layer_t layerFirst, layerLast;
              if (selectedLayersBounds(selectedLayers(),
                                       &layerFirst, &layerLast)) {
                layer_t layerIdx = m_clk.layer;
                layerIdx = std::clamp(layerIdx, layerFirst, layerLast);
                m_clk.layer = layerIdx;
              }
            }

            if (m_clk.frame < m_range.firstFrame())
              m_clk.frame = m_range.firstFrame();
            else if (m_clk.frame > m_range.lastFrame())
              m_clk.frame = m_range.lastFrame();
          }
          break;

        case PART_TAG:
          m_state = STATE_MOVING_TAG;
          m_resizeTagData.reset(m_clk.tag);
          break;
        case PART_TAG_LEFT:
          m_state = STATE_RESIZING_TAG_LEFT;
          m_resizeTagData.reset(m_clk.tag);
          // TODO reduce the scope of the invalidation
          invalidate();
          break;
        case PART_TAG_RIGHT:
          m_state = STATE_RESIZING_TAG_RIGHT;
          m_resizeTagData.reset(m_clk.tag);
          invalidate();
          break;

      }

      // Redraw the new clicked part (header, layer or cel).
      invalidateHit(m_clk);
      break;
    }

    case kMouseLeaveMessage: {
      if (m_hot.part != PART_NOTHING) {
        invalidateHit(m_hot);
        m_hot = Hit();
      }
      break;
    }

    case kMouseMoveMessage: {
      if (!m_document)
        break;

      gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position()
        - bounds().origin();

      Hit hit;
      setHot(hit = hitTest(msg, mousePos));

      if (hasCapture()) {
        switch (m_state) {

          case STATE_SCROLLING: {
            gfx::Point absMousePos = static_cast<MouseMessage*>(msg)->position();
            setViewScroll(
              viewScroll() - gfx::Point(
                (absMousePos.x - m_oldPos.x),
                (absMousePos.y - m_oldPos.y)));

            m_oldPos = absMousePos;
            return true;
          }

          case STATE_MOVING_ONIONSKIN_RANGE_LEFT: {
            gfx::Rect onionRc = getOnionskinFramesBounds();

            int newValue = m_origFrames + (m_clk.frame - hit.frame);
            docPref().onionskin.prevFrames(std::max(0, newValue));

            onionRc |= getOnionskinFramesBounds();
            invalidateRect(onionRc.offset(origin()));
            return true;
          }

          case STATE_MOVING_ONIONSKIN_RANGE_RIGHT: {
            gfx::Rect onionRc = getOnionskinFramesBounds();

            int newValue = m_origFrames - (m_clk.frame - hit.frame);
            docPref().onionskin.nextFrames(std::max(0, newValue));

            onionRc |= getOnionskinFramesBounds();
            invalidateRect(onionRc.offset(origin()));
            return true;
          }

          case STATE_SHOWING_LAYERS:
          case STATE_HIDING_LAYERS:
            m_clk = hit;
            if (hit.part == PART_ROW_EYE_ICON) {
              setLayerVisibleFlag(hit.layer, m_state == STATE_SHOWING_LAYERS);
            }
            break;

          case STATE_LOCKING_LAYERS:
          case STATE_UNLOCKING_LAYERS:
            m_clk = hit;
            if (hit.part == PART_ROW_PADLOCK_ICON) {
              setLayerEditableFlag(hit.layer, m_state == STATE_UNLOCKING_LAYERS);
            }
            break;

          case STATE_ENABLING_CONTINUOUS_LAYERS:
          case STATE_DISABLING_CONTINUOUS_LAYERS:
            m_clk = hit;
            if (hit.part == PART_ROW_CONTINUOUS_ICON) {
              setLayerContinuousFlag(hit.layer, m_state == STATE_ENABLING_CONTINUOUS_LAYERS);
            }
            break;

          case STATE_EXPANDING_LAYERS:
          case STATE_COLLAPSING_LAYERS:
            m_clk = hit;
            if (hit.part == PART_ROW_CONTINUOUS_ICON) {
              setLayerCollapsedFlag(hit.layer, m_state == STATE_COLLAPSING_LAYERS);
              updateByMousePos(msg, mousePosInClientBounds());
            }
            break;

        }

        // If the mouse pressed the mouse's button in the separator,
        // we shouldn't change the hot (so the separator can be
        // tracked to the mouse's released).
        if (m_clk.part == PART_SEPARATOR) {
          setSeparatorX(mousePos.x);
          layout();
          return true;
        }
      }

      updateDropRange(mousePos);

      if (hasCapture()) {
        switch (m_state) {

            case STATE_MOVING_RANGE: {
                frame_t newFrame;
                if (m_range.type() == Range::kLayers) {
                  // If we are moving only layers we don't change the
                  // current frame.
                  newFrame = m_frame;
                }
                else {
                  frame_t firstDrawableFrame;
                  frame_t lastDrawableFrame;
                  getDrawableFrames(&firstDrawableFrame, &lastDrawableFrame);

                  if (hit.frame < firstDrawableFrame)
                    newFrame = firstDrawableFrame - 1;
                  else if (hit.frame > lastDrawableFrame)
                    newFrame = lastDrawableFrame + 1;
                  else
                    newFrame = hit.frame;
                }

                layer_t newLayer;
                if (m_range.type() == Range::kFrames) {
                  // If we are moving only frames we don't change the
                  // current layer.
                  newLayer = getLayerIndex(m_layer);
                }
                else {
                  layer_t firstDrawableLayer;
                  layer_t lastDrawableLayer;
                  getDrawableLayers(&firstDrawableLayer, &lastDrawableLayer);

                  if (hit.layer < firstDrawableLayer)
                    newLayer = firstDrawableLayer - 1;
                  else if (hit.layer > lastDrawableLayer)
                    newLayer = lastDrawableLayer + 1;
                  else
                    newLayer = hit.layer;
                }

                showCel(newLayer, newFrame);
                break;
            }

          case STATE_SELECTING_LAYERS: {
            Layer* hitLayer = m_rows[hit.layer].layer();
            if (m_layer != hitLayer) {
              m_clk.layer = hit.layer;

              // We have to change the range before we generate an
              // onActiveSiteChange() event so observers (like cel
              // properties dialog) know the new selected range.
              m_range = m_startRange;
              m_range.endRange(hitLayer, m_frame);

              setLayer(hitLayer);
            }
            break;
          }

          case STATE_SELECTING_FRAMES: {
            invalidateRange();

            m_range = m_startRange;
            m_range.endRange(m_layer, hit.frame);

            setFrame(m_clk.frame = hit.frame, true);

            invalidateRange();
            break;
          }

          case STATE_SELECTING_CELS: {
            Layer* hitLayer = m_rows[hit.layer].layer();
            if ((m_layer != hitLayer) || (m_frame != hit.frame)) {
              m_clk.layer = hit.layer;

              m_range = m_startRange;
              m_range.endRange(hitLayer, hit.frame);

              setLayer(hitLayer);
              setFrame(m_clk.frame = hit.frame, true);
            }
            break;
          }

          case STATE_MOVING_TAG:
            // TODO
            break;

          case STATE_RESIZING_TAG_LEFT:
          case STATE_RESIZING_TAG_RIGHT: {
            auto tag = doc::get<doc::Tag>(m_resizeTagData.tag);
            if (tag) {
              switch (m_state) {
                case STATE_RESIZING_TAG_LEFT:
                  m_resizeTagData.from = std::clamp(hit.frame, 0, tag->toFrame());
                  break;
                case STATE_RESIZING_TAG_RIGHT:
                  m_resizeTagData.to = std::clamp(hit.frame, tag->fromFrame(), m_sprite->lastFrame());
                  break;
              }
              invalidate();
            }
            break;
          }

        }
      }

      updateStatusBar(msg);
      updateCelOverlayBounds(hit);
      return true;
    }

    case kMouseUpMessage:
      if (hasCapture()) {
        ASSERT(m_document != NULL);

        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

        if (m_state == STATE_SCROLLING) {
          m_state = STATE_STANDBY;
          releaseMouse();
          return true;
        }

        bool regenRows = false;
        bool relayout = false;
        setHot(hitTest(msg, mouseMsg->position() - bounds().origin()));

        switch (m_hot.part) {

          case PART_HEADER_GEAR: {
            gfx::Rect gearBounds =
              getPartBounds(Hit(PART_HEADER_GEAR)).offset(bounds().origin());

            if (!m_confPopup) {
              m_confPopup.reset(new ConfigureTimelinePopup);
              m_confPopup->remapWindow();
            }

            if (!m_confPopup->isVisible()) {
              gfx::Rect bounds = m_confPopup->bounds();
              ui::fit_bounds(display(), BOTTOM, gearBounds, bounds);
              ui::fit_bounds(display(), m_confPopup.get(), bounds);
              m_confPopup->openWindow();
            }
            else {
              m_confPopup->closeWindow(nullptr);
            }
            break;
          }

          case PART_HEADER_FRAME:
            // Show the frame pop-up menu.
            if (mouseMsg->right()) {
              if (m_clk.frame == m_hot.frame) {
                Menu* popupMenu = AppMenus::instance()->getFramePopupMenu();
                if (popupMenu) {
                  popupMenu->showPopup(mouseMsg->position(), display());

                  m_state = STATE_STANDBY;
                  invalidate();
                }
              }
            }
            break;

          case PART_ROW_TEXT:
            // Show the layer pop-up menu.
            if (mouseMsg->right()) {
              if (m_clk.layer == m_hot.layer) {
                Menu* popupMenu = AppMenus::instance()->getLayerPopupMenu();
                if (popupMenu) {
                  popupMenu->showPopup(mouseMsg->position(), display());

                  m_state = STATE_STANDBY;
                  invalidate();
                }
              }
            }
            break;

          case PART_CEL: {
            // Show the cel pop-up menu.
            if (mouseMsg->right()) {
              Menu* popupMenu =
                (m_state == STATE_MOVING_RANGE &&
                 m_range.type() == Range::kCels &&
                 (m_hot.layer != m_clk.layer ||
                  m_hot.frame != m_clk.frame)) ?
                  AppMenus::instance()->getCelMovementPopupMenu():
                  AppMenus::instance()->getCelPopupMenu();
              if (popupMenu) {
                popupMenu->showPopup(mouseMsg->position(), display());

                // Do not drop in this function, the drop is done from
                // the menu in case we've used the
                // CelMovementPopupMenu
                m_state = STATE_STANDBY;
                invalidate();
              }
            }
            break;
          }

          case PART_TAG: {
            Tag* tag = m_clk.getTag();
            if (tag) {
              Params params;
              params.set("id", base::convert_to<std::string>(tag->id()).c_str());

              // As the m_clk.tag can be deleted with
              // RemoveTag command, we've to clean all references
              // to it from Hit() structures.
              cleanClk();
              m_hot = m_clk;

              if (mouseMsg->right()) {
                Menu* popupMenu = AppMenus::instance()->getTagPopupMenu();
                if (popupMenu) {
                  AppMenuItem::setContextParams(params);
                  popupMenu->showPopup(mouseMsg->position(), display());
                  AppMenuItem::setContextParams(Params());

                  m_state = STATE_STANDBY;
                  invalidate();
                }
              }
              else if (mouseMsg->left()) {
                Command* command = Commands::instance()
                  ->byId(CommandId::FrameTagProperties());
                UIContext::instance()->executeCommand(command, params);
              }
            }
            break;
          }

          case PART_TAG_SWITCH_BAND_BUTTON:
            if (m_clk.band >= 0) {
              focusTagBand(m_clk.band);
              regenRows = true;
              relayout = true;
            }
            break;

        }

        if (regenRows) {
          regenerateRows();
          invalidate();
        }
        if (relayout)
          layout();

        switch (m_state) {
          case STATE_MOVING_RANGE:
            if (m_dropRange.type() != Range::kNone) {
              dropRange(is_copy_key_pressed(mouseMsg) ?
                        Timeline::kCopy:
                        Timeline::kMove);
            }
            break;

          case STATE_MOVING_TAG:
            m_resizeTagData.reset();
            break;

          case STATE_RESIZING_TAG_LEFT:
          case STATE_RESIZING_TAG_RIGHT: {
            auto tag = doc::get<doc::Tag>(m_resizeTagData.tag);
            if (tag) {
              if ((m_state == STATE_RESIZING_TAG_LEFT && tag->fromFrame() != m_resizeTagData.from) ||
                  (m_state == STATE_RESIZING_TAG_RIGHT && tag->toFrame() != m_resizeTagData.to)) {
                Tx tx(UIContext::instance(), Strings::commands_FrameTagProperties());
                tx(new cmd::SetTagRange(
                     tag,
                     (m_state == STATE_RESIZING_TAG_LEFT ? m_resizeTagData.from: tag->fromFrame()),
                     (m_state == STATE_RESIZING_TAG_RIGHT ? m_resizeTagData.to: tag->toFrame())));
                tx.commit();

                regenerateRows();
              }
            }
            m_resizeTagData.reset();
            break;
          }

        }

        // Clean the clicked-part & redraw the hot-part.
        cleanClk();

        if (hasCapture())
          invalidate();
        else
          invalidateHit(m_hot);

        // Restore the cursor.
        m_state = STATE_STANDBY;
        setCursor(msg, hitTest(msg, mouseMsg->position() - bounds().origin()));

        releaseMouse();
        updateStatusBar(msg);
        return true;
      }
      break;

    case kDoubleClickMessage:
      switch (m_hot.part) {

        case PART_ROW_TEXT: {
          Command* command = Commands::instance()
            ->byId(CommandId::LayerProperties());

          UIContext::instance()->executeCommand(command);
          return true;
        }

        case PART_HEADER_FRAME: {
          Command* command = Commands::instance()
            ->byId(CommandId::FrameProperties());
          Params params;
          params.set("frame", "current");

          UIContext::instance()->executeCommand(command, params);
          return true;
        }

        case PART_CEL: {
          Command* command = Commands::instance()
            ->byId(CommandId::CelProperties());

          UIContext::instance()->executeCommand(command);
          return true;
        }

        case PART_TAG_BAND:
          if (m_hot.band >= 0) {
            focusTagBand(m_hot.band);
            regenerateRows();
            invalidate();
            layout();
            return true;
          }
          break;

      }
      break;

    case kKeyDownMessage: {
      bool used = false;

      switch (static_cast<KeyMessage*>(msg)->scancode()) {

        case kKeyEsc:
          if (m_state == STATE_STANDBY) {
            clearAndInvalidateRange();
          }
          else {
            m_state = STATE_STANDBY;
          }

          // Don't use this key, so it's caught by CancelCommand.
          // TODO The deselection of the current range should be
          // handled in CancelCommand itself.
          //used = true;
          break;

        case kKeySpace: {
          // If we receive a key down event when the Space bar is
          // pressed (because the Timeline has the keyboard focus) but
          // we don't have the mouse inside, we don't consume this
          // event so the Space bar can be used by the Editor to
          // activate the hand/pan/scroll tool.
          if (!hasMouse())
            break;

          m_scroll = true;
          used = true;
          break;
        }
      }

      updateByMousePos(msg, mousePosInClientBounds());
      if (used)
        return true;

      break;
    }

    case kKeyUpMessage: {
      bool used = false;

      switch (static_cast<KeyMessage*>(msg)->scancode()) {

        case kKeySpace: {
          m_scroll = false;
          used = true;
          break;
        }
      }

      updateByMousePos(msg, mousePosInClientBounds());
      if (used)
        return true;

      break;
    }

    case kMouseWheelMessage:
      if (m_document) {
        gfx::Point delta = static_cast<MouseMessage*>(msg)->wheelDelta();
        const bool precise = static_cast<MouseMessage*>(msg)->preciseWheel();

        // Zoom timeline
        if (msg->ctrlPressed() || // TODO configurable
            msg->cmdPressed()) {
          double dz = delta.x + delta.y;

          if (precise) {
            dz /= 1.5;
            if (dz < -1.0) dz = -1.0;
            else if (dz > 1.0) dz = 1.0;
          }

          setZoomAndUpdate(m_zoom - dz, true);
        }
        else {
          if (!precise) {
            delta.x *= frameBoxWidth();
            delta.y *= layerBoxHeight();

            if (delta.x == 0 && // On macOS shift already changes the wheel axis
                msg->shiftPressed()) {
              if (std::fabs(delta.y) > delta.x)
                std::swap(delta.x, delta.y);
            }

            if (msg->altPressed()) {
              delta.x *= 3;
              delta.y *= 3;
            }
          }
          setViewScroll(viewScroll() + delta);
        }
      }
      break;

    case kSetCursorMessage:
      if (m_document) {
        setCursor(msg, m_hot);
        return true;
      }
      break;

    case kTouchMagnifyMessage:
      setZoomAndUpdate(
        m_zoom + m_zoom * static_cast<ui::TouchMessage*>(msg)->magnification(),
        true);
      break;
  }

  return Widget::onProcessMessage(msg);
}

void Timeline::onInitTheme(ui::InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);

  auto theme = SkinTheme::get(this);
  int barsize = theme->dimensions.miniScrollbarSize();
  m_hbar.setBarWidth(barsize);
  m_vbar.setBarWidth(barsize);
  m_hbar.setStyle(theme->styles.transparentScrollbar());
  m_vbar.setStyle(theme->styles.transparentScrollbar());
  m_hbar.setThumbStyle(theme->styles.transparentScrollbarThumb());
  m_vbar.setThumbStyle(theme->styles.transparentScrollbarThumb());

  if (m_confPopup)
    m_confPopup->initTheme();
}

void Timeline::onInvalidateRegion(const gfx::Region& region)
{
  Widget::onInvalidateRegion(region);
  m_redrawMarchingAntsOnly = false;
}

void Timeline::onSizeHint(SizeHintEvent& ev)
{
  // This doesn't matter, the AniEditor'll use the entire screen anyway.
  ev.setSizeHint(Size(32, 32));
}

void Timeline::onResize(ui::ResizeEvent& ev)
{
  gfx::Rect rc = ev.bounds();
  setBoundsQuietly(rc);

  gfx::Size sz = m_aniControls.sizeHint();
  m_aniControls.setBounds(
    gfx::Rect(
      rc.x,
      rc.y+(visibleTagBands()-1)*oneTagHeight(),
      (!m_sprite || m_sprite->tags().empty() ? std::min(sz.w, rc.w):
                                               std::min(sz.w, separatorX())),
      oneTagHeight()));

  updateScrollBars();
}

void Timeline::onPaint(ui::PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  bool noDoc = (m_document == NULL);
  if (noDoc)
    goto paintNoDoc;

  try {
    // Lock the sprite to read/render it. Here we don't wait if the
    // document is locked (e.g. a filter is being applied to the
    // sprite) to avoid locking the UI.
    const DocReader docReader(m_document, 0);

    if (m_redrawMarchingAntsOnly) {
      drawClipboardRange(g);
      m_redrawMarchingAntsOnly = false;
      return;
    }

    layer_t layer, firstLayer, lastLayer;
    frame_t frame, firstFrame, lastFrame;

    getDrawableLayers(&firstLayer, &lastLayer);
    getDrawableFrames(&firstFrame, &lastFrame);

    drawTop(g);

    // Draw the header for layers.
    drawHeader(g);

    // Draw the header for each visible frame.
    {
      IntersectClip clip(g, getFrameHeadersBounds());
      if (clip) {
        for (frame=firstFrame; frame<=lastFrame; ++frame)
          drawHeaderFrame(g, frame);

        // Draw onionskin indicators.
        gfx::Rect bounds = getOnionskinFramesBounds();
        if (!bounds.isEmpty()) {
          drawPart(
            g, bounds, nullptr,
            skinTheme()->styles.timelineOnionskinRange(),
            false, false, false);
        }
      }
    }

    // Draw each visible layer.
    DrawCelData data;
    for (layer=lastLayer; layer>=firstLayer; --layer) {
      {
        IntersectClip clip(g, getLayerHeadersBounds());
        if (clip)
          drawLayer(g, layer);
      }

      IntersectClip clip(g, getCelsBounds());
      if (!clip)
        continue;

      Layer* layerPtr = m_rows[layer].layer();
      if (!layerPtr->isImage()) {
        // Draw empty cels
        for (frame=firstFrame; frame<=lastFrame; ++frame) {
          drawCel(g, layer, frame, nullptr, nullptr);
        }
        continue;
      }

      // Get the first CelIterator to be drawn (it is the first cel with cel->frame >= first_frame)
      LayerImage* layerImagePtr = static_cast<LayerImage*>(layerPtr);
      data.begin = layerImagePtr->getCelBegin();
      data.end = layerImagePtr->getCelEnd();
      data.it = layerImagePtr->findFirstCelIteratorAfter(firstFrame-1);
      if (firstFrame > 0 && data.it != data.begin)
        data.prevIt = data.it-1;
      else
        data.prevIt = data.end;
      data.nextIt = (data.it != data.end ? data.it+1: data.end);

      // Calculate link range for the active cel
      data.firstLink = data.end;
      data.lastLink = data.end;

      if (layerPtr == m_layer) {
        data.activeIt = layerImagePtr->findCelIterator(m_frame);
        if (data.activeIt != data.end) {
          data.firstLink = data.activeIt;
          data.lastLink = data.activeIt;

          ObjectId imageId = (*data.activeIt)->image()->id();

          auto it2 = data.activeIt;
          if (it2 != data.begin) {
            do {
              --it2;
              if ((*it2)->image()->id() == imageId) {
                data.firstLink = it2;
                if ((*data.firstLink)->frame() < firstFrame)
                  break;
              }
            } while (it2 != data.begin);
          }

          it2 = data.activeIt;
          while (it2 != data.end) {
            if ((*it2)->image()->id() == imageId) {
              data.lastLink = it2;
              if ((*data.lastLink)->frame() > lastFrame)
                break;
            }
            ++it2;
          }
        }
      }
      else
        data.activeIt = data.end;

      // Draw every visible cel for each layer.
      for (frame=firstFrame; frame<=lastFrame; ++frame) {
        Cel* cel =
          (data.it != data.end &&
           (*data.it)->frame() == frame ? *data.it: nullptr);

        drawCel(g, layer, frame, cel, &data);

        if (cel) {
          data.prevIt = data.it;
          data.it = data.nextIt; // Point to next cel
          if (data.nextIt != data.end)
            ++data.nextIt;
        }
      }
    }

    drawPaddings(g);
    drawTags(g);
    drawRangeOutline(g);
    drawClipboardRange(g);
    drawCelOverlay(g);

#if 0 // Use this code to debug the calculated m_dropRange by updateDropRange()
    {
      g->drawRect(gfx::rgba(255, 255, 0), getRangeBounds(m_range));
      g->drawRect(gfx::rgba(255, 0, 0), getRangeBounds(m_dropRange));
    }
#endif
  }
  catch (const LockedDocException&) {
    // The sprite is locked, so we defer the rendering of the sprite
    // for later.
    noDoc = true;
    defer_invalid_rect(g->getClipBounds().offset(bounds().origin()));
  }

paintNoDoc:;
  if (noDoc)
    drawPart(
      g, clientBounds(), nullptr,
      skinTheme()->styles.timelinePadding());
}

void Timeline::onBeforeCommandExecution(CommandExecutionEvent& ev)
{
  m_savedVersion = (m_document ? m_document->sprite()->version(): 0);
}

void Timeline::onAfterCommandExecution(CommandExecutionEvent& ev)
{
  if (!m_document)
    return;

  // TODO Improve this: check if the structure of layers/frames has changed
  const doc::ObjectVersion currentVersion = m_document->sprite()->version();
  if (m_savedVersion != currentVersion) {
    regenerateRows();
    showCurrentCel();
    invalidate();
  }
}

void Timeline::onActiveSiteChange(const Site& site)
{
  if (hasMouse()) {
    updateStatusBarForFrame(site.frame(), nullptr, site.cel());
  }
}

void Timeline::onRemoveDocument(Doc* document)
{
  if (document == m_document) {
    detachDocument();
  }
}

void Timeline::onGeneralUpdate(DocEvent& ev)
{
  invalidate();
}

void Timeline::onAddLayer(DocEvent& ev)
{
  ASSERT(ev.layer() != NULL);

  setLayer(ev.layer());

  regenerateRows();
  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

// TODO similar to ActiveSiteHandler::onBeforeRemoveLayer() and Editor::onBeforeRemoveLayer()
void Timeline::onBeforeRemoveLayer(DocEvent& ev)
{
  Layer* layerToSelect = candidate_if_layer_is_deleted(m_layer, ev.layer());
  if (m_layer != layerToSelect)
    setLayer(layerToSelect);

  // Remove layer from ranges
  m_range.eraseAndAdjust(ev.layer());
  m_startRange.eraseAndAdjust(ev.layer());
  m_dropRange.eraseAndAdjust(ev.layer());

  ASSERT(!m_range.contains(ev.layer()));
  ASSERT(!m_startRange.contains(ev.layer()));
  ASSERT(!m_dropRange.contains(ev.layer()));
}

// We have to regenerate the layer rows (m_rows) after the layer is
// removed from the sprite.
void Timeline::onAfterRemoveLayer(DocEvent& ev)
{
  regenerateRows();
  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

void Timeline::onAddFrame(DocEvent& ev)
{
  setFrame(ev.frame(), false);

  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

// TODO similar to ActiveSiteHandler::onRemoveFrame()
void Timeline::onRemoveFrame(DocEvent& ev)
{
  // Adjust current frame of all editors that are in a frame more
  // advanced that the removed one.
  if (m_frame > ev.frame()) {
    setFrame(m_frame-1, false);
  }
  // If the editor was in the previous "last frame" (current value of
  // totalFrames()), we've to adjust it to the new last frame
  // (lastFrame())
  else if (m_frame >= sprite()->totalFrames()) {
    setFrame(sprite()->lastFrame(), false);
  }

  // Disable the selected range when we remove frames
  if (m_range.enabled())
    clearAndInvalidateRange();

  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

void Timeline::onAddCel(DocEvent& ev)
{
  invalidateLayer(ev.layer());
}

void Timeline::onAfterRemoveCel(DocEvent& ev)
{
  invalidateLayer(ev.layer());
}

void Timeline::onLayerNameChange(DocEvent& ev)
{
  invalidate();
}

void Timeline::onAddTag(DocEvent& ev)
{
  if (m_tagFocusBand >= 0) {
    m_tagFocusBand = -1;
    regenerateRows();
    layout();
  }
}

void Timeline::onRemoveTag(DocEvent& ev)
{
  onAddTag(ev);
}

void Timeline::onTagChange(DocEvent& ev)
{
  invalidateHit(Hit(PART_TAGS));
}

void Timeline::onTagRename(DocEvent& ev)
{
  invalidateHit(Hit(PART_TAGS));
}

void Timeline::onLayerCollapsedChanged(DocEvent& ev)
{
  regenerateRows();
  invalidate();
}

void Timeline::onStateChanged(Editor* editor)
{
  m_aniControls.updateUsingEditor(editor);
}

void Timeline::onAfterFrameChanged(Editor* editor)
{
  if (m_fromTimeline)
    return;

  setFrame(editor->frame(), false);

  if (!hasCapture() && !editor->keepTimelineRange())
    clearAndInvalidateRange();

  showCurrentCel();
}

void Timeline::onAfterLayerChanged(Editor* editor)
{
  if (m_fromTimeline)
    return;

  if (!hasCapture())
    m_range.clearRange();

  setLayer(editor->layer());
  showCurrentCel();
}

void Timeline::onDestroyEditor(Editor* editor)
{
  ASSERT(m_editor == editor);
  if (m_editor == editor) {
    m_editor->remove_observer(this);
    m_editor = nullptr;
  }
}

void Timeline::setCursor(ui::Message* msg, const Hit& hit)
{
  // Scrolling.
  if (m_state == STATE_SCROLLING || m_scroll) {
    ui::set_mouse_cursor(kScrollCursor);
  }
  // Moving.
  else if (m_state == STATE_MOVING_RANGE) {
    if (is_copy_key_pressed(msg))
      ui::set_mouse_cursor(kArrowPlusCursor);
    else
      ui::set_mouse_cursor(kMoveCursor);
  }
  // Normal state.
  else if (hit.part == PART_HEADER_ONIONSKIN_RANGE_LEFT
    || m_state == STATE_MOVING_ONIONSKIN_RANGE_LEFT) {
    ui::set_mouse_cursor(kSizeWCursor);
  }
  else if (hit.part == PART_HEADER_ONIONSKIN_RANGE_RIGHT
    || m_state == STATE_MOVING_ONIONSKIN_RANGE_RIGHT) {
    ui::set_mouse_cursor(kSizeECursor);
  }
  else if (hit.part == PART_RANGE_OUTLINE) {
    ui::set_mouse_cursor(kMoveCursor);
  }
  else if (hit.part == PART_SEPARATOR) {
    ui::set_mouse_cursor(kSizeWECursor);
  }
  else if (hit.part == PART_TAG) {
    ui::set_mouse_cursor(kHandCursor);
  }
  else if (hit.part == PART_TAG_RIGHT) {
    ui::set_mouse_cursor(kSizeECursor);
  }
  else if (hit.part == PART_TAG_LEFT) {
    ui::set_mouse_cursor(kSizeWCursor);
  }
  else {
    ui::set_mouse_cursor(kArrowCursor);
  }
}

void Timeline::getDrawableLayers(layer_t* firstDrawableLayer,
                                 layer_t* lastDrawableLayer)
{
  layer_t i = lastLayer()
            - ((viewScroll().y + getCelsBounds().h) / layerBoxHeight());
  i = std::clamp(i, firstLayer(), lastLayer());

  layer_t j = lastLayer() - viewScroll().y / layerBoxHeight();;
  if (!m_rows.empty())
    j = std::clamp(j, firstLayer(), lastLayer());
  else
    j = -1;

  *firstDrawableLayer = i;
  *lastDrawableLayer = j;
}

void Timeline::getDrawableFrames(frame_t* firstFrame, frame_t* lastFrame)
{
  *firstFrame = frame_t(viewScroll().x / frameBoxWidth());
  *lastFrame = frame_t((viewScroll().x
      + getCelsBounds().w) / frameBoxWidth());
}

void Timeline::drawPart(ui::Graphics* g, const gfx::Rect& bounds,
                        const std::string* text, ui::Style* style,
                        const bool is_active,
                        const bool is_hover,
                        const bool is_clicked,
                        const bool is_disabled)
{
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  PaintWidgetPartInfo info;
  info.text = text;
  info.styleFlags =
    (is_active ? ui::Style::Layer::kFocus: 0) |
    (is_hover ? ui::Style::Layer::kMouse: 0) |
    (is_clicked ? ui::Style::Layer::kSelected: 0) |
    (is_disabled ? ui::Style::Layer::kDisabled: 0);

  theme()->paintWidgetPart(g, style, bounds, info);
}

void Timeline::drawClipboardRange(ui::Graphics* g)
{
  Doc* clipboard_document;
  DocRange clipboard_range;
  Clipboard::instance()->getDocumentRangeInfo(
    &clipboard_document,
    &clipboard_range);

  if (!m_document ||
      clipboard_document != m_document ||
      !m_clipboard_timer.isRunning())
    return;

  IntersectClip clip(g, getRangeClipBounds(clipboard_range));
  if (clip) {
    ui::Paint paint;
    paint.style(ui::Paint::Stroke);
    ui::set_checkered_paint_mode(paint, m_offset_count,
                                 gfx::rgba(0, 0, 0, 255),
                                 gfx::rgba(255, 255, 255, 255));
    g->drawRect(getRangeBounds(clipboard_range), paint);
  }
}

void Timeline::drawTop(ui::Graphics* g)
{
  g->fillRect(skinTheme()->colors.workspace(),
    getPartBounds(Hit(PART_TOP)));
}

void Timeline::drawHeader(ui::Graphics* g)
{
  auto& styles = skinTheme()->styles;
  bool allInvisible = allLayersInvisible();
  bool allLocked = allLayersLocked();
  bool allContinuous = allLayersContinuous();

  drawPart(g, getPartBounds(Hit(PART_HEADER_EYE)),
    nullptr,
    allInvisible ? styles.timelineClosedEye(): styles.timelineOpenEye(),
    m_clk.part == PART_HEADER_EYE,
    m_hot.part == PART_HEADER_EYE,
    m_clk.part == PART_HEADER_EYE);

  drawPart(g, getPartBounds(Hit(PART_HEADER_PADLOCK)),
    nullptr,
    allLocked ? styles.timelineClosedPadlock(): styles.timelineOpenPadlock(),
    m_clk.part == PART_HEADER_PADLOCK,
    m_hot.part == PART_HEADER_PADLOCK,
    m_clk.part == PART_HEADER_PADLOCK);

  drawPart(g, getPartBounds(Hit(PART_HEADER_CONTINUOUS)),
    nullptr,
    allContinuous ? styles.timelineContinuous(): styles.timelineDiscontinuous(),
    m_clk.part == PART_HEADER_CONTINUOUS,
    m_hot.part == PART_HEADER_CONTINUOUS,
    m_clk.part == PART_HEADER_CONTINUOUS);

  drawPart(g, getPartBounds(Hit(PART_HEADER_GEAR)),
    nullptr,
    styles.timelineGear(),
    m_clk.part == PART_HEADER_GEAR,
    m_hot.part == PART_HEADER_GEAR,
    m_clk.part == PART_HEADER_GEAR);

  drawPart(g, getPartBounds(Hit(PART_HEADER_ONIONSKIN)),
    NULL, styles.timelineOnionskin(),
    docPref().onionskin.active() || (m_clk.part == PART_HEADER_ONIONSKIN),
    m_hot.part == PART_HEADER_ONIONSKIN,
    m_clk.part == PART_HEADER_ONIONSKIN);

  // Empty header space.
  drawPart(g, getPartBounds(Hit(PART_HEADER_LAYER)),
    NULL, styles.timelineBox(), false, false, false);
}

void Timeline::drawHeaderFrame(ui::Graphics* g, frame_t frame)
{
  bool is_active = isFrameActive(frame);
  bool is_hover = (m_hot.part == PART_HEADER_FRAME && m_hot.frame == frame);
  bool is_clicked = (m_clk.part == PART_HEADER_FRAME && m_clk.frame == frame);
  gfx::Rect bounds = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), frame));
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  // Draw the header for the layers.
  const int n = (docPref().timeline.firstFrame()+frame);
  std::string text = base::convert_to<std::string, int>(n % 100);
  if (n >= 100 && (n % 100) < 10)
    text.insert(0, 1, '0');

  drawPart(g, bounds, &text,
           skinTheme()->styles.timelineHeaderFrame(),
           is_active, is_hover, is_clicked);
}

void Timeline::drawLayer(ui::Graphics* g, int layerIdx)
{
  ASSERT(layerIdx >= 0 && layerIdx < int(m_rows.size()));
  if (layerIdx < 0 || layerIdx >= m_rows.size())
    return;

  auto& styles = skinTheme()->styles;
  Layer* layer = m_rows[layerIdx].layer();
  bool is_active = isLayerActive(layerIdx);
  bool hotlayer = (m_hot.layer == layerIdx);
  bool clklayer = (m_clk.layer == layerIdx);
  gfx::Rect bounds = getPartBounds(Hit(PART_ROW, layerIdx, firstFrame()));
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  // Draw the eye (visible flag).
  bounds = getPartBounds(Hit(PART_ROW_EYE_ICON, layerIdx));
  drawPart(
    g, bounds, nullptr,
    (layer->isVisible() ? styles.timelineOpenEye():
                          styles.timelineClosedEye()),
    is_active || (clklayer && m_clk.part == PART_ROW_EYE_ICON),
    (hotlayer && m_hot.part == PART_ROW_EYE_ICON),
    (clklayer && m_clk.part == PART_ROW_EYE_ICON),
    !m_rows[layerIdx].parentVisible());

  // Draw the padlock (editable flag).
  bounds = getPartBounds(Hit(PART_ROW_PADLOCK_ICON, layerIdx));
  drawPart(
    g, bounds, nullptr,
    (layer->isEditable() ? styles.timelineOpenPadlock():
                           styles.timelineClosedPadlock()),
    is_active || (clklayer && m_clk.part == PART_ROW_PADLOCK_ICON),
    (hotlayer && m_hot.part == PART_ROW_PADLOCK_ICON),
    (clklayer && m_clk.part == PART_ROW_PADLOCK_ICON),
    !m_rows[layerIdx].parentEditable());

  // Draw the continuous flag/group icon.
  bounds = getPartBounds(Hit(PART_ROW_CONTINUOUS_ICON, layerIdx));
  if (layer->isImage()) {
    drawPart(g, bounds, nullptr,
             layer->isContinuous() ? styles.timelineContinuous():
                                     styles.timelineDiscontinuous(),
             is_active || (clklayer && m_clk.part == PART_ROW_CONTINUOUS_ICON),
             (hotlayer && m_hot.part == PART_ROW_CONTINUOUS_ICON),
             (clklayer && m_clk.part == PART_ROW_CONTINUOUS_ICON));
  }
  else if (layer->isGroup()) {
    drawPart(g, bounds, nullptr,
             layer->isCollapsed() ? styles.timelineClosedGroup():
                                    styles.timelineOpenGroup(),
             is_active || (clklayer && m_clk.part == PART_ROW_CONTINUOUS_ICON),
             (hotlayer && m_hot.part == PART_ROW_CONTINUOUS_ICON),
             (clklayer && m_clk.part == PART_ROW_CONTINUOUS_ICON));
  }

  // Get the layer's name bounds.
  bounds = getPartBounds(Hit(PART_ROW_TEXT, layerIdx));

  // Draw layer name.
  doc::color_t layerColor = layer->userData().color();
  gfx::Rect textBounds = bounds;
  if (m_rows[layerIdx].level() > 0) {
    const int frameBoxWithWithoutZoom =
      skinTheme()->dimensions.timelineBaseSize();
    const int w = m_rows[layerIdx].level()*frameBoxWithWithoutZoom;
    textBounds.x += w;
    textBounds.w -= w;
  }

  // Layer name background
  drawPart(g, bounds, nullptr, styles.timelineLayer(),
           is_active || (clklayer && m_clk.part == PART_ROW_TEXT),
           (hotlayer && m_hot.part == PART_ROW_TEXT),
           (clklayer && m_clk.part == PART_ROW_TEXT));

  if (doc::rgba_geta(layerColor) > 0) {
    // Fill with an user-defined custom color.
    auto b2 = textBounds;
    b2.shrink(1*guiscale()).inflate(1*guiscale());
    g->fillRect(gfx::rgba(doc::rgba_getr(layerColor),
                          doc::rgba_getg(layerColor),
                          doc::rgba_getb(layerColor),
                          doc::rgba_geta(layerColor)),
                b2);
  }

  // Tilemap icon
  if (layer->isTilemap()) {
    drawPart(g, textBounds, nullptr, styles.timelineTilemapLayer(),
             is_active || (clklayer && m_clk.part == PART_ROW_TEXT),
             (hotlayer && m_hot.part == PART_ROW_TEXT),
             (clklayer && m_clk.part == PART_ROW_TEXT));

    gfx::Size sz = skinTheme()->calcSizeHint(
      this, skinTheme()->styles.timelineTilemapLayer());
    textBounds.x += sz.w;
    textBounds.w -= sz.w;
  }

  // Layer text
  drawPart(g, textBounds,
           &layer->name(),
           styles.timelineLayerTextOnly(),
           is_active,
           (hotlayer && m_hot.part == PART_ROW_TEXT),
           (clklayer && m_clk.part == PART_ROW_TEXT));

  if (layer->isBackground()) {
    int s = ui::guiscale();
    g->fillRect(
      is_active ?
      skinTheme()->colors.timelineClickedText():
      skinTheme()->colors.timelineNormalText(),
      gfx::Rect(bounds.x+4*s,
        bounds.y+bounds.h-2*s,
        font()->textLength(layer->name().c_str()), s));
  }
  else if (layer->isReference()) {
    int s = ui::guiscale();
    g->fillRect(
      is_active ?
      skinTheme()->colors.timelineClickedText():
      skinTheme()->colors.timelineNormalText(),
      gfx::Rect(bounds.x+4*s,
        bounds.y+bounds.h/2,
        font()->textLength(layer->name().c_str()), s));
  }

  // If this layer wasn't clicked but there are another layer clicked,
  // we have to draw some indicators to show that the user can move
  // layers.
  if (hotlayer && !is_active && m_clk.part == PART_ROW_TEXT) {
    // TODO this should be skinneable
    g->fillRect(
      skinTheme()->colors.timelineActive(),
      gfx::Rect(bounds.x, bounds.y, bounds.w, 2));
  }
}

void Timeline::drawCel(ui::Graphics* g, layer_t layerIndex, frame_t frame, Cel* cel, DrawCelData* data)
{
  auto& styles = skinTheme()->styles;
  Layer* layer = m_rows[layerIndex].layer();
  Image* image = (cel ? cel->image(): nullptr);
  bool is_hover = (m_hot.part == PART_CEL &&
    m_hot.layer == layerIndex &&
    m_hot.frame == frame);
  const bool is_active = isCelActive(layerIndex, frame);
  const bool is_loosely_active = isCelLooselyActive(layerIndex, frame);
  const bool is_empty = (image == nullptr);
  gfx::Rect bounds = getPartBounds(Hit(PART_CEL, layerIndex, frame));
  gfx::Rect full_bounds = bounds;
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  // Draw background
  if (layer == m_layer && frame == m_frame)
    drawPart(g, bounds, nullptr,
             m_range.enabled() ? styles.timelineFocusedCel():
                                 styles.timelineSelectedCel(), false, is_hover, true);
  else if (m_range.enabled() && is_active)
    drawPart(g, bounds, nullptr, styles.timelineSelectedCel(), false, is_hover, true);
  else
    drawPart(g, bounds, nullptr, styles.timelineBox(), is_loosely_active, is_hover);

  // Fill with an user-defined custom color.
  if (cel && cel->data()) {
    doc::color_t celColor = cel->data()->userData().color();
    if (doc::rgba_geta(celColor) > 0) {
      auto b2 = bounds;
      b2.shrink(1 * guiscale()).inflate(1 * guiscale());
      g->fillRect(gfx::rgba(doc::rgba_getr(celColor),
                            doc::rgba_getg(celColor),
                            doc::rgba_getb(celColor),
                            doc::rgba_geta(celColor)),
                  b2);
    }
  }

  // Draw keyframe shape

  ui::Style* style = nullptr;
  bool fromLeft = false;
  bool fromRight = false;
  if (is_empty || !data) {
    style = styles.timelineEmptyFrame();
  }
  else {
    // Calculate which cel is next to this one (in previous and next
    // frame).
    Cel* left = (data->prevIt != data->end ? *data->prevIt: nullptr);
    Cel* right = (data->nextIt != data->end ? *data->nextIt: nullptr);
    if (left && left->frame() != frame-1) left = nullptr;
    if (right && right->frame() != frame+1) right = nullptr;

    ObjectId leftImg = (left ? left->image()->id(): 0);
    ObjectId rightImg = (right ? right->image()->id(): 0);
    fromLeft = (leftImg == cel->image()->id());
    fromRight = (rightImg == cel->image()->id());

    if (fromLeft && fromRight)
      style = styles.timelineFromBoth();
    else if (fromLeft)
      style = styles.timelineFromLeft();
    else if (fromRight)
      style = styles.timelineFromRight();
    else
      style = styles.timelineKeyframe();
  }

  drawPart(g, bounds, nullptr, style, is_loosely_active, is_hover);

  // Draw thumbnail
  if ((docPref().thumbnails.enabled() && m_zoom > 1) && image) {
    gfx::Rect thumb_bounds =
      gfx::Rect(bounds).shrink(
        skinTheme()->calcBorder(this, style));

    if (!thumb_bounds.isEmpty()) {
      if (os::SurfaceRef surface = thumb::get_cel_thumbnail(cel, thumb_bounds.size())) {
        const int t = std::clamp(thumb_bounds.w/8, 4, 16);
        draw_checkered_grid(g, thumb_bounds, gfx::Size(t, t), docPref());

        g->drawRgbaSurface(surface.get(),
                           thumb_bounds.center().x-surface->width()/2,
                           thumb_bounds.center().y-surface->height()/2);
      }
    }
  }

  // Draw decorators to link the activeCel with its links.
  if (data && data->activeIt != data->end)
    drawCelLinkDecorators(g, full_bounds, cel, frame, is_loosely_active, is_hover, data);

  // Draw 'z' if this cel has a custom z-index (non-zero)
  if (cel && cel->zIndex() != 0) {
    drawPart(g, bounds, nullptr, styles.timelineZindex(), is_loosely_active, is_hover);
  }
}

void Timeline::updateCelOverlayBounds(const Hit& hit)
{
  gfx::Rect rc;

  if (docPref().thumbnails.overlayEnabled() && hit.part == PART_CEL) {
    m_thumbnailsOverlayHit = hit;

    int max_size = headerBoxWidth() * docPref().thumbnails.overlaySize();
    int width, height;
    if (m_sprite->width() > m_sprite->height()) {
      width  = max_size;
      height = max_size * m_sprite->height() / m_sprite->width();
    }
    else {
      width  = max_size * m_sprite->width() / m_sprite->height();
      height = max_size;
    }

    gfx::Rect client_bounds = clientBounds();
    gfx::Point center = client_bounds.center();

    gfx::Rect bounds_cel = getPartBounds(m_thumbnailsOverlayHit);
    rc = gfx::Rect(
      bounds_cel.x + m_thumbnailsOverlayDirection.x,
      bounds_cel.y + m_thumbnailsOverlayDirection.y,
      width,
      height);

    if (!client_bounds.contains(rc)) {
      m_thumbnailsOverlayDirection = gfx::Point(
        bounds_cel.x < center.x ? (int)(frameBoxWidth()*1.0) : -width,
        bounds_cel.y < center.y ? (int)(frameBoxWidth()*0.5) : -height+(int)(frameBoxWidth()*0.5));
      rc.setOrigin(gfx::Point(
        bounds_cel.x + m_thumbnailsOverlayDirection.x,
        bounds_cel.y + m_thumbnailsOverlayDirection.y));
    }
  }
  else {
    rc = gfx::Rect(0, 0, 0, 0);
  }

  if (rc == m_thumbnailsOverlayBounds)
    return;

  if (!m_thumbnailsOverlayBounds.isEmpty())
    invalidateRect(gfx::Rect(m_thumbnailsOverlayBounds).offset(origin()));
  if (!rc.isEmpty())
    invalidateRect(gfx::Rect(rc).offset(origin()));

  m_thumbnailsOverlayVisible = !rc.isEmpty();
  m_thumbnailsOverlayBounds = rc;
}

void Timeline::drawCelOverlay(ui::Graphics* g)
{
  if (!m_thumbnailsOverlayVisible)
    return;

  Layer* layer = m_rows[m_thumbnailsOverlayHit.layer].layer();
  Cel* cel = layer->cel(m_thumbnailsOverlayHit.frame);
  if (!cel)
    return;

  Image* image = cel->image();
  if (!image)
    return;

  IntersectClip clip(g, m_thumbnailsOverlayBounds);
  if (!clip)
    return;

  gfx::Rect rc = m_sprite->bounds().fitIn(
    gfx::Rect(m_thumbnailsOverlayBounds).shrink(1));
  if (os::SurfaceRef surface = thumb::get_cel_thumbnail(cel, rc.size())) {
    draw_checkered_grid(g, rc, gfx::Size(8, 8)*ui::guiscale(), docPref());

    g->drawRgbaSurface(surface.get(),
                       rc.center().x-surface->width()/2,
                       rc.center().y-surface->height()/2);
    g->drawRect(gfx::rgba(0, 0, 0, 128), m_thumbnailsOverlayBounds);
  }
}

void Timeline::drawCelLinkDecorators(ui::Graphics* g, const gfx::Rect& bounds,
                                     Cel* cel, frame_t frame, bool is_active, bool is_hover,
                                     DrawCelData* data)
{
  auto& styles = skinTheme()->styles;
  ObjectId imageId = (*data->activeIt)->image()->id();

  ui::Style* style1 = nullptr;
  ui::Style* style2 = nullptr;

  // Links at the left or right side
  bool left = (data->firstLink != data->end ? frame > (*data->firstLink)->frame(): false);
  bool right = (data->lastLink != data->end ? frame < (*data->lastLink)->frame(): false);

  if (cel && cel->image()->id() == imageId) {
    if (left) {
      Cel* prevCel = m_layer->cel(cel->frame()-1);
      if (!prevCel || prevCel->image()->id() != imageId)
        style1 = styles.timelineLeftLink();
    }
    if (right) {
      Cel* nextCel = m_layer->cel(cel->frame()+1);
      if (!nextCel || nextCel->image()->id() != imageId)
        style2 = styles.timelineRightLink();
    }
  }
  else {
    if (left && right)
      style1 = styles.timelineBothLinks();
  }

  if (style1) drawPart(g, bounds, nullptr, style1, is_active, is_hover);
  if (style2) drawPart(g, bounds, nullptr, style2, is_active, is_hover);
}

void Timeline::drawTags(ui::Graphics* g)
{
  IntersectClip clip(g, getPartBounds(Hit(PART_TAGS)));
  if (!clip)
    return;

  SkinTheme* theme = skinTheme();
  auto& styles = theme->styles;

  g->fillRect(theme->colors.workspace(),
    gfx::Rect(
      0, font()->height(),
      clientBounds().w,
      theme->dimensions.timelineTagsAreaHeight()));

  // Draw active frame tag band
  if (m_hot.band >= 0 &&
      m_tagBands > 1 &&
      m_tagFocusBand < 0) {
    gfx::Rect bandBounds =
      getPartBounds(Hit(PART_TAG_BAND, -1, 0,
                        doc::NullId, m_hot.band));
    g->fillRect(theme->colors.timelineBandHighlight(), bandBounds);
  }

  int passes = (m_tagFocusBand >= 0 ? 2: 1);
  for (int pass=0; pass<passes; ++pass) {
    for (Tag* tag : m_sprite->tags()) {
      int band = -1;
      if (m_tagFocusBand >= 0) {
        auto it = m_tagBand.find(tag);
        if (it != m_tagBand.end()) {
          band = it->second;
          if ((pass == 0 && band == m_tagFocusBand) ||
              (pass == 1 && band != m_tagFocusBand))
            continue;
        }
      }

      doc::frame_t fromFrame = tag->fromFrame();
      doc::frame_t toFrame = tag->toFrame();
      if (m_resizeTagData.tag == tag->id()) {
        fromFrame = m_resizeTagData.from;
        toFrame = m_resizeTagData.to;
      }

      gfx::Rect bounds1 = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), fromFrame));
      gfx::Rect bounds2 = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), toFrame));
      gfx::Rect bounds = bounds1.createUnion(bounds2);
      gfx::Rect tagBounds = getPartBounds(Hit(PART_TAG, 0, 0, tag->id()));
      bounds.h = bounds.y2() - tagBounds.y2();
      bounds.y = tagBounds.y2();

      int dx = 0, dw = 0;
      if (m_dropTarget.outside &&
          m_dropTarget.hhit != DropTarget::HNone &&
          m_dropRange.type() == DocRange::kFrames) {
        switch (m_dropTarget.hhit) {
          case DropTarget::Before:
            if (m_dropRange.firstFrame() == tag->fromFrame()) {
              dx = +frameBoxWidth()/4;
              dw = -frameBoxWidth()/4;
            }
            else if (m_dropRange.firstFrame()-1 == tag->toFrame()) {
              dw = -frameBoxWidth()/4;
            }
            break;
          case DropTarget::After:
            if (m_dropRange.lastFrame() == tag->toFrame()) {
              dw = -frameBoxWidth()/4;
            }
            else if (m_dropRange.lastFrame()+1 == tag->fromFrame()) {
              dx = +frameBoxWidth()/4;
              dw = -frameBoxWidth()/4;
            }
            break;
        }
      }
      bounds.x += dx;
      bounds.w += dw;
      tagBounds.x += dx;

      const gfx::Color tagColor =
        (m_tagFocusBand < 0 || pass == 1) ?
        tag->color(): theme->colors.timelineBandBg();
      gfx::Color bg = tagColor;

      // Draw the tag braces
      drawTagBraces(g, bg, bounds, bounds);
      if ((m_clk.part == PART_TAG_LEFT && m_clk.tag == tag->id()) ||
          (m_clk.part != PART_TAG_LEFT &&
           m_hot.part == PART_TAG_LEFT && m_hot.tag == tag->id())) {
        if (m_clk.part == PART_TAG_LEFT)
          bg = color_utils::blackandwhite_neg(tagColor);
        else
          bg = Timeline::highlightColor(tagColor);
        drawTagBraces(g, bg, bounds, gfx::Rect(bounds.x, bounds.y,
                                               frameBoxWidth()/2, bounds.h));
      }
      else if ((m_clk.part == PART_TAG_RIGHT && m_clk.tag == tag->id()) ||
               (m_clk.part != PART_TAG_RIGHT &&
                m_hot.part == PART_TAG_RIGHT && m_hot.tag == tag->id())) {
        if (m_clk.part == PART_TAG_RIGHT)
          bg = color_utils::blackandwhite_neg(tagColor);
        else
          bg = Timeline::highlightColor(tagColor);
        drawTagBraces(g, bg, bounds,
                      gfx::Rect(bounds.x2()-frameBoxWidth()/2, bounds.y,
                                frameBoxWidth()/2, bounds.h));
      }

      // Draw tag text
      if (m_tagFocusBand < 0 || pass == 1) {
        bounds = tagBounds;

        if (m_clk.part == PART_TAG && m_clk.tag == tag->id())
          bg = color_utils::blackandwhite_neg(tagColor);
        else if (m_hot.part == PART_TAG && m_hot.tag == tag->id())
          bg = Timeline::highlightColor(tagColor);
        else
          bg = tagColor;
        g->fillRect(bg, bounds);

        bounds.y += 2*ui::guiscale();
        bounds.x += 2*ui::guiscale();
        g->drawText(
          tag->name(),
          color_utils::blackandwhite_neg(bg),
          gfx::ColorNone,
          bounds.origin());
      }
    }
  }

  // Draw button to expand/collapse the active band
  if (m_hot.band >= 0 && m_tagBands > 1) {
    gfx::Rect butBounds =
      getPartBounds(Hit(PART_TAG_SWITCH_BAND_BUTTON, -1, 0,
                        doc::NullId, m_hot.band));
    PaintWidgetPartInfo info;
    if (m_hot.part == PART_TAG_SWITCH_BAND_BUTTON) {
      info.styleFlags |= ui::Style::Layer::kMouse;
      if (hasCapture())
        info.styleFlags |= ui::Style::Layer::kSelected;
    }
    theme->paintWidgetPart(g, styles.timelineSwitchBandButton(),
                           butBounds, info);
  }
}

void Timeline::drawTagBraces(ui::Graphics* g,
                             gfx::Color tagColor,
                             const gfx::Rect& bounds,
                             const gfx::Rect& clipBounds)
{
  IntersectClip clip(g, clipBounds);
  if (clip) {
    SkinTheme* theme = skinTheme();
    auto& styles = theme->styles;
    for (auto& layer : styles.timelineLoopRange()->layers()) {
      if (layer.type() == Style::Layer::Type::kBackground ||
          layer.type() == Style::Layer::Type::kBackgroundBorder ||
          layer.type() == Style::Layer::Type::kBorder) {
        const_cast<Style::Layer*>(&layer)->setColor(tagColor);
      }
    }
    drawPart(g, bounds, nullptr, styles.timelineLoopRange());
  }
}

void Timeline::drawRangeOutline(ui::Graphics* g)
{
  auto& styles = skinTheme()->styles;

  IntersectClip clip(g, getRangeClipBounds(m_range).enlarge(outlineWidth()));
  if (!clip)
    return;

  PaintWidgetPartInfo info;
  info.styleFlags =
    (m_range.enabled() ? ui::Style::Layer::kFocus: 0) |
    (m_hot.part == PART_RANGE_OUTLINE ? ui::Style::Layer::kMouse: 0);

  gfx::Rect bounds = getPartBounds(Hit(PART_RANGE_OUTLINE));
  theme()->paintWidgetPart(
    g, styles.timelineRangeOutline(), bounds, info);

  Range drop = m_dropRange;
  gfx::Rect dropBounds = getRangeBounds(drop);

  switch (drop.type()) {

    case Range::kCels: {
      dropBounds = dropBounds.enlarge(outlineWidth());
      info.styleFlags = ui::Style::Layer::kFocus;
      theme()->paintWidgetPart(
        g, styles.timelineRangeOutline(), dropBounds, info);
      break;
    }

    case Range::kFrames: {
      int w = 5 * guiscale(); // TODO get width from the skin info

      if (m_dropTarget.hhit == DropTarget::Before)
        dropBounds.x -= w/2;
      else if (drop == m_range)
        dropBounds.x = dropBounds.x + getRangeBounds(m_range).w - w/2;
      else
        dropBounds.x = dropBounds.x + dropBounds.w - w/2;

      dropBounds.w = w;

      info.styleFlags = 0;
      theme()->paintWidgetPart(
        g, styles.timelineDropFrameDeco(), dropBounds, info);
      break;
    }

    case Range::kLayers: {
      int h = 5 * guiscale(); // TODO get height from the skin info

      if (m_dropTarget.vhit == DropTarget::Top)
        dropBounds.y -= h/2;
      else if (drop == m_range)
        dropBounds.y = dropBounds.y + getRangeBounds(m_range).h - h/2;
      else
        dropBounds.y = dropBounds.y + dropBounds.h - h/2;

      dropBounds.h = h;

      theme()->paintWidgetPart(
        g, styles.timelineDropLayerDeco(), dropBounds, info);
      break;
    }
  }
}

void Timeline::drawPaddings(ui::Graphics* g)
{
  auto& styles = skinTheme()->styles;

  gfx::Rect client = clientBounds();
  gfx::Rect bottomLayer;
  gfx::Rect lastFrame;
  int top = topHeight();

  if (!m_rows.empty()) {
    bottomLayer = getPartBounds(Hit(PART_ROW, firstLayer()));
    lastFrame = getPartBounds(Hit(PART_CEL, firstLayer(), this->lastFrame()));
  }
  else {
    bottomLayer = getPartBounds(Hit(PART_HEADER_LAYER));
    lastFrame = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), this->lastFrame()));
  }

  drawPart(g,
    gfx::Rect(lastFrame.x+lastFrame.w, client.y + top,
      client.w - (lastFrame.x+lastFrame.w),
      bottomLayer.y+bottomLayer.h),
    NULL, styles.timelinePaddingTr());

  drawPart(g,
    gfx::Rect(client.x, bottomLayer.y+bottomLayer.h,
      lastFrame.x+lastFrame.w - client.x, client.h - (bottomLayer.y+bottomLayer.h)),
    NULL, styles.timelinePaddingBl());

  drawPart(g,
    gfx::Rect(lastFrame.x+lastFrame.w, bottomLayer.y+bottomLayer.h,
      client.w - (lastFrame.x+lastFrame.w),
      client.h - (bottomLayer.y+bottomLayer.h)),
    NULL, styles.timelinePaddingBr());
}

gfx::Rect Timeline::getLayerHeadersBounds() const
{
  gfx::Rect rc = clientBounds();
  rc.w = separatorX();
  int h = topHeight() + headerBoxHeight();
  rc.y += h;
  rc.h -= h;
  return rc;
}

gfx::Rect Timeline::getFrameHeadersBounds() const
{
  gfx::Rect rc = clientBounds();
  rc.x += separatorX();
  rc.y += topHeight();
  rc.w -= separatorX();
  rc.h = headerBoxHeight();
  return rc;
}

gfx::Rect Timeline::getOnionskinFramesBounds() const
{
  DocumentPreferences& docPref = this->docPref();
  if (!docPref.onionskin.active())
    return gfx::Rect();

  frame_t firstFrame = m_frame - docPref.onionskin.prevFrames();
  frame_t lastFrame = m_frame + docPref.onionskin.nextFrames();

  if (firstFrame < this->firstFrame())
    firstFrame = this->firstFrame();

  if (lastFrame > this->lastFrame())
    lastFrame = this->lastFrame();

  return getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), firstFrame))
    .createUnion(getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), lastFrame)));
}

gfx::Rect Timeline::getCelsBounds() const
{
  gfx::Rect rc = clientBounds();
  rc.x += separatorX();
  rc.w -= separatorX();
  rc.y += headerBoxHeight() + topHeight();
  rc.h -= headerBoxHeight() + topHeight();
  return rc;
}

gfx::Rect Timeline::getPartBounds(const Hit& hit) const
{
  gfx::Rect bounds = clientBounds();
  int y = topHeight();

  switch (hit.part) {

    case PART_NOTHING:
      break;

    case PART_TOP:
      return gfx::Rect(bounds.x, bounds.y, bounds.w, y);

    case PART_SEPARATOR:
      return gfx::Rect(bounds.x + separatorX(), bounds.y + y,
                       separatorX() + m_separator_w, bounds.h - y);

    case PART_HEADER_EYE:
      return gfx::Rect(bounds.x + headerBoxWidth()*0, bounds.y + y,
                       headerBoxWidth(), headerBoxHeight());

    case PART_HEADER_PADLOCK:
      return gfx::Rect(bounds.x + headerBoxWidth()*1, bounds.y + y,
                       headerBoxWidth(), headerBoxHeight());

    case PART_HEADER_CONTINUOUS:
      return gfx::Rect(bounds.x + headerBoxWidth()*2, bounds.y + y,
                       headerBoxWidth(), headerBoxHeight());

    case PART_HEADER_GEAR:
      return gfx::Rect(bounds.x + headerBoxWidth()*3, bounds.y + y,
                       headerBoxWidth(), headerBoxHeight());

    case PART_HEADER_ONIONSKIN:
      return gfx::Rect(bounds.x + headerBoxWidth()*4, bounds.y + y,
                       headerBoxWidth(), headerBoxHeight());

    case PART_HEADER_LAYER:
      return gfx::Rect(bounds.x + headerBoxWidth()*5, bounds.y + y,
                       separatorX() - headerBoxWidth()*5, headerBoxHeight());

    case PART_HEADER_FRAME:
      return gfx::Rect(
        bounds.x + separatorX() + m_separator_w - 1
        + frameBoxWidth()*std::max(firstFrame(), hit.frame) - viewScroll().x,
        bounds.y + y, frameBoxWidth(), headerBoxHeight());

    case PART_ROW:
      if (validLayer(hit.layer)) {
        return gfx::Rect(bounds.x,
          bounds.y + y + headerBoxHeight() + layerBoxHeight()*(lastLayer()-hit.layer) - viewScroll().y,
          separatorX(), layerBoxHeight());
      }
      break;

    case PART_ROW_EYE_ICON:
      if (validLayer(hit.layer)) {
        return gfx::Rect(bounds.x,
          bounds.y + y + headerBoxHeight() + layerBoxHeight()*(lastLayer()-hit.layer) - viewScroll().y,
          headerBoxWidth(), layerBoxHeight());
      }
      break;

    case PART_ROW_PADLOCK_ICON:
      if (validLayer(hit.layer)) {
        return gfx::Rect(bounds.x + headerBoxWidth(),
          bounds.y + y + headerBoxHeight() + layerBoxHeight()*(lastLayer()-hit.layer) - viewScroll().y,
          headerBoxWidth(), layerBoxHeight());
      }
      break;

    case PART_ROW_CONTINUOUS_ICON:
      if (validLayer(hit.layer)) {
        return gfx::Rect(bounds.x + 2* headerBoxWidth(),
          bounds.y + y + headerBoxHeight() + layerBoxHeight()*(lastLayer()-hit.layer) - viewScroll().y,
          headerBoxWidth(), layerBoxHeight());
      }
      break;

    case PART_ROW_TEXT:
      if (validLayer(hit.layer)) {
        int x = headerBoxWidth()*3;
        return gfx::Rect(bounds.x + x,
          bounds.y + y + headerBoxHeight() + layerBoxHeight()*(lastLayer()-hit.layer) - viewScroll().y,
          separatorX() - x, layerBoxHeight());
      }
      break;

    case PART_CEL:
      if (validLayer(hit.layer) && hit.frame >= frame_t(0)) {
        return gfx::Rect(
          bounds.x + separatorX() + m_separator_w - 1 + frameBoxWidth()*hit.frame - viewScroll().x,
          bounds.y + y + headerBoxHeight() + layerBoxHeight()*(lastLayer()-hit.layer) - viewScroll().y,
          frameBoxWidth(), layerBoxHeight());
      }
      break;

    case PART_RANGE_OUTLINE: {
      gfx::Rect rc = getRangeBounds(m_range);
      int s = outlineWidth();
      rc.enlarge(gfx::Border(s-1, s-1, s, s));
      if (rc.x < bounds.x) rc.offset(s, 0).inflate(-s, 0);
      if (rc.y < bounds.y) rc.offset(0, s).inflate(0, -s);
      return rc;
    }

    case PART_TAG: {
      Tag* tag = hit.getTag();
      if (tag) {
        doc::frame_t fromFrame = tag->fromFrame();
        doc::frame_t toFrame = tag->toFrame();
        if (m_resizeTagData.tag == tag->id()) {
          fromFrame = m_resizeTagData.from;
          toFrame = m_resizeTagData.to;
        }

        gfx::Rect bounds1 = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), fromFrame));
        gfx::Rect bounds2 = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), toFrame));
        gfx::Rect bounds = bounds1.createUnion(bounds2);
        bounds.y -= skinTheme()->dimensions.timelineTagsAreaHeight();

        int textHeight = font()->height();
        bounds.y -= textHeight + 2*ui::guiscale();
        bounds.x += 3*ui::guiscale();
        bounds.w = font()->textLength(tag->name().c_str()) + 4*ui::guiscale();
        bounds.h = font()->height() + 2*ui::guiscale();

        if (m_tagFocusBand < 0) {
          auto it = m_tagBand.find(tag);
          if (it != m_tagBand.end()) {
            int dy = (m_tagBands-it->second-1)*oneTagHeight();
            bounds.y -= dy;
          }
        }

        return bounds;
      }
      break;
    }

    case PART_TAGS:
      return gfx::Rect(
        bounds.x + separatorX() + m_separator_w - 1,
        bounds.y,
        bounds.w - separatorX() - m_separator_w + 1, y);

    case PART_TAG_BAND:
      return gfx::Rect(
        bounds.x + separatorX() + m_separator_w - 1,
        bounds.y
        + (m_tagFocusBand < 0 ? oneTagHeight() * std::max(0, hit.band): 0),
        bounds.w - separatorX() - m_separator_w + 1,
        oneTagHeight());

    case PART_TAG_SWITCH_BUTTONS: {
      gfx::Size sz = theme()->calcSizeHint(
        this, skinTheme()->styles.timelineSwitchBandButton());

      return gfx::Rect(
        bounds.x + bounds.w - sz.w,
        bounds.y,
        sz.w, y);
    }

    case PART_TAG_SWITCH_BAND_BUTTON: {
      gfx::Size sz = theme()->calcSizeHint(
        this, skinTheme()->styles.timelineSwitchBandButton());

      return gfx::Rect(
        bounds.x + bounds.w - sz.w - 2*ui::guiscale(),
        bounds.y
        + (m_tagFocusBand < 0 ? oneTagHeight() * std::max(0, hit.band): 0)
        + oneTagHeight()/2 - sz.h/2,
        sz.w, sz.h);
    }

  }

  return gfx::Rect();
}

gfx::Rect Timeline::getRangeBounds(const Range& range) const
{
  gfx::Rect rc;
  switch (range.type()) {
    case Range::kNone:
      // Return empty rectangle
      break;
    case Range::kCels:
      for (auto layer : range.selectedLayers()) {
        layer_t layerIdx = getLayerIndex(layer);
        for (auto frame : range.selectedFrames())
          rc |= getPartBounds(Hit(PART_CEL, layerIdx, frame));
      }
      break;
    case Range::kFrames: {
      for (auto frame : range.selectedFrames()) {
        rc |= getPartBounds(Hit(PART_HEADER_FRAME, 0, frame));
        rc |= getPartBounds(Hit(PART_CEL, 0, frame));
      }
      break;
    }
    case Range::kLayers:
      for (auto layer : range.selectedLayers()) {
        layer_t layerIdx = getLayerIndex(layer);
        rc |= getPartBounds(Hit(PART_ROW_TEXT, layerIdx));
        rc |= getPartBounds(Hit(PART_CEL, layerIdx, m_sprite->lastFrame()));
      }
      break;
  }
  return rc;
}

gfx::Rect Timeline::getRangeClipBounds(const Range& range) const
{
  gfx::Rect celBounds = getCelsBounds();
  gfx::Rect clipBounds, unionBounds;
  switch (range.type()) {
    case Range::kCels:
      clipBounds = celBounds;
      break;
    case Range::kFrames: {
      clipBounds = getFrameHeadersBounds();

      unionBounds = (clipBounds | celBounds);
      clipBounds.y = unionBounds.y;
      clipBounds.h = unionBounds.h;
      break;
    }
    case Range::kLayers: {
      clipBounds = getLayerHeadersBounds();

      unionBounds = (clipBounds | celBounds);
      clipBounds.x = unionBounds.x;
      clipBounds.w = unionBounds.w;
      break;
    }
  }
  return clipBounds;
}

void Timeline::invalidateHit(const Hit& hit)
{
  if (hit.band >= 0) {
    Hit hit2 = hit;
    hit2.part = PART_TAG_BAND;
    invalidateRect(getPartBounds(hit2).offset(origin()));
  }

  invalidateRect(getPartBounds(hit).offset(origin()));
}

void Timeline::invalidateLayer(const Layer* layer)
{
  if (layer == nullptr)
    return;

  layer_t layerIdx = getLayerIndex(layer);
  if (layerIdx < firstLayer())
    return;

  gfx::Rect rc = getPartBounds(Hit(PART_ROW, layerIdx));
  gfx::Rect rcCels;
  rcCels |= getPartBounds(Hit(PART_CEL, layerIdx, firstFrame()));
  rcCels |= getPartBounds(Hit(PART_CEL, layerIdx, lastFrame()));
  rcCels &= getCelsBounds();
  rc |= rcCels;
  rc.offset(origin());
  invalidateRect(rc);
}

void Timeline::invalidateFrame(const frame_t frame)
{
  if (!validFrame(frame))
    return;

  gfx::Rect rc = getPartBounds(Hit(PART_HEADER_FRAME, -1, frame));
  gfx::Rect rcCels;
  rcCels |= getPartBounds(Hit(PART_CEL, firstLayer(), frame));
  rcCels |= getPartBounds(Hit(PART_CEL, lastLayer(), frame));
  rcCels &= getCelsBounds();
  rc |= rcCels;
  rc.offset(origin());
  invalidateRect(rc);
}

void Timeline::regenerateRows()
{
  ASSERT(m_document);
  ASSERT(m_sprite);

  size_t nlayers = 0;
  for_each_expanded_layer(
    m_sprite->root(),
    [&nlayers](Layer* layer, int level, LayerFlags flags) {
      ++nlayers;
    });

  if (m_rows.size() != nlayers) {
    if (nlayers > 0)
      m_rows.resize(nlayers);
    else
      m_rows.clear();
  }

  size_t i = 0;
  for_each_expanded_layer(
    m_sprite->root(),
    [&i, this](Layer* layer, int level, LayerFlags flags) {
      m_rows[i++] = Row(layer, level, flags);
    });

  regenerateTagBands();
  updateScrollBars();
}

void Timeline::regenerateTagBands()
{
  const bool oldEmptyTagBand = m_tagBand.empty();

  // TODO improve this implementation
  std::vector<unsigned char> tagsPerFrame(m_sprite->totalFrames(), 0);
  std::vector<Tag*> bands(4, nullptr);
  m_tagBand.clear();
  for (Tag* tag : m_sprite->tags()) {
    frame_t f = tag->fromFrame();

    int b=0;
    for (; b<int(bands.size()); ++b) {
      if (!bands[b] ||
          tag->fromFrame() > calcTagVisibleToFrame(bands[b])) {
        bands[b] = tag;
        m_tagBand[tag] = b;
        break;
      }
    }
    if (b == int(bands.size()))
      m_tagBand[tag] = tagsPerFrame[f];

    frame_t toFrame = calcTagVisibleToFrame(tag);
    if (toFrame >= frame_t(tagsPerFrame.size()))
      tagsPerFrame.resize(toFrame+1, 0);
    for (; f<=toFrame; ++f) {
      ASSERT(f < frame_t(tagsPerFrame.size()));
      if (tagsPerFrame[f] < 255)
        ++tagsPerFrame[f];
    }
  }

  const int oldVisibleBands = visibleTagBands();
  m_tagBands = 0;
  for (int i : tagsPerFrame)
    m_tagBands = std::max(m_tagBands, i);

  if (m_tagFocusBand >= m_tagBands)
    m_tagFocusBand = -1;

  if (oldVisibleBands != visibleTagBands() ||
      // This case is to re-layout the timeline when the AniControl
      // can use more/less space because there weren't tags and now
      // there tags, or viceversa.
      oldEmptyTagBand != m_tagBand.empty()) {
    layout();
  }
}

int Timeline::visibleTagBands() const
{
  if (m_tagBands > 1 && m_tagFocusBand == -1)
    return m_tagBands;
  else
    return 1;
}

void Timeline::updateScrollBars()
{
  gfx::Rect rc = bounds();
  m_viewportArea = getCelsBounds().offset(rc.origin());
  ui::setup_scrollbars(getScrollableSize(),
                       m_viewportArea, *this,
                       m_hbar,
                       m_vbar);

  setViewScroll(viewScroll());
}

void Timeline::updateByMousePos(ui::Message* msg, const gfx::Point& mousePos)
{
  Hit hit = hitTest(msg, mousePos);
  if (hasMouseOver())
    setCursor(msg, hit);
  setHot(hit);
}

Timeline::Hit Timeline::hitTest(ui::Message* msg, const gfx::Point& mousePos)
{
  Hit hit(PART_NOTHING, -1, -1);
  if (!m_document)
    return hit;

  if (m_clk.part == PART_SEPARATOR) {
    hit.part = PART_SEPARATOR;
  }
  else {
    gfx::Point scroll = viewScroll();
    int top = topHeight();

    hit.layer = lastLayer() -
      ((mousePos.y
        - top
        - headerBoxHeight()
        + scroll.y) / layerBoxHeight());

    hit.frame = frame_t((mousePos.x
        - separatorX()
        - m_separator_w
        + scroll.x) / frameBoxWidth());

    // Flag which indicates that we are in the are below the Background layer/last layer area
    if (hit.layer < 0)
      hit.veryBottom = true;

    if (hasCapture()) {
      hit.layer = std::clamp(hit.layer, firstLayer(), lastLayer());
      if (isMovingCel())
        hit.frame = std::max(firstFrame(), hit.frame);
      else
        hit.frame = std::clamp(hit.frame, firstFrame(), lastFrame());
    }
    else {
      if (hit.layer > lastLayer()) hit.layer = -1;
      if (hit.frame > lastFrame()) hit.frame = -1;
    }

    // Is the mouse over onionskin handles?
    gfx::Rect bounds = getOnionskinFramesBounds();
    if (!bounds.isEmpty() && gfx::Rect(bounds.x, bounds.y, 3, bounds.h).contains(mousePos)) {
      hit.part = PART_HEADER_ONIONSKIN_RANGE_LEFT;
    }
    else if (!bounds.isEmpty() && gfx::Rect(bounds.x+bounds.w-3, bounds.y, 3, bounds.h).contains(mousePos)) {
      hit.part = PART_HEADER_ONIONSKIN_RANGE_RIGHT;
    }
    // Is the mouse on the frame tags area?
    else if (getPartBounds(Hit(PART_TAGS)).contains(mousePos)) {
      // Mouse in switch band button
      if (hit.part == PART_NOTHING) {
        if (m_tagFocusBand < 0) {
          for (int band=0; band<m_tagBands; ++band) {
            gfx::Rect bounds = getPartBounds(
              Hit(PART_TAG_SWITCH_BAND_BUTTON, 0, 0,
                  doc::NullId, band));
            if (bounds.contains(mousePos)) {
              hit.part = PART_TAG_SWITCH_BAND_BUTTON;
              hit.band = band;
              break;
            }
          }
        }
        else {
          gfx::Rect bounds = getPartBounds(
            Hit(PART_TAG_SWITCH_BAND_BUTTON, 0, 0,
                doc::NullId, m_tagFocusBand));
          if (bounds.contains(mousePos)) {
            hit.part = PART_TAG_SWITCH_BAND_BUTTON;
            hit.band = m_tagFocusBand;
          }
        }
      }

      // Mouse in frame tags
      if (hit.part == PART_NOTHING) {
        for (Tag* tag : m_sprite->tags()) {
          const int band = m_tagBand[tag];

          // Skip unfocused bands
          if (m_tagFocusBand >= 0 &&
              m_tagFocusBand != band) {
            continue;
          }

          gfx::Rect tagBounds = getPartBounds(Hit(PART_TAG, 0, 0, tag->id()));
          if (tagBounds.contains(mousePos)) {
            hit.part = PART_TAG;
            hit.tag = tag->id();
            hit.band = band;
            break;
          }
          // Check if we are in the left/right handles to resize the tag
          else {
            gfx::Rect bounds1 = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), tag->fromFrame()));
            gfx::Rect bounds2 = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), tag->toFrame()));
            gfx::Rect bounds = bounds1.createUnion(bounds2);
            bounds.h = bounds.y2() - tagBounds.y2();
            bounds.y = tagBounds.y2();

            gfx::Rect bandBounds = getPartBounds(Hit(PART_TAG_BAND, 0, 0, doc::NullId, band));

            const int fw = frameBoxWidth()/2;
            if (gfx::Rect(bounds.x2()-fw, bounds.y, fw, bounds.h).contains(mousePos)) {
              hit.part = PART_TAG_RIGHT;
              hit.tag = tag->id();
              hit.band = band;
              // If we are in the band, we hit this tag, in other
              // case, we can try to hit other tag that might be a
              // better match.
              if (bandBounds.contains(mousePos))
                break;
            }
            else if (gfx::Rect(bounds.x, bounds.y, fw, bounds.h).contains(mousePos)) {
              hit.part = PART_TAG_LEFT;
              hit.tag = tag->id();
              hit.band = band;
              if (bandBounds.contains(mousePos))
                break;
            }
          }
        }
      }

      // Mouse in bands
      if (hit.part == PART_NOTHING) {
        if (m_tagFocusBand < 0) {
          for (int band=0; band<m_tagBands; ++band) {
            gfx::Rect bounds = getPartBounds(
              Hit(PART_TAG_BAND, 0, 0,
                  doc::NullId, band));
            if (bounds.contains(mousePos)) {
              hit.part = PART_TAG_BAND;
              hit.band = band;
              break;
            }
          }
        }
        else {
          gfx::Rect bounds = getPartBounds(
            Hit(PART_TAG_BAND, 0, 0,
                doc::NullId, m_tagFocusBand));
          if (bounds.contains(mousePos)) {
            hit.part = PART_TAG_BAND;
            hit.band = m_tagFocusBand;
          }
        }
      }
    }
    // Is the mouse on the separator.
    else if (mousePos.x > separatorX()-4 &&
             mousePos.x <= separatorX()) {
      hit.part = PART_SEPARATOR;
    }
    // Is the mouse on the headers?
    else if (mousePos.y >= top && mousePos.y < top+headerBoxHeight()) {
      if (mousePos.x < separatorX()) {
        if (getPartBounds(Hit(PART_HEADER_EYE)).contains(mousePos))
          hit.part = PART_HEADER_EYE;
        else if (getPartBounds(Hit(PART_HEADER_PADLOCK)).contains(mousePos))
          hit.part = PART_HEADER_PADLOCK;
        else if (getPartBounds(Hit(PART_HEADER_CONTINUOUS)).contains(mousePos))
          hit.part = PART_HEADER_CONTINUOUS;
        else if (getPartBounds(Hit(PART_HEADER_GEAR)).contains(mousePos))
          hit.part = PART_HEADER_GEAR;
        else if (getPartBounds(Hit(PART_HEADER_ONIONSKIN)).contains(mousePos))
          hit.part = PART_HEADER_ONIONSKIN;
        else if (getPartBounds(Hit(PART_HEADER_LAYER)).contains(mousePos))
          hit.part = PART_HEADER_LAYER;
      }
      else {
        hit.part = PART_HEADER_FRAME;
      }
    }
    // Activate a flag in case that the hit is in the header area (AniControls and tags).
    else if (mousePos.y < top+headerBoxHeight())
      hit.part = PART_TOP;
    // Is the mouse on a layer's label?
    else if (mousePos.x < separatorX()) {
      if (getPartBounds(Hit(PART_ROW_EYE_ICON, hit.layer)).contains(mousePos))
        hit.part = PART_ROW_EYE_ICON;
      else if (getPartBounds(Hit(PART_ROW_PADLOCK_ICON, hit.layer)).contains(mousePos))
        hit.part = PART_ROW_PADLOCK_ICON;
      else if (getPartBounds(Hit(PART_ROW_CONTINUOUS_ICON, hit.layer)).contains(mousePos))
        hit.part = PART_ROW_CONTINUOUS_ICON;
      else if (getPartBounds(Hit(PART_ROW_TEXT, hit.layer)).contains(mousePos))
        hit.part = PART_ROW_TEXT;
      else
        hit.part = PART_ROW;
    }
    else if (validLayer(hit.layer) && validFrame(hit.frame)) {
      hit.part = PART_CEL;
    }
    else
      hit.part = PART_NOTHING;

    if (!hasCapture()) {
      gfx::Rect outline = getPartBounds(Hit(PART_RANGE_OUTLINE));
      if (outline.contains(mousePos)) {
        auto mouseMsg = dynamic_cast<MouseMessage*>(msg);

        if (// With Ctrl and Alt key we can drag the range from any place (not necessary from the outline.
            is_copy_key_pressed(msg) ||
            // Drag with right-click
            (m_state == STATE_STANDBY &&
             mouseMsg &&
             mouseMsg->right()) ||
            // Drag with left-click only if we are inside the range edges
            !gfx::Rect(outline).shrink(2*outlineWidth()).contains(mousePos)) {
          hit.part = PART_RANGE_OUTLINE;
        }
      }
    }
  }

  return hit;
}

Timeline::Hit Timeline::hitTestCel(const gfx::Point& mousePos)
{
  Hit hit(PART_NOTHING, -1, -1);
  if (!m_document)
    return hit;

  gfx::Point scroll = viewScroll();
  int top = topHeight();

  hit.layer = lastLayer() - (
    (mousePos.y
     - top
     - headerBoxHeight()
     + scroll.y) / layerBoxHeight());

  hit.frame = frame_t((mousePos.x
                       - separatorX()
                       - m_separator_w
                       + scroll.x) / frameBoxWidth());

  hit.layer = std::clamp(hit.layer, firstLayer(), lastLayer());
  hit.frame = std::max(firstFrame(), hit.frame);

  return hit;
}

void Timeline::setHot(const Hit& hit)
{
  // If the part, layer or frame change.
  if (m_hot != hit) {
    // Invalidate the whole control.
    if (isMovingCel()) {
      invalidate();
    }
    // Invalidate the old and new 'hot' thing.
    else {
      invalidateHit(m_hot);
      invalidateHit(hit);
    }

    // Change the new 'hot' thing.
    m_hot = hit;
  }
}

void Timeline::updateStatusBar(ui::Message* msg)
{
  if (!hasMouse())
    return;

  StatusBar* sb = StatusBar::instance();

  if (m_state == STATE_MOVING_RANGE && msg) {
    const char* verb = is_copy_key_pressed(msg) ? "Copy": "Move";

    switch (m_range.type()) {

      case Range::kCels:
        sb->setStatusText(0, fmt::format("{} cels", verb));
        return;

      case Range::kFrames:
        if (validFrame(m_hot.frame)) {
          if (m_dropTarget.hhit == DropTarget::Before) {
            sb->setStatusText(0, fmt::format("{} before frame {}",
                                             verb, int(m_dropRange.firstFrame()+1)));
            return;
          }
          else if (m_dropTarget.hhit == DropTarget::After) {
            sb->setStatusText(0, fmt::format("{} after frame {}",
                                             verb, int(m_dropRange.lastFrame()+1)));
            return;
          }
        }
        break;

      case Range::kLayers: {
        layer_t firstLayer;
        layer_t lastLayer;
        if (!selectedLayersBounds(m_dropRange.selectedLayers(),
                                  &firstLayer, &lastLayer))
          break;

        if (m_dropTarget.vhit == DropTarget::VeryBottom) {
          sb->setStatusText(0, fmt::format("{} at the very bottom", verb));
          return;
        }

        layer_t layerIdx = -1;
        if (m_dropTarget.vhit == DropTarget::Bottom ||
            m_dropTarget.vhit == DropTarget::FirstChild)
          layerIdx = firstLayer;
        else if (m_dropTarget.vhit == DropTarget::Top)
          layerIdx = lastLayer;

        Layer* layer = (validLayer(layerIdx) ? m_rows[layerIdx].layer(): nullptr);
        if (layer) {
          switch (m_dropTarget.vhit) {
            case DropTarget::Bottom:
              sb->setStatusText(0, fmt::format("{} below layer '{}'",
                                               verb, layer->name()));
              return;
            case DropTarget::Top:
              sb->setStatusText(0, fmt::format("{} above layer '{}'",
                                               verb, layer->name()));
              return;
            case DropTarget::FirstChild:
              sb->setStatusText(0, fmt::format("{} as first child of group '{}'",
                                               verb, layer->name()));
              return;
          }
        }
        break;
      }

    }
  }
  else {
    Layer* layer = (validLayer(m_hot.layer) ? m_rows[m_hot.layer].layer():
                                              nullptr);

    switch (m_hot.part) {

      case PART_HEADER_ONIONSKIN: {
        sb->setStatusText(0, fmt::format("Onionskin is {}",
                                         docPref().onionskin.active()
                                         ? "enabled": "disabled"));
        return;
      }

      case PART_ROW_TEXT:
        if (layer != NULL) {
          sb->setStatusText(
            0, fmt::format("{} '{}' [{}{}]",
                           layer->isReference() ? "Reference layer": "Layer",
                           layer->name(),
                           layer->isVisible() ? "visible": "hidden",
                           layer->isEditable() ? "": " locked"));
          return;
        }
        break;

      case PART_ROW_EYE_ICON:
        if (layer != NULL) {
          sb->setStatusText(
            0, fmt::format("Layer '{}' is {}",
                           layer->name(),
                           layer->isVisible() ? "visible": "hidden"));
          return;
        }
        break;

      case PART_ROW_PADLOCK_ICON:
        if (layer != NULL) {
          sb->setStatusText(
            0, fmt::format("Layer '{}' is {}",
                           layer->name(),
                           layer->isEditable() ? "unlocked (editable)":
                                                 "locked (read-only)"));
          return;
        }
        break;

      case PART_ROW_CONTINUOUS_ICON:
        if (layer) {
          if (layer->isImage())
            sb->setStatusText(
              0, fmt::format("Layer '{}' is {} ({})",
                             layer->name(),
                             layer->isContinuous() ? "continuous":
                                                     "discontinuous",
                             layer->isContinuous() ? "prefer linked cels/frames":
                                                     "prefer individual cels/frames"));
          else if (layer->isGroup())
            sb->setStatusText(
              0, fmt::format("Group '{}'", layer->name()));
          return;
        }
        break;

      case PART_HEADER_FRAME:
      case PART_CEL:
      case PART_TAG: {
        frame_t frame = m_frame;
        if (validFrame(m_hot.frame))
          frame = m_hot.frame;

        updateStatusBarForFrame(
          frame,
          m_hot.getTag(),
          (layer ? layer->cel(frame) : nullptr));
        return;
      }
    }
  }

  sb->showDefaultText();
}

void Timeline::updateStatusBarForFrame(const frame_t frame,
                                       const Tag* tag,
                                       const Cel* cel)
{
  if (!m_sprite)
    return;

  char buf[256] = { 0 };
  frame_t base = docPref().timeline.firstFrame();
  frame_t firstFrame = frame;
  frame_t lastFrame = frame;

  if (tag) {
    firstFrame = tag->fromFrame();
    lastFrame = tag->toFrame();
  }
  else if (m_range.enabled() &&
           m_range.frames() > 1) {
    firstFrame = m_range.firstFrame();
    lastFrame = m_range.lastFrame();
  }

  std::sprintf(
    buf+std::strlen(buf), ":frame: %d",
    base+frame);
  if (firstFrame != lastFrame) {
    std::sprintf(
      buf+std::strlen(buf), " [%d...%d]",
      int(base+firstFrame),
      int(base+lastFrame));
  }

  std::sprintf(
    buf+std::strlen(buf), " :clock: %s",
    human_readable_time(m_sprite->frameDuration(frame)).c_str());
  if (firstFrame != lastFrame) {
    std::sprintf(
      buf+std::strlen(buf), " [%s]",
      tag ?
      human_readable_time(tagFramesDuration(tag)).c_str():
      human_readable_time(selectedFramesDuration()).c_str());
  }
  if (m_sprite->totalFrames() > 1)
    std::sprintf(
      buf+std::strlen(buf), "/%s",
      human_readable_time(m_sprite->totalAnimationDuration()).c_str());

  if (cel) {
    std::sprintf(
      buf+std::strlen(buf), " Cel :pos: %d %d :size: %d %d",
      cel->bounds().x, cel->bounds().y,
      cel->bounds().w, cel->bounds().h);

    if (cel->links() > 0) {
      std::sprintf(
        buf+std::strlen(buf), " Links %d",
        int(cel->links()));
    }
  }

  StatusBar::instance()->setStatusText(0, buf);
}

void Timeline::showCel(layer_t layer, frame_t frame)
{
  gfx::Point scroll = viewScroll();

  gfx::Rect viewport = m_viewportArea;

  // Add the horizontal bar space to the viewport area if the viewport
  // is not big enough to show one cel.
  if (m_hbar.isVisible() && viewport.h < layerBoxHeight())
    viewport.h += m_vbar.getBarWidth();

  gfx::Rect celBounds(
    viewport.x + frameBoxWidth()*frame - scroll.x,
    viewport.y + layerBoxHeight()*(lastLayer() - layer) - scroll.y,
    frameBoxWidth(), layerBoxHeight());

  const bool isPlaying = m_editor->isPlaying();

  // Here we use <= instead of < to avoid jumping between this
  // condition and the "else if" one when we are playing the
  // animation.
  if (celBounds.x <= viewport.x) {
    scroll.x -= viewport.x - celBounds.x;
    if (isPlaying)
      scroll.x -= viewport.w/2 - celBounds.w/2;
  }
  else if (celBounds.x2() > viewport.x2()) {
    scroll.x += celBounds.x2() - viewport.x2();
    if (isPlaying)
      scroll.x += viewport.w/2 - celBounds.w/2;
  }

  if (celBounds.y <= viewport.y) {
    scroll.y -= viewport.y - celBounds.y;
  }
  else if (celBounds.y2() > viewport.y2()) {
    scroll.y += celBounds.y2() - viewport.y2();
  }

  setViewScroll(scroll);
}

void Timeline::showCurrentCel()
{
  layer_t layer = getLayerIndex(m_layer);
  if (layer >= firstLayer())
    showCel(layer, m_frame);
}

void Timeline::focusTagBand(int band)
{
  if (m_tagFocusBand < 0) {
    m_tagFocusBand = band;
  }
  else {
    m_tagFocusBand = -1;
  }
}

void Timeline::cleanClk()
{
  invalidateHit(m_clk);
  m_clk = Hit(PART_NOTHING);
}

gfx::Size Timeline::getScrollableSize() const
{
  if (m_sprite) {
    return gfx::Size(
      m_sprite->totalFrames() * frameBoxWidth() + getCelsBounds().w/2,
      (m_rows.size()+1) * layerBoxHeight());
  }
  else
    return gfx::Size(0, 0);
}

gfx::Point Timeline::getMaxScrollablePos() const
{
  if (m_sprite) {
    gfx::Size size = getScrollableSize();
    int max_scroll_x = size.w - getCelsBounds().w + 1*guiscale();
    int max_scroll_y = size.h - getCelsBounds().h + 1*guiscale();
    max_scroll_x = std::max(0, max_scroll_x);
    max_scroll_y = std::max(0, max_scroll_y);
    return gfx::Point(max_scroll_x, max_scroll_y);
  }
  else
    return gfx::Point(0, 0);
}

bool Timeline::allLayersVisible()
{
  for (Layer* topLayer : m_sprite->root()->layers())
    if (!topLayer->isVisible())
      return false;

  return true;
}

bool Timeline::allLayersInvisible()
{
  for (Layer* topLayer : m_sprite->root()->layers())
    if (topLayer->isVisible())
      return false;

  return true;
}

bool Timeline::allLayersLocked()
{
  for (Layer* topLayer : m_sprite->root()->layers())
    if (topLayer->isEditable())
      return false;

  return true;
}

bool Timeline::allLayersUnlocked()
{
  for (Layer* topLayer : m_sprite->root()->layers())
    if (!topLayer->isEditable())
      return false;

  return true;
}

bool Timeline::allLayersContinuous()
{
  for (size_t i=0; i<m_rows.size(); i++)
    if (!m_rows[i].layer()->isContinuous())
      return false;

  return true;
}

bool Timeline::allLayersDiscontinuous()
{
  for (size_t i=0; i<m_rows.size(); i++)
    if (m_rows[i].layer()->isContinuous())
      return false;

  return true;
}

layer_t Timeline::getLayerIndex(const Layer* layer) const
{
  for (int i=0; i<(int)m_rows.size(); i++)
    if (m_rows[i].layer() == layer)
      return i;

  return -1;
}

bool Timeline::isLayerActive(const layer_t layerIndex) const
{
  if (layerIndex == getLayerIndex(m_layer))
    return true;
  else
    return m_range.contains(m_rows[layerIndex].layer());
}

bool Timeline::isFrameActive(const frame_t frame) const
{
  if (frame == m_frame)
    return true;
  else
    return m_range.contains(frame);
}

bool Timeline::isCelActive(const layer_t layerIdx, const frame_t frame) const
{
  if (m_range.enabled())
    return m_range.contains(m_rows[layerIdx].layer(), frame);
  else
    return (layerIdx == getLayerIndex(m_layer) &&
            frame == m_frame);
}

bool Timeline::isCelLooselyActive(const layer_t layerIdx, const frame_t frame) const
{
  if (m_range.enabled())
    return (m_range.contains(m_rows[layerIdx].layer()) ||
            m_range.contains(frame));
  else
    return (layerIdx == getLayerIndex(m_layer) ||
            frame == m_frame);
}

void Timeline::dropRange(DropOp op)
{
  bool copy = (op == Timeline::kCopy);
  Range newFromRange;
  DocRangePlace place = kDocRangeAfter;
  Range dropRange = m_dropRange;
  bool outside = m_dropTarget.outside;

  switch (m_range.type()) {

    case Range::kFrames:
      if (m_dropTarget.hhit == DropTarget::Before)
        place = kDocRangeBefore;
      break;

    case Range::kLayers:
      switch (m_dropTarget.vhit) {
        case DropTarget::Bottom:
          place = kDocRangeBefore;
          break;
        case DropTarget::FirstChild:
          place = kDocRangeFirstChild;
          break;
        case DropTarget::VeryBottom:
          place = kDocRangeBefore;
          {
            Layer* layer = m_sprite->root()->firstLayer();
            dropRange.clearRange();
            dropRange.startRange(layer, -1, Range::kLayers);
            dropRange.endRange(layer, -1);
          }
          break;
      }
      break;
  }

  prepareToMoveRange();

  try {
    TagsHandling tagsHandling = (outside ? kFitOutsideTags:
                                           kFitInsideTags);

    invalidateRange();
    if (copy)
      newFromRange = copy_range(m_document, m_range, dropRange,
                                place, tagsHandling);
    else
      newFromRange = move_range(m_document, m_range, dropRange,
                                place, tagsHandling);

    // If we drop a cel in the same frame (but in another layer),
    // document views are not updated, so we are forcing the updating of
    // all views.
    m_document->notifyGeneralUpdate();

    moveRange(newFromRange);

    invalidateRange();

    if (m_range.type() == Range::kFrames &&
        m_sprite &&
        !m_sprite->tags().empty()) {
      invalidateRect(getFrameHeadersBounds().offset(origin()));
    }

    // Update the sprite position after the command was executed
    // TODO improve this workaround
    Cmd* cmd = m_document->undoHistory()->lastExecutedCmd();
    if (auto cmdTx = dynamic_cast<CmdTransaction*>(cmd))
      cmdTx->updateSpritePositionAfter();
  }
  catch (const std::exception& ex) {
    Console::showException(ex);
  }
}

gfx::Size Timeline::visibleSize() const
{
  return getCelsBounds().size();
}

gfx::Point Timeline::viewScroll() const
{
  return gfx::Point(m_hbar.getPos(), m_vbar.getPos());
}

void Timeline::setViewScroll(const gfx::Point& pt)
{
  const gfx::Point oldScroll = viewScroll();
  const gfx::Point maxPos = getMaxScrollablePos();
  gfx::Point newScroll = pt;
  newScroll.x = std::clamp(newScroll.x, 0, maxPos.x);
  newScroll.y = std::clamp(newScroll.y, 0, maxPos.y);

  if (newScroll.y != oldScroll.y) {
    gfx::Rect rc;
    rc = getLayerHeadersBounds();
    rc.offset(origin());
    invalidateRect(rc);
  }

  if (newScroll != oldScroll) {
    gfx::Rect rc;
    if (m_tagBands > 0)
      rc |= getPartBounds(Hit(PART_TAG_BAND));
    if (m_range.enabled())
      rc |= getRangeBounds(m_range).enlarge(outlineWidth());
    rc |= getFrameHeadersBounds();
    rc |= getCelsBounds();
    rc.offset(origin());
    invalidateRect(rc);
  }

  m_hbar.setPos(newScroll.x);
  m_vbar.setPos(newScroll.y);
}


void Timeline::lockRange()
{
  ++m_rangeLocks;
}

void Timeline::unlockRange()
{
  --m_rangeLocks;
}

void Timeline::updateDropRange(const gfx::Point& pt)
{
  const DropTarget oldDropTarget = m_dropTarget;
  m_dropTarget.hhit = DropTarget::HNone;
  m_dropTarget.vhit = DropTarget::VNone;
  m_dropTarget.outside = false;

  if (m_state != STATE_MOVING_RANGE) {
    m_dropRange.clearRange();
    return;
  }

  switch (m_range.type()) {

    case Range::kCels:
      m_dropRange = m_range;
      m_dropRange.displace(m_hot.layer - m_clk.layer,
                           m_hot.frame - m_clk.frame);
      break;

    case Range::kFrames:
    case Range::kLayers:
      m_dropRange.clearRange();
      m_dropRange.startRange(m_rows[m_hot.layer].layer(), m_hot.frame, m_range.type());
      m_dropRange.endRange(m_rows[m_hot.layer].layer(), m_hot.frame);
      break;
  }

  gfx::Rect bounds = getRangeBounds(m_dropRange);

  if (pt.x < bounds.x + bounds.w/2) {
    m_dropTarget.hhit = DropTarget::Before;
    m_dropTarget.outside = (pt.x < bounds.x+2);
  }
  else {
    m_dropTarget.hhit = DropTarget::After;
    m_dropTarget.outside = (pt.x >= bounds.x2()-2);
  }

  if (m_hot.veryBottom)
    m_dropTarget.vhit = DropTarget::VeryBottom;
  else if (pt.y < bounds.y + bounds.h/2)
    m_dropTarget.vhit = DropTarget::Top;
  // Special drop target for expanded groups
  else if (m_range.type() == Range::kLayers &&
           m_hot.layer >= 0 &&
           m_hot.layer < int(m_rows.size()) &&
           m_rows[m_hot.layer].layer()->isGroup() &&
           static_cast<LayerGroup*>(m_rows[m_hot.layer].layer())->isExpanded()) {
    m_dropTarget.vhit = DropTarget::FirstChild;
  }
  else {
    m_dropTarget.vhit = DropTarget::Bottom;
  }

  if (oldDropTarget != m_dropTarget)
    invalidate();
}

void Timeline::clearClipboardRange()
{
  Doc* clipboard_document;
  DocRange clipboard_range;
  auto clipboard = Clipboard::instance();
  clipboard->getDocumentRangeInfo(
    &clipboard_document,
    &clipboard_range);

  if (!m_document || clipboard_document != m_document)
    return;

  clipboard->clearContent();
  m_clipboard_timer.stop();
}

void Timeline::invalidateRange()
{
  if (m_range.enabled()) {
    for (const Layer* layer : m_range.selectedLayers())
      invalidateLayer(layer);
    for (const frame_t frame : m_range.selectedFrames())
      invalidateFrame(frame);

    invalidateHit(Hit(PART_RANGE_OUTLINE));
  }
}

void Timeline::clearAndInvalidateRange()
{
  if (m_range.enabled()) {
    notify_observers(&TimelineObserver::onBeforeRangeChanged, this);

    invalidateRange();
    m_range.clearRange();
  }
}

DocumentPreferences& Timeline::docPref() const
{
  return Preferences::instance().document(m_document);
}

skin::SkinTheme* Timeline::skinTheme() const
{
  return SkinTheme::get(this);
}

int Timeline::headerBoxWidth() const
{
  return skinTheme()->dimensions.timelineBaseSize();
}

int Timeline::headerBoxHeight() const
{
  return skinTheme()->dimensions.timelineBaseSize();
}

int Timeline::layerBoxHeight() const
{
  return int(zoom()*skinTheme()->dimensions.timelineBaseSize());
}

int Timeline::frameBoxWidth() const
{
  return int(zoom()*skinTheme()->dimensions.timelineBaseSize());
}

int Timeline::outlineWidth() const
{
  return skinTheme()->dimensions.timelineOutlineWidth();
}

int Timeline::oneTagHeight() const
{
  return
    font()->height() +
    2*ui::guiscale() +
    skinTheme()->dimensions.timelineTagsAreaHeight();
}

double Timeline::zoom() const
{
  if (docPref().thumbnails.enabled())
    return m_zoom;
  else
    return 1.0;
}

// Returns the last frame where the frame tag (or frame tag label)
// is visible in the timeline.
int Timeline::calcTagVisibleToFrame(Tag* tag) const
{
  return
    std::max(tag->toFrame(),
             tag->fromFrame() +
             font()->textLength(tag->name())/frameBoxWidth());
}

int Timeline::topHeight() const
{
  int h = 0;
  if (m_document && m_sprite) {
    h += skinTheme()->dimensions.timelineTopBorder();
    h += oneTagHeight() * visibleTagBands();
  }
  return h;
}

void Timeline::onNewInputPriority(InputChainElement* element,
                                  const ui::Message* msg)
{
  // It looks like the user wants to execute commands targetting the
  // ColorBar instead of the Timeline. Here we disable the selected
  // range, so commands like Clear, Copy, Cut, etc. don't target the
  // Timeline and they are sent to the active sprite editor.
  //
  // If the Workspace is selected (an sprite Editor), maybe the user
  // want to move the X/Y position of all cels in the Timeline range.
  // That is why we don't disable the range in this case.
  Workspace* workspace = dynamic_cast<Workspace*>(element);
  if (!workspace) {
    // With Ctrl or Shift we can combine ColorBar selection + Timeline
    // selection.
    if (msg && (msg->ctrlPressed() || msg->shiftPressed()))
      return;

    if (element != this && m_rangeLocks == 0) {
      if (!Preferences::instance().timeline.keepSelection()) {
        m_range.clearRange();
        invalidate();
      }
    }
  }
}

bool Timeline::onCanCut(Context* ctx)
{
  return false;                 // TODO
}

bool Timeline::onCanCopy(Context* ctx)
{
  return
    m_range.enabled() &&
    ctx->checkFlags(ContextFlags::HasActiveDocument);
}

bool Timeline::onCanPaste(Context* ctx)
{
  return
    (ctx->clipboard()->format() == ClipboardFormat::DocRange &&
     ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable));
}

bool Timeline::onCanClear(Context* ctx)
{
  return (m_document && m_sprite && m_range.enabled());
}

bool Timeline::onCut(Context* ctx)
{
  return false;                 // TODO
}

bool Timeline::onCopy(Context* ctx)
{
  if (m_range.enabled()) {
    const ContextReader reader(ctx);
    if (reader.document()) {
      ctx->clipboard()->copyRange(reader, m_range);
      return true;
    }
  }
  return false;
}

bool Timeline::onPaste(Context* ctx)
{
  auto clipboard = ctx->clipboard();
  if (clipboard->format() == ClipboardFormat::DocRange) {
    // After a paste action, we just remove the marching ant
    // (following paste commands will paste the same source range, but
    // we just disable the marching ants effect).
    if (m_clipboard_timer.isRunning()) {
      m_clipboard_timer.stop();
      m_redrawMarchingAntsOnly = false;
      invalidate();
    }
    clipboard->paste(ctx, true);
    return true;
  }
  else
    return false;
}

bool Timeline::onClear(Context* ctx)
{
  if (!m_document ||
      !m_sprite ||
      !m_range.enabled() ||
      // If the mask is visible the delete command will be handled by
      // the Editor
      m_document->isMaskVisible())
    return false;

  Command* cmd = nullptr;

  switch (m_range.type()) {
    case DocRange::kCels:
      cmd = Commands::instance()->byId(CommandId::ClearCel());
      break;
    case DocRange::kFrames:
      cmd = Commands::instance()->byId(CommandId::RemoveFrame());
      break;
    case DocRange::kLayers:
      cmd = Commands::instance()->byId(CommandId::RemoveLayer());
      break;
  }

  if (cmd) {
    ctx->executeCommand(cmd);
    return true;
  }
  else
    return false;
}

void Timeline::onCancel(Context* ctx)
{
  if (m_rangeLocks == 0)
    clearAndInvalidateRange();

  clearClipboardRange();
  invalidate();
}

int Timeline::tagFramesDuration(const Tag* tag) const
{
  ASSERT(m_sprite);
  ASSERT(tag);

  int duration = 0;
  for (frame_t f=tag->fromFrame();
       f<tag->toFrame(); ++f) {
    duration += m_sprite->frameDuration(f);
  }
  return duration;
}

int Timeline::selectedFramesDuration() const
{
  ASSERT(m_sprite);

  int duration = 0;
  for (frame_t f=0; f<m_sprite->totalFrames(); ++f) {
    if (isFrameActive(f))
      duration += m_sprite->frameDuration(f);
  }
  return duration; // TODO cache this value
}

void Timeline::setLayerVisibleFlag(const layer_t l, const bool state)
{
  if (!validLayer(l))
    return;

  Row& row = m_rows[l];
  Layer* layer = row.layer();
  ASSERT(layer);
  if (!layer)
    return;

  bool redrawEditors = false;
  bool regenRows = false;

  if (layer->isVisible() != state) {
    layer->setVisible(state);
    redrawEditors = true;

    // Regenerate rows because might change the flag of the children
    // (the flag is propagated to the children in m_inheritedFlags).
    if (layer->isGroup() && layer->isExpanded()) {
      regenRows = true;
    }

    // Show parents too
    if (!row.parentVisible() && state) {
      layer = layer->parent();
      while (layer) {
        if (!layer->isVisible()) {
          layer->setVisible(true);
          regenRows = true;
          redrawEditors = true;
        }
        layer = layer->parent();
      }
    }
  }

  if (regenRows) {
    regenerateRows();
    invalidate();
  }

  if (redrawEditors)
    m_document->notifyGeneralUpdate();
}

void Timeline::setLayerEditableFlag(const layer_t l, const bool state)
{
  Row& row = m_rows[l];
  Layer* layer = row.layer();
  ASSERT(layer);
  if (!layer)
    return;

  bool regenRows = false;

  if (layer->isEditable() != state) {
    layer->setEditable(state);

    if (layer->isGroup() && layer->isExpanded())
      regenRows = true;

    // Make parents editable too
    if (!row.parentEditable() && state) {
      layer = layer->parent();
      while (layer) {
        if (!layer->isEditable()) {
          layer->setEditable(true);
          regenRows = true;
        }
        layer = layer->parent();
      }
    }
  }

  if (regenRows) {
    regenerateRows();
    invalidate();
  }
}

void Timeline::setLayerContinuousFlag(const layer_t l, const bool state)
{
  Layer* layer = m_rows[l].layer();
  ASSERT(layer);
  if (!layer)
    return;

  if (layer->isImage() && layer->isContinuous() != state) {
    layer->setContinuous(state);
    invalidate();
  }
}

void Timeline::setLayerCollapsedFlag(const layer_t l, const bool state)
{
  Layer* layer = m_rows[l].layer();
  ASSERT(layer);
  if (!layer)
    return;

  if (layer->isGroup() && layer->isCollapsed() != state) {
    layer->setCollapsed(state);
    m_document->notifyLayerGroupCollapseChange(layer);
  }
}

int Timeline::separatorX() const
{
  return std::clamp(m_separator_x,
                    headerBoxWidth(),
                    std::max(bounds().w-guiscale(),
                             headerBoxWidth()));
}

void Timeline::setSeparatorX(int newValue)
{
  m_separator_x = std::max(0, newValue);
}

//static
gfx::Color Timeline::highlightColor(const gfx::Color color)
{
  int r, g, b;
  r = gfx::getr(color)+64; // TODO make this customizable in the theme XML?
  g = gfx::getg(color)+64;
  b = gfx::getb(color)+64;
  r = std::clamp(r, 0, 255);
  g = std::clamp(g, 0, 255);
  b = std::clamp(b, 0, 255);
  return gfx::rgba(r, g, b, gfx::geta(color));
}

} // namespace app
