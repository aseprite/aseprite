// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/timeline/timeline.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/document_range_ops.h"
#include "app/document_undo.h"
#include "app/loop_tag.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/thumbnails.h"
#include "app/transaction.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/configure_timeline_popup.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/input_chain.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "base/bind.h"
#include "base/convert_to.h"
#include "base/memory.h"
#include "base/scoped_value.h"
#include "base/unique_ptr.h"
#include "doc/doc.h"
#include "doc/document_event.h"
#include "doc/frame_tag.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "she/font.h"
#include "she/surface.h"
#include "she/system.h"
#include "ui/scroll_helper.h"
#include "ui/ui.h"

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
  PART_HEADER_FRAME_TAGS,
  PART_LAYER,
  PART_LAYER_EYE_ICON,
  PART_LAYER_PADLOCK_ICON,
  PART_LAYER_CONTINUOUS_ICON,
  PART_LAYER_TEXT,
  PART_CEL,
  PART_RANGE_OUTLINE,
  PART_FRAME_TAG,
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

} // anonymous namespace

Timeline::Timeline()
  : Widget(kGenericWidget)
  , m_hbar(HORIZONTAL, this)
  , m_vbar(VERTICAL, this)
  , m_zoom(1.0)
  , m_context(UIContext::instance())
  , m_editor(NULL)
  , m_document(NULL)
  , m_sprite(NULL)
  , m_state(STATE_STANDBY)
  , m_separator_x(100 * guiscale())
  , m_separator_w(1)
  , m_confPopup(NULL)
  , m_clipboard_timer(100, this)
  , m_offset_count(0)
  , m_scroll(false)
  , m_fromTimeline(false)
{
  enableFlags(CTRL_RIGHT_CLICK);

  m_ctxConn = m_context->AfterCommandExecution.connect(
    &Timeline::onAfterCommandExecution, this);
  m_context->documents().add_observer(this);

  setDoubleBuffered(true);
  addChild(&m_aniControls);
  addChild(&m_hbar);
  addChild(&m_vbar);

  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  int barsize = theme->dimensions.miniScrollbarSize();
  m_hbar.setBarWidth(barsize);
  m_vbar.setBarWidth(barsize);
  m_hbar.setTransparent(true);
  m_vbar.setTransparent(true);
  m_hbar.setStyle(theme->styles.transparentScrollbar());
  m_vbar.setStyle(theme->styles.transparentScrollbar());
  m_hbar.setThumbStyle(theme->styles.transparentScrollbarThumb());
  m_vbar.setThumbStyle(theme->styles.transparentScrollbarThumb());
}

Timeline::~Timeline()
{
  m_clipboard_timer.stop();

  detachDocument();
  m_context->documents().remove_observer(this);
  delete m_confPopup;
}

void Timeline::setZoom(double zoom)
{
  m_zoom = MID(1.0, zoom, 10.0);
  m_thumbnailsOverlayDirection = gfx::Point(int(frameBoxWidth()*1.0), int(frameBoxWidth()*0.5));
  m_thumbnailsOverlayVisible = false;
}

void Timeline::setZoomAndUpdate(double zoom)
{
  if (zoom != m_zoom) {
    setZoom(zoom);
    updateScrollBars();
    setViewScroll(viewScroll());
    invalidate();
  }
  if (zoom != docPref().thumbnails.zoom()) {
    docPref().thumbnails.zoom(zoom);
  }
}

void Timeline::onThumbnailsPrefChange()
{
  setZoomAndUpdate(docPref().thumbnails.zoom());
}

void Timeline::updateUsingEditor(Editor* editor)
{
  m_aniControls.updateUsingEditor(editor);

  detachDocument();

  if (m_range.enabled()) {
    m_range.clearRange();
    invalidate();
  }

  // We always update the editor. In this way the timeline keeps in
  // sync with the active editor.
  m_editor = editor;

  if (m_editor)
    m_editor->add_observer(this);
  else
    return;                // No editor specified.

  Site site;
  DocumentView* view = m_editor->getDocumentView();
  view->getSite(&site);

  site.document()->add_observer(this);

  app::Document* app_document = static_cast<app::Document*>(site.document());
  DocumentPreferences& docPref = Preferences::instance().document(app_document);

  m_thumbnailsPrefConn = docPref.thumbnails.AfterChange.connect(
    base::Bind<void>(&Timeline::onThumbnailsPrefChange, this));

  setZoom(docPref.thumbnails.zoom());

  // If we are already in the same position as the "editor", we don't
  // need to update the at all timeline.
  if (m_document == site.document() &&
      m_sprite == site.sprite() &&
      m_layer == site.layer() &&
      m_frame == site.frame())
    return;

  m_document = static_cast<app::Document*>(site.document());
  m_sprite = site.sprite();
  m_layer = site.layer();
  m_frame = site.frame();
  m_state = STATE_STANDBY;
  m_hot.part = PART_NOTHING;
  m_clk.part = PART_NOTHING;

  m_firstFrameConn = Preferences::instance().document(m_document)
    .timeline.firstFrame.AfterChange.connect(base::Bind<void>(&Timeline::invalidate, this));

  setFocusStop(true);
  regenerateLayers();
  setViewScroll(viewScroll());
  showCurrentCel();
}

