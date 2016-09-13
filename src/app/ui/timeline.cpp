// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/timeline.h"

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
#include "app/transaction.h"
#include "app/ui/app_menuitem.h"
#include "app/ui/configure_timeline_popup.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/input_chain.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "app/ui/status_bar.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "app/util/clipboard.h"
#include "base/convert_to.h"
#include "base/memory.h"
#include "base/scoped_value.h"
#include "doc/doc.h"
#include "doc/document_event.h"
#include "doc/frame_tag.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "she/font.h"
#include "ui/scroll_helper.h"
#include "ui/ui.h"

#include <cstdio>
#include <vector>

// Size of the thumbnail in the screen (width x height), the really
// size of the thumbnail bitmap is specified in the
// 'generate_thumbnail' routine.
#define THUMBSIZE       (12*guiscale())

// Height of the headers.
#define HDRSIZE         THUMBSIZE

// Width of the frames.
#define FRMSIZE         THUMBSIZE

// Height of the layers.
#define LAYSIZE         THUMBSIZE

// Space between icons and other information in the layer.
#define ICONSEP         (2*guiscale())

#define OUTLINE_WIDTH   (skinTheme()->dimensions.timelineOutlineWidth())

// Space between the icon-bitmap and the edge of the surrounding button.
#define ICONBORDER      0

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

Timeline::Timeline()
  : Widget(kGenericWidget)
  , m_hbar(HORIZONTAL, this)
  , m_vbar(VERTICAL, this)
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

  int barsize = skinTheme()->dimensions.miniScrollbarSize();
  m_hbar.setBarWidth(barsize);
  m_vbar.setBarWidth(barsize);
  m_hbar.setBgColor(gfx::rgba(0, 0, 0, 128));
  m_vbar.setBgColor(gfx::rgba(0, 0, 0, 128));
  m_hbar.setTransparent(true);
  m_vbar.setTransparent(true);
}

Timeline::~Timeline()
{
  m_clipboard_timer.stop();

  detachDocument();
  m_context->documents().remove_observer(this);
  delete m_confPopup;
}