void Timeline::detachDocument()
{
  m_firstFrameConn.disconnect();

  if (m_document) {
    m_thumbnailsPrefConn.disconnect();
    m_document->remove_observer(this);
    m_document = NULL;
  }

  if (m_editor) {
    m_editor->remove_observer(this);
    m_editor = NULL;
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

  m_layer = layer;

  // Expand all parents
  if (m_layer) {
    LayerGroup* group = m_layer->parent();
    while (group != m_layer->sprite()->root()) {
      // Expand this group
      group->setCollapsed(false);
      group = group->parent();
    }
    regenerateLayers();
  }

  invalidate();

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

  m_frame = frame;
  invalidate();

  if (m_editor->frame() != frame) {
    bool isPlaying = m_editor->isPlaying();

    if (isPlaying)
      m_editor->stop();

    m_editor->setFrame(m_frame);

    if (isPlaying)
      m_editor->play(false,
                     Preferences::instance().editor.playAll());
  }
}

void Timeline::prepareToMoveRange()
{
  ASSERT(m_range.enabled());

  layer_t i = 0;
  for (auto layer : m_range.selectedLayers().toLayerList()) {
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

void Timeline::moveRange(Range& range)
{
  regenerateLayers();

  // We have to change the range before we generate an
  // onActiveSiteChange() event so observers (like cel properties
  // dialog) know the new selected range.
  m_range = range;

  layer_t i = 0;
  for (auto layer : range.selectedLayers().toLayerList()) {
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
}

void Timeline::activateClipboardRange()
{
  m_clipboard_timer.start();
  invalidate();
}

bool Timeline::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kFocusEnterMessage:
      App::instance()->inputChain().prioritize(this);
      break;

    case kTimerMessage:
      if (static_cast<TimerMessage*>(msg)->timer() == &m_clipboard_timer) {
        Document* clipboard_document;
        DocumentRange clipboard_range;
        clipboard::get_document_range_info(
          &clipboard_document,
          &clipboard_range);

        if (isVisible() && m_document && clipboard_document == m_document) {
          // Set offset to make selection-movement effect
          if (m_offset_count < 7)
            m_offset_count++;
          else
            m_offset_count = 0;
        }
        else if (m_clipboard_timer.isRunning()) {
          m_clipboard_timer.stop();
        }

        invalidate();
      }
      break;

    case kMouseDownMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

      if (!m_document)
        break;

      if (mouseMsg->middle() ||
          she::instance()->isKeyPressed(kKeySpace)) {
        captureMouse();
        m_state = STATE_SCROLLING;
        m_oldPos = static_cast<MouseMessage*>(msg)->position();
        return true;
      }

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
              m_range.clearRange();
            m_range.startRange(m_layer, m_clk.frame, Range::kFrames);
            m_startRange = m_range;

            setFrame(m_clk.frame, true);
          }
          break;
        }
        case PART_LAYER_TEXT: {
          base::ScopedValue<bool> lock(m_fromTimeline, true, false);
          layer_t old_layer = getLayerIndex(m_layer);
          bool selectLayer = (mouseMsg->left() || !isLayerActive(m_clk.layer));

          if (selectLayer) {
            m_state = STATE_SELECTING_LAYERS;
            if (clearRange)
              m_range.clearRange();
            m_range.startRange(m_layers[m_clk.layer].layer,
                               m_frame, Range::kLayers);
            m_startRange = m_range;

            // Did the user select another layer?
            if (old_layer != m_clk.layer) {
              setLayer(m_layers[m_clk.layer].layer);
              invalidate();
            }
          }

          // Change the scroll to show the new selected layer/cel.
          showCel(m_clk.layer, m_frame);
          break;
        }
        case PART_LAYER_EYE_ICON:
          break;
        case PART_LAYER_PADLOCK_ICON:
          break;
        case PART_LAYER_CONTINUOUS_ICON:
          break;
        case PART_CEL: {
          base::ScopedValue<bool> lock(m_fromTimeline, true, false);
          layer_t old_layer = getLayerIndex(m_layer);
          bool selectCel = (mouseMsg->left()
            || !isLayerActive(m_clk.layer)
            || !isFrameActive(m_clk.frame));
          frame_t old_frame = m_frame;

          if (selectCel) {
            m_state = STATE_SELECTING_CELS;
            m_range.clearRange();
            m_range.startRange(m_layers[m_clk.layer].layer,
                               m_clk.frame, Range::kCels);
            m_startRange = m_range;
          }

          // Select the new clicked-part.
          if (old_layer != m_clk.layer
            || old_frame != m_clk.frame) {
            setLayer(m_layers[m_clk.layer].layer);
            setFrame(m_clk.frame, true);
            invalidate();
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
                layerIdx = MID(layerFirst, layerIdx, layerLast);
                m_clk.layer = layerIdx;
              }
            }

            if (m_clk.frame < m_range.firstFrame())
              m_clk.frame = m_range.firstFrame();
            else if (m_clk.frame > m_range.lastFrame())
              m_clk.frame = m_range.lastFrame();
          }
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
            int newValue = m_origFrames + (m_clk.frame - hit.frame);
            docPref().onionskin.prevFrames(MAX(0, newValue));
            invalidate();
            return true;
          }

          case STATE_MOVING_ONIONSKIN_RANGE_RIGHT:
            int newValue = m_origFrames - (m_clk.frame - hit.frame);
            docPref().onionskin.nextFrames(MAX(0, newValue));
            invalidate();
            return true;
        }

        // If the mouse pressed the mouse's button in the separator,
        // we shouldn't change the hot (so the separator can be
        // tracked to the mouse's released).
        if (m_clk.part == PART_SEPARATOR) {
          m_separator_x = MAX(0, mousePos.x);
          layout();
          return true;
        }
      }

      updateDropRange(mousePos);

      if (hasCapture()) {
        switch (m_state) {

          case STATE_SELECTING_LAYERS: {
            Layer* hitLayer = m_layers[hit.layer].layer;
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
            m_range = m_startRange;
            m_range.endRange(m_layer, hit.frame);

            setFrame(m_clk.frame = hit.frame, true);
            break;
          }

          case STATE_SELECTING_CELS:
            Layer* hitLayer = m_layers[hit.layer].layer;
            if ((m_layer != hitLayer) || (m_frame != hit.frame)) {
              m_clk.layer = hit.layer;

              m_range = m_startRange;
              m_range.endRange(hitLayer, hit.frame);

              setLayer(hitLayer);
              setFrame(m_clk.frame = hit.frame, true);
            }
            break;
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

        bool regenLayers = false;
        setHot(hitTest(msg, mouseMsg->position() - bounds().origin()));

        switch (m_hot.part) {

          case PART_NOTHING:
          case PART_SEPARATOR:
          case PART_HEADER_LAYER:
            // Do nothing.
            break;

          case PART_HEADER_EYE: {
            ASSERT(m_sprite);
            if (!m_sprite)
              break;

            bool newVisibleState = !allLayersVisible();
            for (Layer* topLayer : m_sprite->root()->layers()) {
              if (topLayer->isVisible() != newVisibleState) {
                topLayer->setVisible(newVisibleState);
                if (topLayer->isGroup())
                  regenLayers = true;
              }
            }

            // Redraw all views.
            m_document->notifyGeneralUpdate();
            break;
          }

          case PART_HEADER_PADLOCK: {
            ASSERT(m_sprite);
            if (!m_sprite)
              break;

            bool newEditableState = !allLayersUnlocked();
            for (Layer* topLayer : m_sprite->root()->layers()) {
              if (topLayer->isEditable() != newEditableState) {
                topLayer->setEditable(newEditableState);
                if (topLayer->isGroup())
                  regenLayers = true;
              }
            }
            break;
          }

          case PART_HEADER_CONTINUOUS: {
            bool newContinuousState = !allLayersContinuous();
            for (size_t i=0; i<m_layers.size(); i++)
              m_layers[i].layer->setContinuous(newContinuousState);
            break;
          }

          case PART_HEADER_GEAR: {
            gfx::Rect gearBounds =
              getPartBounds(Hit(PART_HEADER_GEAR)).offset(bounds().origin());

            if (!m_confPopup) {
              ConfigureTimelinePopup* popup =
                new ConfigureTimelinePopup();

              popup->remapWindow();
              m_confPopup = popup;
            }

            if (!m_confPopup->isVisible()) {
              gfx::Rect bounds = m_confPopup->bounds();
              ui::fit_bounds(BOTTOM, gearBounds, bounds);

              m_confPopup->moveWindow(bounds);
              m_confPopup->openWindow();
            }
            else
              m_confPopup->closeWindow(NULL);
            break;
          }

          case PART_HEADER_ONIONSKIN: {
            docPref().onionskin.active(!docPref().onionskin.active());
            break;
          }

          case PART_HEADER_FRAME:
            // Show the frame pop-up menu.
            if (mouseMsg->right()) {
              if (m_clk.frame == m_hot.frame) {
                Menu* popupMenu = AppMenus::instance()->getFramePopupMenu();
                if (popupMenu) {
                  popupMenu->showPopup(mouseMsg->position());

                  m_state = STATE_STANDBY;
                  invalidate();
                }
              }
            }
            break;

          case PART_LAYER_TEXT:
            // Show the layer pop-up menu.
            if (mouseMsg->right()) {
              if (m_clk.layer == m_hot.layer) {
                Menu* popupMenu = AppMenus::instance()->getLayerPopupMenu();
                if (popupMenu) {
                  popupMenu->showPopup(mouseMsg->position());

                  m_state = STATE_STANDBY;
                  invalidate();
                }
              }
            }
            break;

          case PART_LAYER_EYE_ICON:
            // Hide/show layer.
            if (m_hot.layer == m_clk.layer && validLayer(m_hot.layer)) {
              LayerInfo& info = m_layers[m_clk.layer];
              Layer* layer = info.layer;
              ASSERT(layer);

              // Show parents
              if (!info.parentVisible()) {
                regenLayers = true;

                layer->setVisible(true);
                layer = layer->parent();
                while (layer) {
                  if (!layer->isVisible())
                    layer->setVisible(true);
                  layer = layer->parent();
                }
              }
              else {
                layer->setVisible(!layer->isVisible());
                if (layer->isGroup() && layer->isExpanded())
                  regenLayers = true;
              }

              // Redraw all views.
              m_document->notifyGeneralUpdate();
            }
            break;

          case PART_LAYER_PADLOCK_ICON:
            // Lock/unlock layer.
            if (m_hot.layer == m_clk.layer && validLayer(m_hot.layer)) {
              LayerInfo& info = m_layers[m_clk.layer];
              Layer* layer = info.layer;
              ASSERT(layer);

              // Unlock parents
              if (!info.parentEditable()) {
                regenLayers = true;

                layer->setEditable(true);
                layer = layer->parent();
                while (layer) {
                  if (!layer->isEditable())
                    layer->setEditable(true);
                  layer = layer->parent();
                }
              }
              else {
                layer->setEditable(!layer->isEditable());
                if (layer->isGroup() && layer->isExpanded())
                  regenLayers = true;
              }
            }
            break;

          case PART_LAYER_CONTINUOUS_ICON:
            if (m_hot.layer == m_clk.layer && validLayer(m_hot.layer)) {
              Layer* layer = m_layers[m_clk.layer].layer;
              ASSERT(layer);
              if (layer) {
                if (layer->isImage())
                  layer->setContinuous(!layer->isContinuous());
                else if (layer->isGroup()) {
                  layer->setCollapsed(!layer->isCollapsed());

                  regenerateLayers();
                  invalidate();

                  updateByMousePos(
                    msg, ui::get_mouse_position() - bounds().origin());
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
                popupMenu->showPopup(mouseMsg->position());

                // Do not drop in this function, the drop is done from
                // the menu in case we've used the
                // CelMovementPopupMenu
                m_state = STATE_STANDBY;
                invalidate();
              }
            }
            break;
          }

          case PART_FRAME_TAG: {
            FrameTag* frameTag = m_clk.getFrameTag();
            if (frameTag) {
              Params params;
              params.set("id", base::convert_to<std::string>(frameTag->id()).c_str());

              // As the m_clk.frameTag can be deleted with
              // RemoveFrameTag command, we've to clean all references
              // to it from Hit() structures.
              cleanClk();
              m_hot = m_clk;

              if (mouseMsg->right()) {
                Menu* popupMenu = AppMenus::instance()->getFrameTagPopupMenu();
                if (popupMenu) {
                  AppMenuItem::setContextParams(params);
                  popupMenu->showPopup(mouseMsg->position());
                  AppMenuItem::setContextParams(Params());

                  m_state = STATE_STANDBY;
                  invalidate();
                }
              }
              else if (mouseMsg->left()) {
                Command* command = CommandsModule::instance()
                  ->getCommandByName(CommandId::FrameTagProperties);
                UIContext::instance()->executeCommand(command, params);
              }
            }
            break;
          }

        }

        if (regenLayers) {
          regenerateLayers();
          invalidate();
        }

        if (m_state == STATE_MOVING_RANGE &&
            m_dropRange.type() != Range::kNone) {
          dropRange(isCopyKeyPressed(mouseMsg) ?
            Timeline::kCopy:
            Timeline::kMove);
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

        case PART_LAYER_TEXT: {
          Command* command = CommandsModule::instance()
            ->getCommandByName(CommandId::LayerProperties);

          UIContext::instance()->executeCommand(command);
          return true;
        }

        case PART_HEADER_FRAME: {
          Command* command = CommandsModule::instance()
            ->getCommandByName(CommandId::FrameProperties);
          Params params;
          params.set("frame", "current");

          UIContext::instance()->executeCommand(command, params);
          return true;
        }

        case PART_CEL: {
          Command* command = CommandsModule::instance()
            ->getCommandByName(CommandId::CelProperties);

          UIContext::instance()->executeCommand(command);
          return true;
        }

      }
      break;

    case kKeyDownMessage: {
      bool used = false;

      switch (static_cast<KeyMessage*>(msg)->scancode()) {

        case kKeyEsc:
          if (m_state == STATE_STANDBY) {
            m_range.clearRange();
            invalidate();
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

      updateByMousePos(msg,
        ui::get_mouse_position() - bounds().origin());

      if (used)
        return true;

      break;
    }

    case kKeyUpMessage: {
      bool used = false;

      switch (static_cast<KeyMessage*>(msg)->scancode()) {

        case kKeySpace: {
          m_scroll = false;

          // We have to clear all the kKeySpace keys in buffer.
          she::instance()->clearKeyboardBuffer();
          used = true;
          break;
        }
      }

      updateByMousePos(msg,
        ui::get_mouse_position() - bounds().origin());

      if (used)
        return true;

      break;
    }

    case kMouseWheelMessage:
      if (m_document) {
        gfx::Point delta = static_cast<MouseMessage*>(msg)->wheelDelta();
        if (!static_cast<MouseMessage*>(msg)->preciseWheel()) {
          delta.x *= frameBoxWidth();
          delta.y *= layerBoxHeight();

          if (msg->shiftPressed()) {
            // On macOS shift already changes the wheel axis
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
      break;

    case kSetCursorMessage:
      if (m_document) {
        setCursor(msg, m_hot);
        return true;
      }
      break;

    case kTouchMagnifyMessage:
      setZoomAndUpdate(m_zoom + m_zoom * static_cast<ui::TouchMessage*>(msg)->magnification());
      break;
  }

  return Widget::onProcessMessage(msg);
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
      rc.y+MAX(0, m_tagBands-1)*oneTagHeight(),
      MIN(sz.w, m_separator_x),
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
    // Lock the sprite to read/render it. We wait 1/4 secs in case
    // the background thread is making a backup.
    const DocumentReader documentReader(m_document, 250);

    layer_t layer, firstLayer, lastLayer;
    frame_t frame, firstFrame, lastFrame;

    getDrawableLayers(g, &firstLayer, &lastLayer);
    getDrawableFrames(g, &firstFrame, &lastFrame);

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

      Layer* layerPtr = m_layers[layer].layer;
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
    drawFrameTags(g);
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
  catch (const LockedDocumentException&) {
    noDoc = true;
    defer_invalid_rect(g->getClipBounds().offset(bounds().origin()));
  }

paintNoDoc:;
  if (noDoc)
    drawPart(
      g, clientBounds(), nullptr,
      skinTheme()->styles.timelinePadding());
}

void Timeline::onAfterCommandExecution(CommandExecutionEvent& ev)
{
  if (!m_document)
    return;

  regenerateLayers();
  showCurrentCel();
  invalidate();
}

void Timeline::onRemoveDocument(doc::Document* document)
{
  if (document == m_document) {
    detachDocument();
  }
}

void Timeline::onGeneralUpdate(DocumentEvent& ev)
{
  invalidate();
}

void Timeline::onAddLayer(doc::DocumentEvent& ev)
{
  ASSERT(ev.layer() != NULL);

  setLayer(ev.layer());

  regenerateLayers();
  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

void Timeline::onAfterRemoveLayer(doc::DocumentEvent& ev)
{
  Sprite* sprite = ev.sprite();
  Layer* layer = ev.layer();

  // If the layer that was removed is the selected one
  if (layer == getLayer()) {
    LayerGroup* parent = layer->parent();
    Layer* layer_select = NULL;

    // Select previous layer, or next layer, or the parent (if it is
    // not the main layer of sprite set).
    if (layer->getPrevious())
      layer_select = layer->getPrevious();
    else if (layer->getNext())
      layer_select = layer->getNext();
    else if (parent != sprite->root())
      layer_select = parent;

    setLayer(layer_select);
  }

  regenerateLayers();
  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

void Timeline::onAddFrame(doc::DocumentEvent& ev)
{
  setFrame(ev.frame(), false);

  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

void Timeline::onRemoveFrame(doc::DocumentEvent& ev)
{
  // Adjust current frame of all editors that are in a frame more
  // advanced that the removed one.
  if (getFrame() > ev.frame()) {
    setFrame(getFrame()-1, false);
  }
  // If the editor was in the previous "last frame" (current value of
  // totalFrames()), we've to adjust it to the new last frame
  // (lastFrame())
  else if (getFrame() >= sprite()->totalFrames()) {
    setFrame(sprite()->lastFrame(), false);
  }

  // Disable the selected range when we remove frames
  if (m_range.enabled())
    m_range.clearRange();

  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

void Timeline::onSelectionChanged(doc::DocumentEvent& ev)
{
  m_range.clearRange();
  invalidate();
}

void Timeline::onLayerNameChange(doc::DocumentEvent& ev)
{
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

  if (!hasCapture())
    m_range.clearRange();

  showCurrentCel();
  invalidate();
}

void Timeline::onAfterLayerChanged(Editor* editor)
{
  if (m_fromTimeline)
    return;

  if (!hasCapture())
    m_range.clearRange();

  setLayer(editor->layer());

  showCurrentCel();
  invalidate();
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
    if (isCopyKeyPressed(msg))
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
  else if (hit.part == PART_FRAME_TAG) {
    ui::set_mouse_cursor(kHandCursor);
  }
  else {
    ui::set_mouse_cursor(kArrowCursor);
  }
}

void Timeline::getDrawableLayers(ui::Graphics* g, layer_t* firstLayer, layer_t* lastLayer)
{
  int hpx = (clientBounds().h - headerBoxHeight() - topHeight());
  layer_t i = this->lastLayer() - ((viewScroll().y+hpx) / layerBoxHeight());
  i = MID(this->firstLayer(), i, this->lastLayer());

  layer_t j = i + (hpx / layerBoxHeight() + 1);
  if (!m_layers.empty())
    j = MID(this->firstLayer(), j, this->lastLayer());
  else
    j = -1;

  *firstLayer = i;
  *lastLayer = j;
}

void Timeline::getDrawableFrames(ui::Graphics* g, frame_t* firstFrame, frame_t* lastFrame)
{
  int availW = (clientBounds().w - m_separator_x);

  *firstFrame = frame_t(viewScroll().x / frameBoxWidth());
  *lastFrame = *firstFrame
    + frame_t(availW / frameBoxWidth())
    + ((availW % frameBoxWidth()) > 0 ? 1: 0);
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
  Document* clipboard_document;
  DocumentRange clipboard_range;
  clipboard::get_document_range_info(
    &clipboard_document,
    &clipboard_range);

  if (!m_document || clipboard_document != m_document)
    return;

  if (!m_clipboard_timer.isRunning())
    m_clipboard_timer.start();

  CheckedDrawMode checked(g, m_offset_count,
                          gfx::rgba(0, 0, 0, 255),
                          gfx::rgba(255, 255, 255, 255));
  g->drawRect(gfx::rgba(0, 0, 0),
    getRangeBounds(clipboard_range));
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
    false,
    m_hot.part == PART_HEADER_GEAR,
    m_clk.part == PART_HEADER_GEAR);

  drawPart(g, getPartBounds(Hit(PART_HEADER_ONIONSKIN)),
    NULL, styles.timelineOnionskin(),
    docPref().onionskin.active(),
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
  std::string text = base::convert_to<std::string, int>(
    (docPref().timeline.firstFrame()+frame) % 100);

  drawPart(g, bounds, &text,
           skinTheme()->styles.timelineHeaderFrame(),
           is_active, is_hover, is_clicked);
}

void Timeline::drawLayer(ui::Graphics* g, int layerIdx)
{
  auto& styles = skinTheme()->styles;
  Layer* layer = m_layers[layerIdx].layer;
  bool is_active = isLayerActive(layerIdx);
  bool hotlayer = (m_hot.layer == layerIdx);
  bool clklayer = (m_clk.layer == layerIdx);
  gfx::Rect bounds = getPartBounds(Hit(PART_LAYER, layerIdx, firstFrame()));
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  // Draw the eye (visible flag).
  bounds = getPartBounds(Hit(PART_LAYER_EYE_ICON, layerIdx));
  drawPart(
    g, bounds, nullptr,
    (layer->isVisible() ? styles.timelineOpenEye():
                          styles.timelineClosedEye()),
    is_active,
    (hotlayer && m_hot.part == PART_LAYER_EYE_ICON),
    (clklayer && m_clk.part == PART_LAYER_EYE_ICON),
    !m_layers[layerIdx].parentVisible());

  // Draw the padlock (editable flag).
  bounds = getPartBounds(Hit(PART_LAYER_PADLOCK_ICON, layerIdx));
  drawPart(
    g, bounds, nullptr,
    (layer->isEditable() ? styles.timelineOpenPadlock():
                           styles.timelineClosedPadlock()),
    is_active,
    (hotlayer && m_hot.part == PART_LAYER_PADLOCK_ICON),
    (clklayer && m_clk.part == PART_LAYER_PADLOCK_ICON),
    !m_layers[layerIdx].parentEditable());

  // Draw the continuous flag/group icon.
  bounds = getPartBounds(Hit(PART_LAYER_CONTINUOUS_ICON, layerIdx));
  if (layer->isImage()) {
    drawPart(g, bounds, nullptr,
             layer->isContinuous() ? styles.timelineContinuous():
                                     styles.timelineDiscontinuous(),
             is_active,
             (hotlayer && m_hot.part == PART_LAYER_CONTINUOUS_ICON),
             (clklayer && m_clk.part == PART_LAYER_CONTINUOUS_ICON));
  }
  else if (layer->isGroup()) {
    drawPart(g, bounds, nullptr,
             layer->isCollapsed() ? styles.timelineClosedGroup():
                                    styles.timelineOpenGroup(),
             is_active,
             (hotlayer && m_hot.part == PART_LAYER_CONTINUOUS_ICON),
             (clklayer && m_clk.part == PART_LAYER_CONTINUOUS_ICON));
  }

  // Get the layer's name bounds.
  bounds = getPartBounds(Hit(PART_LAYER_TEXT, layerIdx));

  // Draw layer name.
  doc::color_t layerColor = layer->userData().color();
  gfx::Rect textBounds = bounds;
  if (m_layers[layerIdx].level > 0) {
    int w = m_layers[layerIdx].level*frameBoxWidth();
    textBounds.x += w;
    textBounds.w -= w;
  }

  drawPart(g, bounds, nullptr, styles.timelineLayer(),
           is_active,
           (hotlayer && m_hot.part == PART_LAYER_TEXT),
           (clklayer && m_clk.part == PART_LAYER_TEXT));

  if (doc::rgba_geta(layerColor) > 0) {
    // Fill with an user-defined custom color.
    auto b2 = bounds;
    b2.shrink(1*guiscale()).inflate(1*guiscale());
    g->fillRect(gfx::rgba(doc::rgba_getr(layerColor),
                          doc::rgba_getg(layerColor),
                          doc::rgba_getb(layerColor),
                          doc::rgba_geta(layerColor)),
                b2);

    drawPart(g, textBounds,
             &layer->name(),
             styles.timelineLayerTextOnly(),
             is_active,
             (hotlayer && m_hot.part == PART_LAYER_TEXT),
             (clklayer && m_clk.part == PART_LAYER_TEXT));
  }
  else {
    drawPart(g, textBounds,
             &layer->name(),
             styles.timelineLayer(),
             is_active,
             (hotlayer && m_hot.part == PART_LAYER_TEXT),
             (clklayer && m_clk.part == PART_LAYER_TEXT));
  }

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
  if (hotlayer && !is_active && m_clk.part == PART_LAYER_TEXT) {
    // TODO this should be skinneable
    g->fillRect(
      skinTheme()->colors.timelineActive(),
      gfx::Rect(bounds.x, bounds.y, bounds.w, 2));
  }
}

void Timeline::drawCel(ui::Graphics* g, layer_t layerIndex, frame_t frame, Cel* cel, DrawCelData* data)
{
  auto& styles = skinTheme()->styles;
  Layer* layer = m_layers[layerIndex].layer;
  Image* image = (cel ? cel->image(): nullptr);
  bool is_hover = (m_hot.part == PART_CEL &&
    m_hot.layer == layerIndex &&
    m_hot.frame == frame);
  const bool is_active = (isLayerActive(layerIndex) ||
                          isFrameActive(frame));
  const bool is_empty = (image == nullptr);
  gfx::Rect bounds = getPartBounds(Hit(PART_CEL, layerIndex, frame));
  gfx::Rect full_bounds = bounds;
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  if (layer == m_layer && frame == m_frame)
    drawPart(g, bounds, nullptr, styles.timelineSelectedCel(), false, false, true);
  else
    drawPart(g, bounds, nullptr, styles.timelineBox(), is_active, is_hover);

  if ((docPref().thumbnails.enabled() || m_zoom > 1) && image) {
    gfx::Rect thumb_bounds = gfx::Rect(bounds).shrink(guiscale()).inflate(guiscale(), guiscale());
    if(m_zoom > 1)
      thumb_bounds.inflate(0, -headerBoxHeight()).offset(0, headerBoxHeight());

    if(!thumb_bounds.isEmpty()) {
      she::Surface* thumb_surf = thumb::get_cel_thumbnail(cel, thumb_bounds.size());

      g->drawRgbaSurface(thumb_surf, thumb_bounds.x, thumb_bounds.y);

      thumb_surf->dispose();
    }
  }

  if (!docPref().thumbnails.enabled() || m_zoom > 1 || !image) {
    bounds.h = headerBoxHeight();

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

    bounds.w = headerBoxWidth();

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
    drawPart(g, bounds, nullptr, style, is_active, is_hover);

    if (m_zoom > 1) {
      if (style == styles.timelineFromBoth() ||
          style == styles.timelineFromRight()) {
        style = styles.timelineFromBoth();
        while ((bounds.x += bounds.w) < full_bounds.x + full_bounds.w) {
          drawPart(g, bounds, nullptr, style, is_active, is_hover);
        }
      }
    }

    // Draw decorators to link the activeCel with its links.
    if (data && data->activeIt != data->end)
      drawCelLinkDecorators(g, full_bounds, cel, frame, is_active, is_hover, data);
  }
}

void Timeline::updateCelOverlayBounds(const Hit& hit)
{
  gfx::Rect inner, outer;

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
    inner = gfx::Rect(
      bounds_cel.x + m_thumbnailsOverlayDirection.x,
      bounds_cel.y + m_thumbnailsOverlayDirection.y,
      width,
      height
    );

    if (!client_bounds.contains(inner)) {
      m_thumbnailsOverlayDirection = gfx::Point(
        bounds_cel.x < center.x ? (int)(frameBoxWidth()*1.0) : -width,
        bounds_cel.y < center.y ? (int)(frameBoxWidth()*0.5) : -height+(int)(frameBoxWidth()*0.5)
      );
      inner.setOrigin(gfx::Point(
        bounds_cel.x + m_thumbnailsOverlayDirection.x,
        bounds_cel.y + m_thumbnailsOverlayDirection.y
      ));
    }

    outer = gfx::Rect(inner).enlarge(1);
  }
  else {
    outer = gfx::Rect(0, 0, 0, 0);
  }

  if (outer != m_thumbnailsOverlayOuter) {
    if (!m_thumbnailsOverlayOuter.isEmpty()) {
      invalidateRect(gfx::Rect(m_thumbnailsOverlayOuter).offset(origin()));
    }
    if (!outer.isEmpty()) {
      invalidateRect(gfx::Rect(outer).offset(origin()));
    }
    m_thumbnailsOverlayVisible = !outer.isEmpty();
    m_thumbnailsOverlayOuter = outer;
    m_thumbnailsOverlayInner = inner;
  }
}

void Timeline::drawCelOverlay(ui::Graphics* g)
{
  if (!m_thumbnailsOverlayVisible) {
    return;
  }

  Layer* layer = m_layers[m_thumbnailsOverlayHit.layer].layer;
  Cel* cel = layer->cel(m_thumbnailsOverlayHit.frame);
  if (!cel) {
    return;
  }
  Image* image = cel->image();
  if (!image) {
    return;
  }

  IntersectClip clip(g, m_thumbnailsOverlayOuter);
  if (!clip)
    return;

  double scale = (
    m_sprite->width() > m_sprite->height() ?
    m_thumbnailsOverlayInner.w / (double)m_sprite->width() :
    m_thumbnailsOverlayInner.h / (double)m_sprite->height()
  );

  gfx::Size overlay_size(
    m_thumbnailsOverlayInner.w,
    m_thumbnailsOverlayInner.h
  );

  gfx::Rect cel_image_on_overlay(
    (int)(cel->x() * scale),
    (int)(cel->y() * scale),
    (int)(image->width() * scale),
    (int)(image->height() * scale)
  );

  she::Surface* overlay_surf = thumb::get_cel_thumbnail(cel, overlay_size, cel_image_on_overlay);

  g->drawRgbaSurface(overlay_surf,
    m_thumbnailsOverlayInner.x, m_thumbnailsOverlayInner.y);
  g->drawRect(gfx::rgba(0,0,0,255), m_thumbnailsOverlayOuter);

  overlay_surf->dispose();
}

void Timeline::drawCelLinkDecorators(ui::Graphics* g, const gfx::Rect& full_bounds,
                                     Cel* cel, frame_t frame, bool is_active, bool is_hover,
                                     DrawCelData* data)
{
  auto& styles = skinTheme()->styles;
  ObjectId imageId = (*data->activeIt)->image()->id();

  gfx::Rect bounds = gfx::Rect(full_bounds).setSize(gfx::Size(headerBoxWidth(), headerBoxHeight()));
  ui::Style* style = NULL;

  // Links at the left or right side
  bool left = (data->firstLink != data->end ? frame > (*data->firstLink)->frame(): false);
  bool right = (data->lastLink != data->end ? frame < (*data->lastLink)->frame(): false);

  if (cel && cel->image()->id() == imageId) {
    if (left) {
      Cel* prevCel = m_layer->cel(cel->frame()-1);
      if (!prevCel || prevCel->image()->id() != imageId)
        style = styles.timelineLeftLink();
    }
    if (right) {
      Cel* nextCel = m_layer->cel(cel->frame()+1);
      if (!nextCel || nextCel->image()->id() != imageId)
        style = styles.timelineRightLink();
    }
  }
  else {
    if (left && right)
      style = styles.timelineBothLinks();
  }

  if (style) {
    drawPart(g, bounds, nullptr, style, is_active, is_hover);

    if (m_zoom > 1 && (style == styles.timelineBothLinks() || style == styles.timelineRightLink())) {
      style = styles.timelineBothLinks();
      while ((bounds.x += bounds.w) < full_bounds.x + full_bounds.w) {
        drawPart(g, bounds, nullptr, style, is_active, is_hover);
      }
    }
  }
}

void Timeline::drawFrameTags(ui::Graphics* g)
{
  IntersectClip clip(g, getPartBounds(Hit(PART_HEADER_FRAME_TAGS)));
  if (!clip)
    return;

  SkinTheme* theme = skinTheme();
  auto& styles = theme->styles;

  g->fillRect(theme->colors.workspace(),
    gfx::Rect(
      0, font()->height(),
      clientBounds().w,
      theme->dimensions.timelineTagsAreaHeight()));

  std::vector<unsigned char> tagsPerFrame(m_sprite->totalFrames(), 0);

  for (FrameTag* frameTag : m_sprite->frameTags()) {
    gfx::Rect bounds1 = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), frameTag->fromFrame()));
    gfx::Rect bounds2 = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), frameTag->toFrame()));
    gfx::Rect bounds = bounds1.createUnion(bounds2);
    gfx::Rect frameTagBounds = getPartBounds(Hit(PART_FRAME_TAG, 0, 0, frameTag->id()));
    bounds.h = bounds.y2() - frameTagBounds.y2();
    bounds.y = frameTagBounds.y2();

    {
      IntersectClip clip(g, bounds);
      if (clip)
        drawPart(g, bounds, nullptr, styles.timelineLoopRange());
    }

    {
      bounds = frameTagBounds;

      gfx::Color bg = frameTag->color();
      if (m_clk.part == PART_FRAME_TAG && m_clk.frameTag == frameTag->id()) {
        bg = color_utils::blackandwhite_neg(bg);
      }
      else if (m_hot.part == PART_FRAME_TAG && m_hot.frameTag == frameTag->id()) {
        int r, g, b;
        r = gfx::getr(bg)+32;
        g = gfx::getg(bg)+32;
        b = gfx::getb(bg)+32;
        r = MID(0, r, 255);
        g = MID(0, g, 255);
        b = MID(0, b, 255);
        bg = gfx::rgba(r, g, b, gfx::geta(bg));
      }
      g->fillRect(bg, bounds);

      bounds.y += 2*ui::guiscale();
      bounds.x += 2*ui::guiscale();
      g->drawText(
        frameTag->name(),
        color_utils::blackandwhite_neg(bg),
        gfx::ColorNone,
        bounds.origin());
    }

    for (frame_t f=frameTag->fromFrame(); f<=frameTag->toFrame(); ++f) {
      if (tagsPerFrame[f] < 255)
        ++tagsPerFrame[f];
    }
  }
}

void Timeline::drawRangeOutline(ui::Graphics* g)
{
  auto& styles = skinTheme()->styles;

  gfx::Rect clipBounds;
  switch (m_range.type()) {
    case Range::kCels: clipBounds = getCelsBounds(); break;
    case Range::kFrames: clipBounds = getFrameHeadersBounds(); break;
    case Range::kLayers: clipBounds = getLayerHeadersBounds(); break;
  }
  IntersectClip clip(g, clipBounds.enlarge(outlineWidth()));
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

  if (!m_layers.empty()) {
    bottomLayer = getPartBounds(Hit(PART_LAYER, firstLayer()));
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
  rc.w = m_separator_x;
  int h = topHeight() + headerBoxHeight();
  rc.y += h;
  rc.h -= h;
  return rc;
}

gfx::Rect Timeline::getFrameHeadersBounds() const
{
  gfx::Rect rc = clientBounds();
  rc.x += m_separator_x;
  rc.y += topHeight();
  rc.w -= m_separator_x;
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
  rc.x += m_separator_x;
  rc.w -= m_separator_x;
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
      return gfx::Rect(bounds.x + m_separator_x, bounds.y + y,
        m_separator_x + m_separator_w, bounds.h - y);

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
                       m_separator_x - headerBoxWidth()*5, headerBoxHeight());

    case PART_HEADER_FRAME:
      return gfx::Rect(
        bounds.x + m_separator_x + m_separator_w - 1
        + frameBoxWidth()*MAX(firstFrame(), hit.frame) - viewScroll().x,
        bounds.y + y, frameBoxWidth(), headerBoxHeight());

    case PART_HEADER_FRAME_TAGS:
      return gfx::Rect(
          bounds.x + m_separator_x + m_separator_w - 1,
          bounds.y,
          bounds.w - m_separator_x - m_separator_w + 1, y);

    case PART_LAYER:
      if (validLayer(hit.layer)) {
        return gfx::Rect(bounds.x,
          bounds.y + y + headerBoxHeight() + layerBoxHeight()*(lastLayer()-hit.layer) - viewScroll().y,
          m_separator_x, layerBoxHeight());
      }
      break;

    case PART_LAYER_EYE_ICON:
      if (validLayer(hit.layer)) {
        return gfx::Rect(bounds.x,
          bounds.y + y + headerBoxHeight() + layerBoxHeight()*(lastLayer()-hit.layer) - viewScroll().y,
          headerBoxWidth(), layerBoxHeight());
      }
      break;

    case PART_LAYER_PADLOCK_ICON:
      if (validLayer(hit.layer)) {
        return gfx::Rect(bounds.x + headerBoxWidth(),
          bounds.y + y + headerBoxHeight() + layerBoxHeight()*(lastLayer()-hit.layer) - viewScroll().y,
          headerBoxWidth(), layerBoxHeight());
      }
      break;

    case PART_LAYER_CONTINUOUS_ICON:
      if (validLayer(hit.layer)) {
        return gfx::Rect(bounds.x + 2* headerBoxWidth(),
          bounds.y + y + headerBoxHeight() + layerBoxHeight()*(lastLayer()-hit.layer) - viewScroll().y,
          headerBoxWidth(), layerBoxHeight());
      }
      break;

    case PART_LAYER_TEXT:
      if (validLayer(hit.layer)) {
        int x = headerBoxWidth()*3;
        return gfx::Rect(bounds.x + x,
          bounds.y + y + headerBoxHeight() + layerBoxHeight()*(lastLayer()-hit.layer) - viewScroll().y,
          m_separator_x - x, layerBoxHeight());
      }
      break;

    case PART_CEL:
      if (validLayer(hit.layer) && hit.frame >= frame_t(0)) {
        return gfx::Rect(
          bounds.x + m_separator_x + m_separator_w - 1 + frameBoxWidth()*hit.frame - viewScroll().x,
          bounds.y + y + headerBoxHeight() + layerBoxHeight()*(lastLayer()-hit.layer) - viewScroll().y,
          frameBoxWidth(), layerBoxHeight());
      }
      break;

    case PART_RANGE_OUTLINE: {
      gfx::Rect rc = getRangeBounds(m_range);
      int s = outlineWidth();
      rc.enlarge(s);
      if (rc.x < bounds.x) rc.offset(s, 0).inflate(-s, 0);
      if (rc.y < bounds.y) rc.offset(0, s).inflate(0, -s);
      return rc;
    }

    case PART_FRAME_TAG: {
      FrameTag* frameTag = hit.getFrameTag();
      if (frameTag) {
        gfx::Rect bounds1 = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), frameTag->fromFrame()));
        gfx::Rect bounds2 = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), frameTag->toFrame()));
        gfx::Rect bounds = bounds1.createUnion(bounds2);
        bounds.y -= skinTheme()->dimensions.timelineTagsAreaHeight();

        int textHeight = font()->height();
        bounds.y -= textHeight + 2*ui::guiscale();
        bounds.x += 3*ui::guiscale();
        bounds.w = font()->textLength(frameTag->name().c_str()) + 4*ui::guiscale();
        bounds.h = font()->height() + 2*ui::guiscale();

        auto it = m_tagBand.find(frameTag);
        if (it != m_tagBand.end()) {
          int dy = (m_tagBands-it->second-1)*oneTagHeight();
          bounds.y -= dy;
        }

        return bounds;
      }
      break;
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
      for (auto frame : range.selectedFrames())
        rc |= getPartBounds(Hit(PART_HEADER_FRAME, 0, frame));
      break;
    }
    case Range::kLayers:
      for (auto layer : range.selectedLayers()) {
        layer_t layerIdx = getLayerIndex(layer);
        rc |= getPartBounds(Hit(PART_LAYER, layerIdx));
      }
      break;
  }
  return rc;
}

void Timeline::invalidateHit(const Hit& hit)
{
  invalidateRect(getPartBounds(hit).offset(origin()));
}

void Timeline::regenerateLayers()
{
  ASSERT(m_document);
  ASSERT(m_sprite);

  size_t nlayers = 0;
  for_each_expanded_layer(
    m_sprite->root(),
    [&nlayers](Layer* layer, int level, LayerFlags flags) {
      ++nlayers;
    });

  if (m_layers.size() != nlayers) {
    if (nlayers > 0)
      m_layers.resize(nlayers);
    else
      m_layers.clear();
  }

  size_t i = 0;
  for_each_expanded_layer(
    m_sprite->root(),
    [&i, this](Layer* layer, int level, LayerFlags flags) {
      m_layers[i++] = LayerInfo(layer, level, flags);
    });

  regenerateTagBands();
  updateScrollBars();
}

void Timeline::regenerateTagBands()
{
  // TODO improve this implementation
  std::vector<unsigned char> tagsPerFrame(m_sprite->totalFrames(), 0);
  std::vector<FrameTag*> bands(4, nullptr);
  m_tagBand.clear();
  for (FrameTag* frameTag : m_sprite->frameTags()) {
    frame_t f = frameTag->fromFrame();

    int b=0;
    for (; b<int(bands.size()); ++b) {
      if (!bands[b] ||
          frameTag->fromFrame() > calcTagVisibleToFrame(bands[b])) {
        bands[b] = frameTag;
        m_tagBand[frameTag] = b;
        break;
      }
    }
    if (b == int(bands.size()))
      m_tagBand[frameTag] = tagsPerFrame[f];

    frame_t toFrame = calcTagVisibleToFrame(frameTag);
    for (; f<=toFrame; ++f) {
      if (tagsPerFrame[f] < 255)
        ++tagsPerFrame[f];
    }
  }
  int oldBands = m_tagBands;
  m_tagBands = 0;
  for (int i : tagsPerFrame)
    m_tagBands = MAX(m_tagBands, i);

  if (oldBands != m_tagBands)
    layout();
}