void Timeline::updateUsingEditor(Editor* editor)
{
  m_aniControls.updateUsingEditor(editor);

  detachDocument();

  if (m_range.enabled()) {
    m_range.disableRange();
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

  setFocusStop(true);
  regenerateLayers();
  setViewScroll(viewScroll());
  showCurrentCel();
}

void Timeline::detachDocument()
{
  if (m_document) {
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

void Timeline::setLayer(Layer* layer)
{
  ASSERT(m_editor != NULL);

  m_layer = layer;
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
      m_editor->play(false);
  }
}

void Timeline::prepareToMoveRange()
{
  ASSERT(m_range.enabled());

  m_moveRangeData.activeRelativeLayer = getLayerIndex(m_layer) - m_range.layerBegin();
  m_moveRangeData.activeRelativeFrame = m_frame - m_range.frameBegin();
}

void Timeline::moveRange(Range& range)
{
  regenerateLayers();

  if (range.layerBegin() >= LayerIndex(0) &&
      range.layerBegin() + m_moveRangeData.activeRelativeLayer < int(m_layers.size())) {
    setLayer(m_layers[range.layerBegin() + m_moveRangeData.activeRelativeLayer]);
  }

  if (range.frameBegin() >= frame_t(0))
    setFrame(range.frameBegin() + m_moveRangeData.activeRelativeFrame, true);

  m_range = range;
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

      if (mouseMsg->middle() || she::is_key_pressed(kKeySpace)) {
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
            m_range.startRange(getLayerIndex(m_layer), m_clk.frame, Range::kFrames);

            setFrame(m_clk.frame, true);
          }
          break;
        }
        case PART_LAYER_TEXT: {
          base::ScopedValue<bool> lock(m_fromTimeline, true, false);
          LayerIndex old_layer = getLayerIndex(m_layer);
          bool selectLayer = (mouseMsg->left() || !isLayerActive(m_clk.layer));

          if (selectLayer) {
            m_state = STATE_SELECTING_LAYERS;
            m_range.startRange(m_clk.layer, m_frame, Range::kLayers);

            // Did the user select another layer?
            if (old_layer != m_clk.layer) {
              setLayer(m_layers[m_clk.layer]);
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
          LayerIndex old_layer = getLayerIndex(m_layer);
          bool selectCel = (mouseMsg->left()
            || !isLayerActive(m_clk.layer)
            || !isFrameActive(m_clk.frame));
          frame_t old_frame = m_frame;

          if (selectCel) {
            m_state = STATE_SELECTING_CELS;
            m_range.startRange(m_clk.layer, m_clk.frame, Range::kCels);
          }

          // Select the new clicked-part.
          if (old_layer != m_clk.layer
            || old_frame != m_clk.frame) {
            setLayer(m_layers[m_clk.layer]);
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

            if (m_clk.layer < m_range.layerBegin())
              m_clk.layer = m_range.layerBegin();
            else if (m_clk.layer > m_range.layerEnd())
              m_clk.layer = m_range.layerEnd();

            if (m_clk.frame < m_range.frameBegin())
              m_clk.frame = m_range.frameBegin();
            else if (m_clk.frame > m_range.frameEnd())
              m_clk.frame = m_range.frameEnd();
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
            if (m_layer != m_layers[hit.layer]) {
              m_range.endRange(hit.layer, m_frame);
              setLayer(m_layers[m_clk.layer = hit.layer]);
            }
            break;
          }

          case STATE_SELECTING_FRAMES: {
            m_range.endRange(getLayerIndex(m_layer), hit.frame);
            setFrame(m_clk.frame = hit.frame, true);
            break;
          }

          case STATE_SELECTING_CELS:
            if ((m_layer != m_layers[hit.layer])
              || (m_frame != hit.frame)) {
              m_range.endRange(hit.layer, hit.frame);
              setLayer(m_layers[m_clk.layer = hit.layer]);
              setFrame(m_clk.frame = hit.frame, true);
            }
            break;
        }
      }

      updateStatusBar(msg);
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

        setHot(hitTest(msg, mouseMsg->position() - bounds().origin()));

        switch (m_hot.part) {

          case PART_NOTHING:
          case PART_SEPARATOR:
          case PART_HEADER_LAYER:
            // Do nothing.
            break;

          case PART_HEADER_EYE: {
            bool newVisibleState = !allLayersVisible();
            for (size_t i=0; i<m_layers.size(); i++)
              m_layers[i]->setVisible(newVisibleState);

            // Redraw all views.
            m_document->notifyGeneralUpdate();
            break;
          }

          case PART_HEADER_PADLOCK: {
            bool newEditableState = !allLayersUnlocked();
            for (size_t i=0; i<m_layers.size(); i++)
              m_layers[i]->setEditable(newEditableState);
            break;
          }

          case PART_HEADER_CONTINUOUS: {
            bool newContinuousState = !allLayersContinuous();
            for (size_t i=0; i<m_layers.size(); i++)
              m_layers[i]->setContinuous(newContinuousState);
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
              m_confPopup->moveWindow(gfx::Rect(
                  gearBounds.x,
                  gearBounds.y-m_confPopup->bounds().h,
                  m_confPopup->bounds().w,
                  m_confPopup->bounds().h));
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
              Layer* layer = m_layers[m_clk.layer];
              ASSERT(layer != NULL);
              layer->setVisible(!layer->isVisible());

              // Redraw all views.
              m_document->notifyGeneralUpdate();
            }
            break;

          case PART_LAYER_PADLOCK_ICON:
            // Lock/unlock layer.
            if (m_hot.layer == m_clk.layer && validLayer(m_hot.layer)) {
              Layer* layer = m_layers[m_clk.layer];
              ASSERT(layer != NULL);
              layer->setEditable(!layer->isEditable());
            }
            break;

          case PART_LAYER_CONTINUOUS_ICON:
            if (m_hot.layer == m_clk.layer && validLayer(m_hot.layer)) {
              Layer* layer = m_layers[m_clk.layer];
              ASSERT(layer != NULL);
              layer->setContinuous(!layer->isContinuous());
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
            m_range.disableRange();
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
          she::clear_keyboard_buffer();
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
        int dz = static_cast<MouseMessage*>(msg)->wheelDelta().y;
        int dx = 0;
        int dy = 0;

        dx += static_cast<MouseMessage*>(msg)->wheelDelta().x;

        if (msg->ctrlPressed())
          dx = dz * FRMSIZE;
        else
          dy = dz * LAYSIZE;

        if (msg->shiftPressed()) {
          dx *= 3;
          dy *= 3;
        }

        setViewScroll(viewScroll() + gfx::Point(dx, dy));
      }
      break;

    case kSetCursorMessage:
      if (m_document) {
        setCursor(msg, m_hot);
        return true;
      }
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
    gfx::Rect(rc.x, rc.y, MIN(sz.w, m_separator_x),
      font()->height() +
      skinTheme()->dimensions.timelineTagsAreaHeight()));

  updateScrollBars();
}

void Timeline::onPaint(ui::PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  bool noDoc = (m_document == NULL);
  if (noDoc)
    goto paintNoDoc;

  try {
    // Lock the sprite to read/render it.
    const DocumentReader documentReader(m_document, 0);

    LayerIndex layer, first_layer, last_layer;
    frame_t frame, first_frame, last_frame;

    getDrawableLayers(g, &first_layer, &last_layer);
    getDrawableFrames(g, &first_frame, &last_frame);

    drawTop(g);

    // Draw the header for layers.
    drawHeader(g);

    // Draw the header for each visible frame.
    {
      IntersectClip clip(g, getFrameHeadersBounds());
      if (clip) {
        for (frame=first_frame; frame<=last_frame; ++frame)
          drawHeaderFrame(g, frame);

        // Draw onionskin indicators.
        gfx::Rect bounds = getOnionskinFramesBounds();
        if (!bounds.isEmpty()) {
          drawPart(g, bounds,
            NULL, skinTheme()->styles.timelineOnionskinRange(),
            false, false, false);
        }
      }
    }

    // Draw each visible layer.
    DrawCelData data;
    for (layer=last_layer; layer>=first_layer; --layer) {
      {
        IntersectClip clip(g, getLayerHeadersBounds());
        if (clip)
          drawLayer(g, layer);
      }

      IntersectClip clip(g, getCelsBounds());
      if (!clip)
        continue;

      // Get the first CelIterator to be drawn (it is the first cel with cel->frame >= first_frame)
      LayerImage* layerPtr = static_cast<LayerImage*>(m_layers[layer]);
      data.begin = layerPtr->getCelBegin();
      data.end = layerPtr->getCelEnd();
      data.it = layerPtr->findFirstCelIteratorAfter(first_frame-1);
      data.prevIt = data.end;
      data.nextIt = (data.it != data.end ? data.it+1: data.end);

      // Calculate link range for the active cel
      data.firstLink = data.end;
      data.lastLink = data.end;

      if (layerPtr == m_layer) {
        data.activeIt = layerPtr->findCelIterator(m_frame);
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
                if ((*data.firstLink)->frame() < first_frame)
                  break;
              }
            } while (it2 != data.begin);
          }

          it2 = data.activeIt;
          while (it2 != data.end) {
            if ((*it2)->image()->id() == imageId) {
              data.lastLink = it2;
              if ((*data.lastLink)->frame() > last_frame)
                break;
            }
            ++it2;
          }
        }
      }
      else
        data.activeIt = data.end;

      // Draw every visible cel for each layer.
      for (frame=first_frame; frame<=last_frame; ++frame) {
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
    drawPart(g, clientBounds(), NULL,
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
  if (document == m_document)
    detachDocument();
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
  invalidate();
}

void Timeline::onAfterRemoveLayer(doc::DocumentEvent& ev)
{
  Sprite* sprite = ev.sprite();
  Layer* layer = ev.layer();

  // If the layer that was removed is the selected one
  if (layer == getLayer()) {
    LayerFolder* parent = layer->parent();
    Layer* layer_select = NULL;

    // Select previous layer, or next layer, or the parent (if it is
    // not the main layer of sprite set).
    if (layer->getPrevious())
      layer_select = layer->getPrevious();
    else if (layer->getNext())
      layer_select = layer->getNext();
    else if (parent != sprite->folder())
      layer_select = parent;

    setLayer(layer_select);
  }

  regenerateLayers();
  showCurrentCel();
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
    m_range.disableRange();

  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

void Timeline::onSelectionChanged(doc::DocumentEvent& ev)
{
  m_range.disableRange();
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
    m_range.disableRange();

  showCurrentCel();
  invalidate();
}

void Timeline::onAfterLayerChanged(Editor* editor)
{
  if (m_fromTimeline)
    return;

  setLayer(editor->layer());

  if (!hasCapture())
    m_range.disableRange();

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

void Timeline::getDrawableLayers(ui::Graphics* g, LayerIndex* first_layer, LayerIndex* last_layer)
{
  int hpx = (clientBounds().h - HDRSIZE - topHeight());
  LayerIndex i = lastLayer() - LayerIndex((viewScroll().y+hpx) / LAYSIZE);
  i = MID(firstLayer(), i, lastLayer());

  LayerIndex j = i + LayerIndex(hpx / LAYSIZE + 1);
  if (!m_layers.empty())
    j = MID(firstLayer(), j, lastLayer());
  else
    j = LayerIndex::NoLayer;

  *first_layer = i;
  *last_layer = j;
}

void Timeline::getDrawableFrames(ui::Graphics* g, frame_t* first_frame, frame_t* last_frame)
{
  int availW = (clientBounds().w - m_separator_x);

  *first_frame = frame_t(viewScroll().x / FRMSIZE);
  *last_frame = *first_frame + frame_t(availW / FRMSIZE) + ((availW % FRMSIZE) > 0 ? 1: 0);
}

void Timeline::drawPart(ui::Graphics* g, const gfx::Rect& bounds,
  const char* text, Style* style,
  bool is_active, bool is_hover, bool is_clicked)
{
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  Style::State state;
  if (is_active) state += Style::active();
  if (is_hover) state += Style::hover();
  if (is_clicked) state += Style::clicked();

  style->paint(g, bounds, text, state);
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

  CheckedDrawMode checked(g, m_offset_count);
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
  SkinTheme::Styles& styles = skinTheme()->styles;
  bool allInvisible = allLayersInvisible();
  bool allLocked = allLayersLocked();
  bool allContinuous = allLayersContinuous();

  drawPart(g, getPartBounds(Hit(PART_HEADER_EYE)),
    NULL,
    allInvisible ? styles.timelineClosedEye(): styles.timelineOpenEye(),
    m_clk.part == PART_HEADER_EYE,
    m_hot.part == PART_HEADER_EYE,
    m_clk.part == PART_HEADER_EYE);

  drawPart(g, getPartBounds(Hit(PART_HEADER_PADLOCK)),
    NULL,
    allLocked ? styles.timelineClosedPadlock(): styles.timelineOpenPadlock(),
    m_clk.part == PART_HEADER_PADLOCK,
    m_hot.part == PART_HEADER_PADLOCK,
    m_clk.part == PART_HEADER_PADLOCK);

  drawPart(g, getPartBounds(Hit(PART_HEADER_CONTINUOUS)),
    NULL,
    allContinuous ? styles.timelineContinuous(): styles.timelineDiscontinuous(),
    m_clk.part == PART_HEADER_CONTINUOUS,
    m_hot.part == PART_HEADER_CONTINUOUS,
    m_clk.part == PART_HEADER_CONTINUOUS);

  drawPart(g, getPartBounds(Hit(PART_HEADER_GEAR)),
    NULL, styles.timelineGear(),
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
  char buf[256];
  std::sprintf(buf, "%d", (frame+1)%100); // Draw only the first two digits.

  she::Font* oldFont = g->font();
  g->setFont(skinTheme()->getMiniFont());
  drawPart(g, bounds, buf, skinTheme()->styles.timelineBox(), is_active, is_hover, is_clicked);
  g->setFont(oldFont);
}

void Timeline::drawLayer(ui::Graphics* g, LayerIndex layerIdx)
{
  SkinTheme::Styles& styles = skinTheme()->styles;
  Layer* layer = m_layers[layerIdx];
  bool is_active = isLayerActive(layerIdx);
  bool hotlayer = (m_hot.layer == layerIdx);
  bool clklayer = (m_clk.layer == layerIdx);
  gfx::Rect bounds = getPartBounds(Hit(PART_LAYER, layerIdx, firstFrame()));
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  // Draw the eye (visible flag).
  bounds = getPartBounds(Hit(PART_LAYER_EYE_ICON, layerIdx));
  drawPart(g, bounds, NULL,
    layer->isVisible() ? styles.timelineOpenEye(): styles.timelineClosedEye(),
    is_active,
    (hotlayer && m_hot.part == PART_LAYER_EYE_ICON),
    (clklayer && m_clk.part == PART_LAYER_EYE_ICON));

  // Draw the padlock (editable flag).
  bounds = getPartBounds(Hit(PART_LAYER_PADLOCK_ICON, layerIdx));
  drawPart(g, bounds, NULL,
    layer->isEditable() ? styles.timelineOpenPadlock(): styles.timelineClosedPadlock(),
    is_active,
    (hotlayer && m_hot.part == PART_LAYER_PADLOCK_ICON),
    (clklayer && m_clk.part == PART_LAYER_PADLOCK_ICON));

  // Draw the continuous flag.
  bounds = getPartBounds(Hit(PART_LAYER_CONTINUOUS_ICON, layerIdx));
  drawPart(g, bounds, NULL,
    layer->isContinuous() ? styles.timelineContinuous(): styles.timelineDiscontinuous(),
    is_active,
    (hotlayer && m_hot.part == PART_LAYER_CONTINUOUS_ICON),
    (clklayer && m_clk.part == PART_LAYER_CONTINUOUS_ICON));

  // Get the layer's name bounds.
  bounds = getPartBounds(Hit(PART_LAYER_TEXT, layerIdx));

  // Draw layer name.
  doc::color_t layerColor = layer->userData().color();
  if (doc::rgba_geta(layerColor) > 0) {
    drawPart(g, bounds, nullptr, styles.timelineLayer(),
             is_active,
             (hotlayer && m_hot.part == PART_LAYER_TEXT),
             (clklayer && m_clk.part == PART_LAYER_TEXT));

    // Fill with an user-defined custom color.
    auto b2 = bounds;
    b2.shrink(1*guiscale()).inflate(1*guiscale());
    g->fillRect(gfx::rgba(doc::rgba_getr(layerColor),
                          doc::rgba_getg(layerColor),
                          doc::rgba_getb(layerColor),
                          doc::rgba_geta(layerColor)),
                b2);

    drawPart(g, bounds, layer->name().c_str(), styles.timelineLayerTextOnly(),
             is_active,
             (hotlayer && m_hot.part == PART_LAYER_TEXT),
             (clklayer && m_clk.part == PART_LAYER_TEXT));
  }
  else {
    drawPart(g, bounds, layer->name().c_str(), styles.timelineLayer(),
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

void Timeline::drawCel(ui::Graphics* g, LayerIndex layerIndex, frame_t frame, Cel* cel, DrawCelData* data)
{
  SkinTheme::Styles& styles = skinTheme()->styles;
  LayerImage* layer = static_cast<LayerImage*>(m_layers[layerIndex]);
  Image* image = (cel ? cel->image(): NULL);
  bool is_hover = (m_hot.part == PART_CEL &&
    m_hot.layer == layerIndex &&
    m_hot.frame == frame);
  bool is_active = (isLayerActive(layerIndex) || isFrameActive(frame));
  bool is_empty = (image == NULL);
  gfx::Rect bounds = getPartBounds(Hit(PART_CEL, layerIndex, frame));
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  if (layer == m_layer && frame == m_frame)
    drawPart(g, bounds, NULL, styles.timelineSelectedCel(), false, false, true);
  else
    drawPart(g, bounds, NULL, styles.timelineBox(), is_active, is_hover);

  // Fill with an user-defined custom color.
  if (cel && cel->data()) {
    doc::color_t celColor = cel->data()->userData().color();
    if (doc::rgba_geta(celColor) > 0) {
      auto b2 = bounds;
      b2.shrink(1*guiscale()).inflate(1*guiscale());
      g->fillRect(gfx::rgba(doc::rgba_getr(celColor),
                            doc::rgba_getg(celColor),
                            doc::rgba_getb(celColor),
                            doc::rgba_geta(celColor)),
                  b2);
    }
  }

  skin::Style* style;
  bool fromLeft = false;
  bool fromRight = false;
  if (is_empty) {
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
  drawPart(g, bounds, NULL, style, is_active, is_hover);

  // Draw decorators to link the activeCel with its links.
  if (data->activeIt != data->end)
    drawCelLinkDecorators(g, bounds, cel, frame, is_active, is_hover, data);
}

void Timeline::drawCelLinkDecorators(ui::Graphics* g, const gfx::Rect& bounds,
                                     Cel* cel, frame_t frame, bool is_active, bool is_hover,
                                     DrawCelData* data)
{
  SkinTheme::Styles& styles = skinTheme()->styles;
  ObjectId imageId = (*data->activeIt)->image()->id();

  // Links at the left or right side
  bool left = (data->firstLink != data->end ? frame > (*data->firstLink)->frame(): false);
  bool right = (data->lastLink != data->end ? frame < (*data->lastLink)->frame(): false);

  if (cel && cel->image()->id() == imageId) {
    if (left) {
      Cel* prevCel = m_layer->cel(cel->frame()-1);
      if (!prevCel || prevCel->image()->id() != imageId)
        drawPart(g, bounds, NULL, styles.timelineLeftLink(), is_active, is_hover);
    }
    if (right) {
      Cel* nextCel = m_layer->cel(cel->frame()+1);
      if (!nextCel || nextCel->image()->id() != imageId)
        drawPart(g, bounds, NULL, styles.timelineRightLink(), is_active, is_hover);
    }
  }
  else {
    if (left && right)
      drawPart(g, bounds, NULL, styles.timelineBothLinks(), is_active, is_hover);
  }
}

void Timeline::drawFrameTags(ui::Graphics* g)
{
  IntersectClip clip(g, getPartBounds(Hit(PART_HEADER_FRAME_TAGS)));
  if (!clip)
    return;

  SkinTheme* theme = skinTheme();
  SkinTheme::Styles& styles = theme->styles;

  g->fillRect(theme->colors.workspace(),
    gfx::Rect(
      0, font()->height(),
      clientBounds().w,
      theme->dimensions.timelineTagsAreaHeight()));

  for (FrameTag* frameTag : m_sprite->frameTags()) {
    gfx::Rect bounds1 = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), frameTag->fromFrame()));
    gfx::Rect bounds2 = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), frameTag->toFrame()));
    gfx::Rect bounds = bounds1.createUnion(bounds2);
    bounds.y -= theme->dimensions.timelineTagsAreaHeight();

    {
      IntersectClip clip(g, bounds);
      if (clip)
        drawPart(g, bounds, NULL, styles.timelineLoopRange());
    }

    {
      bounds = getPartBounds(Hit(PART_FRAME_TAG, LayerIndex(0), 0, frameTag->id()));

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
      g->drawString(
        frameTag->name(),
        color_utils::blackandwhite_neg(bg),
        gfx::ColorNone,
        bounds.origin());
    }
  }
}

void Timeline::drawRangeOutline(ui::Graphics* g)
{
  SkinTheme::Styles& styles = skinTheme()->styles;

  gfx::Rect clipBounds;
  switch (m_range.type()) {
    case Range::kCels: clipBounds = getCelsBounds(); break;
    case Range::kFrames: clipBounds = getFrameHeadersBounds(); break;
    case Range::kLayers: clipBounds = getLayerHeadersBounds(); break;
  }
  IntersectClip clip(g, clipBounds.enlarge(OUTLINE_WIDTH));
  if (!clip)
    return;

  Style::State state;
  if (m_range.enabled()) state += Style::active();
  if (m_hot.part == PART_RANGE_OUTLINE) state += Style::hover();

  gfx::Rect bounds = getPartBounds(Hit(PART_RANGE_OUTLINE));
  styles.timelineRangeOutline()->paint(g, bounds, NULL, state);

  Range drop = m_dropRange;
  gfx::Rect dropBounds = getRangeBounds(drop);

  switch (drop.type()) {

    case Range::kCels: {
      dropBounds = dropBounds.enlarge(OUTLINE_WIDTH);
      styles.timelineRangeOutline()->paint(g, dropBounds, NULL, Style::active());
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

      styles.timelineDropFrameDeco()->paint(g, dropBounds, NULL, Style::State());
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

      styles.timelineDropLayerDeco()->paint(g, dropBounds, NULL, Style::State());
      break;
    }
  }
}

void Timeline::drawPaddings(ui::Graphics* g)
{
  SkinTheme::Styles& styles = skinTheme()->styles;

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
  int h = topHeight() + HDRSIZE;
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
  rc.h = HDRSIZE;
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
  rc.y += HDRSIZE + topHeight();
  rc.h -= HDRSIZE + topHeight();
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
      return gfx::Rect(bounds.x + FRMSIZE*0, bounds.y + y, FRMSIZE, HDRSIZE);

    case PART_HEADER_PADLOCK:
      return gfx::Rect(bounds.x + FRMSIZE*1, bounds.y + y, FRMSIZE, HDRSIZE);

    case PART_HEADER_CONTINUOUS:
      return gfx::Rect(bounds.x + FRMSIZE*2, bounds.y + y, FRMSIZE, HDRSIZE);

    case PART_HEADER_GEAR:
      return gfx::Rect(bounds.x + FRMSIZE*3, bounds.y + y, FRMSIZE, HDRSIZE);

    case PART_HEADER_ONIONSKIN:
      return gfx::Rect(bounds.x + FRMSIZE*4, bounds.y + y, FRMSIZE, HDRSIZE);

    case PART_HEADER_LAYER:
      return gfx::Rect(bounds.x + FRMSIZE*5, bounds.y + y,
        m_separator_x - FRMSIZE*5, HDRSIZE);

    case PART_HEADER_FRAME:
      return gfx::Rect(
        bounds.x + m_separator_x + m_separator_w - 1
        + FRMSIZE*MAX(firstFrame(), hit.frame) - viewScroll().x,
        bounds.y + y, FRMSIZE, HDRSIZE);

    case PART_HEADER_FRAME_TAGS:
      return gfx::Rect(
          bounds.x + m_separator_x + m_separator_w - 1,
          bounds.y,
          bounds.w - m_separator_x - m_separator_w + 1, y);

    case PART_LAYER:
      if (validLayer(hit.layer)) {
        return gfx::Rect(bounds.x,
          bounds.y + y + HDRSIZE + LAYSIZE*(lastLayer()-hit.layer) - viewScroll().y,
          m_separator_x, LAYSIZE);
      }
      break;

    case PART_LAYER_EYE_ICON:
      if (validLayer(hit.layer)) {
        return gfx::Rect(bounds.x,
          bounds.y + y + HDRSIZE + LAYSIZE*(lastLayer()-hit.layer) - viewScroll().y,
          FRMSIZE, LAYSIZE);
      }
      break;

    case PART_LAYER_PADLOCK_ICON:
      if (validLayer(hit.layer)) {
        return gfx::Rect(bounds.x + FRMSIZE,
          bounds.y + y + HDRSIZE + LAYSIZE*(lastLayer()-hit.layer) - viewScroll().y,
          FRMSIZE, LAYSIZE);
      }
      break;

    case PART_LAYER_CONTINUOUS_ICON:
      if (validLayer(hit.layer)) {
        return gfx::Rect(bounds.x + 2*FRMSIZE,
          bounds.y + y + HDRSIZE + LAYSIZE*(lastLayer()-hit.layer) - viewScroll().y,
          FRMSIZE, LAYSIZE);
      }
      break;

    case PART_LAYER_TEXT:
      if (validLayer(hit.layer)) {
        int x = FRMSIZE*3;
        return gfx::Rect(bounds.x + x,
          bounds.y + y + HDRSIZE + LAYSIZE*(lastLayer()-hit.layer) - viewScroll().y,
          m_separator_x - x, LAYSIZE);
      }
      break;

    case PART_CEL:
      if (validLayer(hit.layer) && hit.frame >= frame_t(0)) {
        return gfx::Rect(
          bounds.x + m_separator_x + m_separator_w - 1 + FRMSIZE*hit.frame - viewScroll().x,
          bounds.y + y + HDRSIZE + LAYSIZE*(lastLayer()-hit.layer) - viewScroll().y,
          FRMSIZE, LAYSIZE);
      }
      break;

    case PART_RANGE_OUTLINE: {
      gfx::Rect rc = getRangeBounds(m_range);
      int s = OUTLINE_WIDTH;
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
    case Range::kNone: break; // Return empty rectangle
    case Range::kCels:
      rc = getPartBounds(Hit(PART_CEL, range.layerBegin(), range.frameBegin())).createUnion(
        getPartBounds(Hit(PART_CEL, range.layerEnd(), range.frameEnd())));
      break;
    case Range::kFrames:
      rc = getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), range.frameBegin())).createUnion(
        getPartBounds(Hit(PART_HEADER_FRAME, firstLayer(), range.frameEnd())));
      break;
    case Range::kLayers:
      rc = getPartBounds(Hit(PART_LAYER, range.layerBegin())).createUnion(
        getPartBounds(Hit(PART_LAYER, range.layerEnd())));
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
  ASSERT(m_document != NULL);
  ASSERT(m_sprite != NULL);

  size_t nlayers = m_sprite->countLayers();
  if (m_layers.size() != nlayers) {
    if (nlayers > 0)
      m_layers.resize(nlayers, NULL);
    else
      m_layers.clear();
  }

  for (size_t c=0; c<nlayers; c++)
    m_layers[c] = m_sprite->indexToLayer(LayerIndex(c));

  updateScrollBars();
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
  Hit hit(
    PART_NOTHING,
    LayerIndex::NoLayer,
    frame_t(-1));

  if (!m_document)
    return hit;

  if (m_clk.part == PART_SEPARATOR) {
    hit.part = PART_SEPARATOR;
  }
  else {
    gfx::Point scroll = viewScroll();
    int top = topHeight();

    hit.layer = lastLayer() - LayerIndex(
      (mousePos.y
        - top
        - HDRSIZE
        + scroll.y) / LAYSIZE);

    hit.frame = frame_t((mousePos.x
        - m_separator_x
        - m_separator_w
        + scroll.x) / FRMSIZE);

    if (hasCapture()) {
      hit.layer = MID(firstLayer(), hit.layer, lastLayer());
      if (isMovingCel())
        hit.frame = MAX(firstFrame(), hit.frame);
      else
        hit.frame = MID(firstFrame(), hit.frame, lastFrame());
    }
    else {
      if (hit.layer > lastLayer()) hit.layer = LayerIndex::NoLayer;
      if (hit.frame > lastFrame()) hit.frame = frame_t(-1);
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
        gfx::Rect bounds = getPartBounds(Hit(PART_FRAME_TAG, LayerIndex(0), 0, frameTag->id()));
        if (bounds.contains(mousePos)) {
          hit.part = PART_FRAME_TAG;
          hit.frameTag = frameTag->id();
          break;
        }
      }
    }
    // Is the mouse on the headers?
    else if (mousePos.y >= top && mousePos.y < top+HDRSIZE) {
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
            !gfx::Rect(outline).shrink(2*OUTLINE_WIDTH).contains(mousePos)) {
          hit.part = PART_RANGE_OUTLINE;
        }
      }
    }
  }

  return hit;
}