void Timeline::updateScrollBars()
{
  gfx::Rect rc = bounds();
  m_viewportArea = getCelsBounds().offset(rc.origin());
  ui::setup_scrollbars(getScrollableSize(),
                       m_viewportArea, *this,
                       m_hbar,
                       m_vbar);
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
        - m_separator_x
        - m_separator_w
        + scroll.x) / frameBoxWidth());

    // Flag which indicates that we are in the are below the Background layer/last layer area
    if (hit.layer < 0)
      hit.veryBottom = true;

    if (hasCapture()) {
      hit.layer = MID(firstLayer(), hit.layer, lastLayer());
      if (isMovingCel())
        hit.frame = MAX(firstFrame(), hit.frame);
      else
        hit.frame = MID(firstFrame(), hit.frame, lastFrame());
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
    // Is the mouse on the separator.
    else if (mousePos.x > m_separator_x-4
          && mousePos.x <= m_separator_x)  {
      hit.part = PART_SEPARATOR;
    }
    // Is the mouse on the frame tags area?
    else if (getPartBounds(Hit(PART_HEADER_FRAME_TAGS)).contains(mousePos)) {
      for (FrameTag* frameTag : m_sprite->frameTags()) {
        gfx::Rect bounds = getPartBounds(Hit(PART_FRAME_TAG, 0, 0, frameTag->id()));
        if (bounds.contains(mousePos)) {
          hit.part = PART_FRAME_TAG;
          hit.frameTag = frameTag->id();
          break;
        }
      }
    }
    // Is the mouse on the headers?
    else if (mousePos.y >= top && mousePos.y < top+headerBoxHeight()) {
      if (mousePos.x < m_separator_x) {
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
    else {
      // Is the mouse on a layer's label?
      if (mousePos.x < m_separator_x) {
        if (getPartBounds(Hit(PART_LAYER_EYE_ICON, hit.layer)).contains(mousePos))
          hit.part = PART_LAYER_EYE_ICON;
        else if (getPartBounds(Hit(PART_LAYER_PADLOCK_ICON, hit.layer)).contains(mousePos))
          hit.part = PART_LAYER_PADLOCK_ICON;
        else if (getPartBounds(Hit(PART_LAYER_CONTINUOUS_ICON, hit.layer)).contains(mousePos))
          hit.part = PART_LAYER_CONTINUOUS_ICON;
        else if (getPartBounds(Hit(PART_LAYER_TEXT, hit.layer)).contains(mousePos))
          hit.part = PART_LAYER_TEXT;
        else
          hit.part = PART_LAYER;
      }
      else if (validLayer(hit.layer) && validFrame(hit.frame)) {
        hit.part = PART_CEL;
      }
      else
        hit.part = PART_NOTHING;
    }

    if (!hasCapture()) {
      gfx::Rect outline = getPartBounds(Hit(PART_RANGE_OUTLINE));
      if (outline.contains(mousePos)) {
        auto mouseMsg = dynamic_cast<MouseMessage*>(msg);

        if (// With Ctrl and Alt key we can drag the range from any place (not necessary from the outline.
            isCopyKeyPressed(msg) ||
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
                       - m_separator_x
                       - m_separator_w
                       + scroll.x) / frameBoxWidth());

  hit.layer = MID(firstLayer(), hit.layer, lastLayer());
  hit.frame = MAX(firstFrame(), hit.frame);

  return hit;
}

void Timeline::setHot(const Hit& hit)
{
  // If the part, layer or frame change.
  if (m_hot != hit) {
    // Invalidate the whole control.
    if (m_state == STATE_MOVING_RANGE ||
        hit.part == PART_RANGE_OUTLINE ||
        m_hot.part == PART_RANGE_OUTLINE) {
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

  if (m_state == STATE_MOVING_RANGE) {
    const char* verb = isCopyKeyPressed(msg) ? "Copy": "Move";

    switch (m_range.type()) {

      case Range::kCels:
        sb->setStatusText(0, "%s cels", verb);
        break;

      case Range::kFrames:
        if (validFrame(m_hot.frame)) {
          if (m_dropTarget.hhit == DropTarget::Before) {
            sb->setStatusText(0, "%s before frame %d", verb, int(m_dropRange.firstFrame()+1));
            return;
          }
          else if (m_dropTarget.hhit == DropTarget::After) {
            sb->setStatusText(0, "%s after frame %d", verb, int(m_dropRange.lastFrame()+1));
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
          sb->setStatusText(0, "%s at the very bottom", verb);
          return;
        }

        layer_t layerIdx = -1;
        if (m_dropTarget.vhit == DropTarget::Bottom ||
            m_dropTarget.vhit == DropTarget::FirstChild)
          layerIdx = firstLayer;
        else if (m_dropTarget.vhit == DropTarget::Top)
          layerIdx = lastLayer;

        Layer* layer = (validLayer(layerIdx) ? m_layers[layerIdx].layer: nullptr);
        if (layer) {
          switch (m_dropTarget.vhit) {
            case DropTarget::Bottom:
              sb->setStatusText(0, "%s at bottom of layer %s", verb, layer->name().c_str());
              return;
            case DropTarget::Top:
              sb->setStatusText(0, "%s at top of layer %s", verb, layer->name().c_str());
              return;
            case DropTarget::FirstChild:
              sb->setStatusText(0, "%s as first child of layer %s", verb, layer->name().c_str());
              return;
          }
        }
        break;
      }

    }
  }
  else {
    Layer* layer = (validLayer(m_hot.layer) ? m_layers[m_hot.layer].layer:
                                              nullptr);

    switch (m_hot.part) {

      case PART_HEADER_ONIONSKIN: {
        sb->setStatusText(0, "Onionskin is %s",
          docPref().onionskin.active() ? "enabled": "disabled");
        return;
      }

      case PART_LAYER_TEXT:
        if (layer != NULL) {
          sb->setStatusText(
            0, "%s '%s' [%s%s]",
            layer->isReference() ? "Reference layer": "Layer",
            layer->name().c_str(),
            layer->isVisible() ? "visible": "hidden",
            layer->isEditable() ? "": " locked");
          return;
        }
        break;

      case PART_LAYER_EYE_ICON:
        if (layer != NULL) {
          sb->setStatusText(0, "Layer '%s' is %s",
            layer->name().c_str(),
            layer->isVisible() ? "visible": "hidden");
          return;
        }
        break;

      case PART_LAYER_PADLOCK_ICON:
        if (layer != NULL) {
          sb->setStatusText(0, "Layer '%s' is %s",
            layer->name().c_str(),
            layer->isEditable() ? "unlocked (editable)": "locked (read-only)");
          return;
        }
        break;

      case PART_LAYER_CONTINUOUS_ICON:
        if (layer) {
          if (layer->isImage())
            sb->setStatusText(0, "Layer '%s' is %s (%s)",
                              layer->name().c_str(),
                              layer->isContinuous() ? "continuous": "discontinuous",
                              layer->isContinuous() ? "prefer linked cels/frames": "prefer individual cels/frames");
          else if (layer->isGroup())
            sb->setStatusText(0, "Group '%s'", layer->name().c_str());
          return;
        }
        break;

      case PART_HEADER_FRAME:
        if (validFrame(m_hot.frame)) {
          sb->setStatusText(
            0,
            ":frame: %d :clock: %d",
            (int)m_hot.frame+docPref().timeline.firstFrame(),
            m_sprite->frameDuration(m_hot.frame));
          return;
        }
        break;

      case PART_CEL:
        if (layer) {
          Cel* cel = (layer->isImage() ? layer->cel(m_hot.frame): NULL);
          StatusBar::instance()->setStatusText(0,
            "%s at frame %d"
#ifdef _DEBUG
            " (Image %d)"
#endif
            , cel ? "Cel": "Empty cel"
            , (int)m_hot.frame+1
#ifdef _DEBUG
            , (cel ? cel->image()->id(): 0)
#endif
            );
          return;
        }
        break;
    }
  }

  sb->clearText();
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

  // Here we use <= instead of < to avoid jumping between this
  // condition and the "else if" one when we are playing the
  // animation.
  if (celBounds.x <= viewport.x) {
    scroll.x -= viewport.x - celBounds.x;
  }
  else if (celBounds.x2() > viewport.x2()) {
    scroll.x += celBounds.x2() - viewport.x2();
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
      (m_layers.size()+1) * layerBoxHeight());
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
    max_scroll_x = MAX(0, max_scroll_x);
    max_scroll_y = MAX(0, max_scroll_y);
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
  for (size_t i=0; i<m_layers.size(); i++)
    if (!m_layers[i].layer->isContinuous())
      return false;

  return true;
}

bool Timeline::allLayersDiscontinuous()
{
  for (size_t i=0; i<m_layers.size(); i++)
    if (m_layers[i].layer->isContinuous())
      return false;

  return true;
}

layer_t Timeline::getLayerIndex(const Layer* layer) const
{
  for (int i=0; i<(int)m_layers.size(); i++)
    if (m_layers[i].layer == layer)
      return i;

  return -1;
}

bool Timeline::isLayerActive(const layer_t layerIndex) const
{
  if (layerIndex == getLayerIndex(m_layer))
    return true;
  else
    return m_range.contains(m_layers[layerIndex].layer);
}

bool Timeline::isFrameActive(const frame_t frame) const
{
  if (frame == m_frame)
    return true;
  else
    return m_range.contains(frame);
}

void Timeline::dropRange(DropOp op)
{
  bool copy = (op == Timeline::kCopy);
  Range newFromRange;
  DocumentRangePlace place = kDocumentRangeAfter;
  Range dropRange = m_dropRange;

  switch (m_range.type()) {

    case Range::kFrames:
      if (m_dropTarget.hhit == DropTarget::Before)
        place = kDocumentRangeBefore;
      break;

    case Range::kLayers:
      switch (m_dropTarget.vhit) {
        case DropTarget::Bottom:
          place = kDocumentRangeBefore;
          break;
        case DropTarget::FirstChild:
          place = kDocumentRangeFirstChild;
          break;
        case DropTarget::VeryBottom:
          place = kDocumentRangeBefore;
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
    if (copy)
      newFromRange = copy_range(m_document, m_range, dropRange, place);
    else
      newFromRange = move_range(m_document, m_range, dropRange, place);

    // If we drop a cel in the same frame (but in another layer),
    // document views are not updated, so we are forcing the updating of
    // all views.
    m_document->notifyGeneralUpdate();

    moveRange(newFromRange);
  }
  catch (const std::exception& e) {
    ui::Alert::show("Problem<<%s||&OK", e.what());
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
  newScroll.x = MID(0, newScroll.x, maxPos.x);
  newScroll.y = MID(0, newScroll.y, maxPos.y);

  if (newScroll == oldScroll)
    return;

  m_hbar.setPos(newScroll.x);
  m_vbar.setPos(newScroll.y);
  invalidate();
}

void Timeline::updateDropRange(const gfx::Point& pt)
{
  DropTarget::HHit oldHHit = m_dropTarget.hhit;
  DropTarget::VHit oldVHit = m_dropTarget.vhit;
  m_dropTarget.hhit = DropTarget::HNone;
  m_dropTarget.vhit = DropTarget::VNone;

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
      m_dropRange.startRange(m_layers[m_hot.layer].layer, m_hot.frame, m_range.type());
      m_dropRange.endRange(m_layers[m_hot.layer].layer, m_hot.frame);
      break;
  }

  gfx::Rect bounds = getRangeBounds(m_dropRange);

  if (pt.x < bounds.x + bounds.w/2)
    m_dropTarget.hhit = DropTarget::Before;
  else
    m_dropTarget.hhit = DropTarget::After;

  if (m_hot.veryBottom)
    m_dropTarget.vhit = DropTarget::VeryBottom;
  else if (pt.y < bounds.y + bounds.h/2)
    m_dropTarget.vhit = DropTarget::Top;
  // Special drop target for expanded groups
  else if (m_range.type() == Range::kLayers &&
           m_hot.layer >= 0 &&
           m_hot.layer < m_layers.size() &&
           m_layers[m_hot.layer].layer->isGroup() &&
           static_cast<LayerGroup*>(m_layers[m_hot.layer].layer)->isExpanded()) {
    m_dropTarget.vhit = DropTarget::FirstChild;
  }
  else {
    m_dropTarget.vhit = DropTarget::Bottom;
  }

  if (oldHHit != m_dropTarget.hhit ||
      oldVHit != m_dropTarget.vhit) {
    invalidate();
  }
}

void Timeline::clearClipboardRange()
{
  Document* clipboard_document;
  DocumentRange clipboard_range;
  clipboard::get_document_range_info(
    &clipboard_document,
    &clipboard_range);

  if (!m_document || clipboard_document != m_document)
    return;

  clipboard::clear_content();
  m_clipboard_timer.stop();
}

bool Timeline::isCopyKeyPressed(ui::Message* msg)
{
  return msg->ctrlPressed() ||  // Ctrl is common on Windows
         msg->altPressed();    // Alt is common on Mac OS X
}

DocumentPreferences& Timeline::docPref() const
{
  return Preferences::instance().document(m_document);
}

skin::SkinTheme* Timeline::skinTheme() const
{
  return static_cast<SkinTheme*>(theme());
}

gfx::Size Timeline::celBoxSize() const
{
  return gfx::Size(frameBoxWidth(), layerBoxHeight());
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
  return int(m_zoom*skinTheme()->dimensions.timelineBaseSize() + int(m_zoom > 1) * headerBoxHeight());
}

int Timeline::frameBoxWidth() const
{
  return int(m_zoom*skinTheme()->dimensions.timelineBaseSize());
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

// Returns the last frame where the frame tag (or frame tag label)
// is visible in the timeline.
int Timeline::calcTagVisibleToFrame(FrameTag* frameTag) const
{
  return
    MAX(frameTag->toFrame(),
        frameTag->fromFrame() +
        font()->textLength(frameTag->name())/frameBoxWidth());
}

int Timeline::topHeight() const
{
  int h = 0;
  if (m_document && m_sprite) {
    h += skinTheme()->dimensions.timelineTopBorder();
    h += oneTagHeight() * MAX(1, m_tagBands);
  }
  return h;
}

FrameTag* Timeline::Hit::getFrameTag() const
{
  return get<FrameTag>(frameTag);
}

void Timeline::onNewInputPriority(InputChainElement* element)
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
    m_range.clearRange();
    invalidate();
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
    (clipboard::get_current_format() == clipboard::ClipboardDocumentRange &&
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
      clipboard::copy_range(reader, m_range);
      return true;
    }
  }
  return false;
}

bool Timeline::onPaste(Context* ctx)
{
  if (clipboard::get_current_format() == clipboard::ClipboardDocumentRange) {
    clipboard::paste();
    return true;
  }
  else
    return false;
}

bool Timeline::onClear(Context* ctx)
{
  if (!m_document || !m_sprite || !m_range.enabled())
    return false;

  Command* cmd = nullptr;

  switch (m_range.type()) {
    case DocumentRange::kCels:
      cmd = CommandsModule::instance()->getCommandByName(CommandId::ClearCel);
      break;
    case DocumentRange::kFrames:
      cmd = CommandsModule::instance()->getCommandByName(CommandId::RemoveFrame);
      break;
    case DocumentRange::kLayers:
      cmd = CommandsModule::instance()->getCommandByName(CommandId::RemoveLayer);
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
  m_range.clearRange();
  clearClipboardRange();
  invalidate();
}

} // namespace app