Timeline::Hit Timeline::hitTestCel(const gfx::Point& mousePos)
{
  Hit hit(
    PART_NOTHING,
    LayerIndex::NoLayer,
    frame_t(-1));

  if (!m_document)
    return hit;

  gfx::Point scroll = viewScroll();
  int top = topHeight();

  hit.layer = lastLayer() - LayerIndex(
    (mousePos.y
     - top
     - HDRSIZE
     + scroll.y) / LAYSIZE);

  hit.frame = frame_t((mousePos.x
                       - m_separator_x
                       - m_separator_w
                       + scroll.x) / FRMSIZE);

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
            sb->setStatusText(0, "%s before frame %d", verb, int(m_dropRange.frameBegin()+1));
            return;
          }
          else if (m_dropTarget.hhit == DropTarget::After) {
            sb->setStatusText(0, "%s after frame %d", verb, int(m_dropRange.frameEnd()+1));
            return;
          }
        }
        break;

      case Range::kLayers: {
        int layerIdx = -1;
        if (m_dropTarget.vhit == DropTarget::Bottom)
          layerIdx = m_dropRange.layerBegin();
        else if (m_dropTarget.vhit == DropTarget::Top)
          layerIdx = m_dropRange.layerEnd();

        Layer* layer = ((layerIdx >= 0 && layerIdx < (int)m_layers.size()) ? m_layers[layerIdx]: NULL);
        if (layer) {
          if (m_dropTarget.vhit == DropTarget::Bottom) {
            sb->setStatusText(0, "%s at bottom of layer %s", verb, layer->name().c_str());
            return;
          }
          else if (m_dropTarget.vhit == DropTarget::Top) {
            sb->setStatusText(0, "%s at top of layer %s", verb, layer->name().c_str());
            return;
          }
        }
        break;
      }

    }
  }
  else {
    Layer* layer = (validLayer(m_hot.layer) ? m_layers[m_hot.layer]: NULL);

    switch (m_hot.part) {

      case PART_HEADER_ONIONSKIN: {
        sb->setStatusText(0, "Onionskin is %s",
          docPref().onionskin.active() ? "enabled": "disabled");
        return;
      }

      case PART_LAYER_TEXT:
        if (layer != NULL) {
          sb->setStatusText(0, "Layer '%s' [%s%s]",
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
        if (layer != NULL) {
          sb->setStatusText(0, "Layer '%s' is %s (%s)",
            layer->name().c_str(),
            layer->isContinuous() ? "continuous": "discontinuous",
            layer->isContinuous() ? "prefer linked cels/frames": "prefer individual cels/frames");
          return;
        }
        break;

      case PART_HEADER_FRAME:
        if (validFrame(m_hot.frame)) {
          sb->setStatusText(0,
            ":frame: %d :clock: %d",
            (int)m_hot.frame+1,
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

void Timeline::showCel(LayerIndex layer, frame_t frame)
{
  gfx::Point scroll = viewScroll();

  gfx::Rect viewport = m_viewportArea;

  // Add the horizontal bar space to the viewport area if the viewport
  // is not big enough to show one cel.
  if (m_hbar.isVisible() && viewport.h < LAYSIZE)
    viewport.h += m_vbar.getBarWidth();

  gfx::Rect celBounds(
    viewport.x + FRMSIZE*frame - scroll.x,
    viewport.y + LAYSIZE*(lastLayer() - layer) - scroll.y,
    FRMSIZE, LAYSIZE);

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
  LayerIndex layer = getLayerIndex(m_layer);
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
      m_sprite->totalFrames() * FRMSIZE + bounds().w/2,
      m_layers.size() * LAYSIZE + bounds().h/2);
  }
  else
    return gfx::Size(0, 0);
}

gfx::Point Timeline::getMaxScrollablePos() const
{
  if (m_sprite) {
    gfx::Size size = getScrollableSize();
    int max_scroll_x = size.w - bounds().w/2;
    int max_scroll_y = size.h - bounds().h/2;
    max_scroll_x = MAX(0, max_scroll_x);
    max_scroll_y = MAX(0, max_scroll_y);
    return gfx::Point(max_scroll_x, max_scroll_y);
  }
  else
    return gfx::Point(0, 0);
}

bool Timeline::allLayersVisible()
{
  for (size_t i=0; i<m_layers.size(); i++)
    if (!m_layers[i]->isVisible())
      return false;

  return true;
}

bool Timeline::allLayersInvisible()
{
  for (size_t i=0; i<m_layers.size(); i++)
    if (m_layers[i]->isVisible())
      return false;

  return true;
}

bool Timeline::allLayersLocked()
{
  for (size_t i=0; i<m_layers.size(); i++)
    if (m_layers[i]->isEditable())
      return false;

  return true;
}

bool Timeline::allLayersUnlocked()
{
  for (size_t i=0; i<m_layers.size(); i++)
    if (!m_layers[i]->isEditable())
      return false;

  return true;
}

bool Timeline::allLayersContinuous()
{
  for (size_t i=0; i<m_layers.size(); i++)
    if (!m_layers[i]->isContinuous())
      return false;

  return true;
}

bool Timeline::allLayersDiscontinuous()
{
  for (size_t i=0; i<m_layers.size(); i++)
    if (m_layers[i]->isContinuous())
      return false;

  return true;
}

LayerIndex Timeline::getLayerIndex(const Layer* layer) const
{
  for (int i=0; i<(int)m_layers.size(); i++)
    if (m_layers[i] == layer) {
      ASSERT(m_sprite->layerToIndex(layer) == LayerIndex(i));
      return LayerIndex(i);
    }

  return LayerIndex::NoLayer;
}

bool Timeline::isLayerActive(LayerIndex layerIndex) const
{
  if (layerIndex == getLayerIndex(m_layer))
    return true;
  else
    return m_range.inRange(layerIndex);
}

bool Timeline::isFrameActive(frame_t frame) const
{
  if (frame == m_frame)
    return true;
  else
    return m_range.inRange(frame);
}

void Timeline::dropRange(DropOp op)
{
  bool copy = (op == Timeline::kCopy);
  Range newFromRange;
  DocumentRangePlace place = kDocumentRangeAfter;

  switch (m_range.type()) {
    case Range::kFrames:
      if (m_dropTarget.hhit == DropTarget::Before)
        place = kDocumentRangeBefore;
      break;
    case Range::kLayers:
      if (m_dropTarget.vhit == DropTarget::Bottom)
        place = kDocumentRangeBefore;
      break;
  }

  prepareToMoveRange();

  try {
    if (copy)
      newFromRange = copy_range(m_document, m_range, m_dropRange, place);
    else
      newFromRange = move_range(m_document, m_range, m_dropRange, place);

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
    m_dropRange.disableRange();
    return;
  }

  switch (m_range.type()) {

    case Range::kCels: {
      frame_t dx = m_hot.frame - m_clk.frame;
      LayerIndex dy = m_hot.layer - m_clk.layer;

      LayerIndex layerIdx = dy+m_range.layerBegin();
      layerIdx = MID(firstLayer(), layerIdx, LayerIndex(m_layers.size() - m_range.layers()));

      frame_t frame = dx+m_range.frameBegin();
      frame = MAX(firstFrame(), frame);

      m_dropRange.startRange(layerIdx, frame, m_range.type());
      m_dropRange.endRange(
        layerIdx+LayerIndex(m_range.layers()-1),
        frame+m_range.frames()-1);
      break;
    }

    case Range::kFrames: {
      frame_t frame = m_hot.frame;
      frame_t frameEnd = frame;

      if (frame >= m_range.frameBegin() && frame <= m_range.frameEnd()) {
        frame = m_range.frameBegin();
        frameEnd = frame + m_range.frames() - 1;
      }

      LayerIndex layerIdx = getLayerIndex(m_layer);
      m_dropRange.startRange(layerIdx, frame, m_range.type());
      m_dropRange.endRange(layerIdx, frameEnd);
      break;
    }

    case Range::kLayers: {
      LayerIndex layer = m_hot.layer;
      LayerIndex layerEnd = layer;

      if (layer >= m_range.layerBegin() && layer <= m_range.layerEnd()) {
        layer = m_range.layerBegin();
        layerEnd = layer + LayerIndex(m_range.layers() - 1);
      }

      m_dropRange.startRange(layer, m_frame, m_range.type());
      m_dropRange.endRange(layerEnd, m_frame);
      break;
    }
  }

  gfx::Rect bounds = getRangeBounds(m_dropRange);

  if (pt.x < bounds.x + bounds.w/2)
    m_dropTarget.hhit = DropTarget::Before;
  else
    m_dropTarget.hhit = DropTarget::After;

  if (pt.y < bounds.y + bounds.h/2)
    m_dropTarget.vhit = DropTarget::Top;
  else
    m_dropTarget.vhit = DropTarget::Bottom;

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

int Timeline::topHeight() const
{
  int h = 0;
  if (m_document && m_sprite) {
    h += skinTheme()->dimensions.timelineTopBorder();
    h += font()->height();
    h += skinTheme()->dimensions.timelineTagsAreaHeight();
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
    m_range.disableRange();
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
  m_range.disableRange();
  clearClipboardRange();
  invalidate();
}

} // namespace app
