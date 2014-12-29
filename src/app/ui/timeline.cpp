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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/timeline.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/document_range_ops.h"
#include "app/document_undo.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/ui/configure_timeline_popup.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/undo_transaction.h"
#include "app/util/clipboard.h"
#include "base/memory.h"
#include "doc/document_event.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "doc/doc.h"
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

#define OUTLINE_WIDTH   (2*guiscale()) // TODO theme specific

// Space between the icon-bitmap and the edge of the surrounding button.
#define ICONBORDER      0

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace doc;
using namespace ui;

static const char* kTimeline = "timeline";
static const char* kTimelineBox = "timeline_box";
static const char* kTimelineOpenEye = "timeline_open_eye";
static const char* kTimelineClosedEye = "timeline_closed_eye";
static const char* kTimelineOpenPadlock = "timeline_open_padlock";
static const char* kTimelineClosedPadlock = "timeline_closed_padlock";
static const char* kTimelineLayer = "timeline_layer";
static const char* kTimelineEmptyFrame = "timeline_empty_frame";
static const char* kTimelineKeyframe = "timeline_keyframe";
static const char* kTimelineFromLeft = "timeline_fromleft";
static const char* kTimelineFromRight = "timeline_fromright";
static const char* kTimelineFromBoth = "timeline_fromboth";
static const char* kTimelineGear = "timeline_gear";
static const char* kTimelineOnionskin = "timeline_onionskin";
static const char* kTimelineOnionskinRange = "timeline_onionskin_range";
static const char* kTimelinePadding = "timeline_padding";
static const char* kTimelinePaddingTr = "timeline_padding_tr";
static const char* kTimelinePaddingBl = "timeline_padding_bl";
static const char* kTimelinePaddingBr = "timeline_padding_br";
static const char* kTimelineSelectedCelStyle = "timeline_selected_cel";
static const char* kTimelineRangeOutlineStyle = "timeline_range_outline";
static const char* kTimelineDropLayerDecoStyle = "timeline_drop_layer_deco";
static const char* kTimelineDropFrameDecoStyle = "timeline_drop_frame_deco";
static const char* kTimelineLoopRangeStyle = "timeline_loop_range";

static const char* kTimelineActiveColor = "timeline_active";

enum {
  A_PART_NOTHING,
  A_PART_SEPARATOR,
  A_PART_HEADER_EYE,
  A_PART_HEADER_PADLOCK,
  A_PART_HEADER_GEAR,
  A_PART_HEADER_ONIONSKIN,
  A_PART_HEADER_ONIONSKIN_RANGE_LEFT,
  A_PART_HEADER_ONIONSKIN_RANGE_RIGHT,
  A_PART_HEADER_LAYER,
  A_PART_HEADER_FRAME,
  A_PART_LAYER,
  A_PART_LAYER_EYE_ICON,
  A_PART_LAYER_PADLOCK_ICON,
  A_PART_LAYER_TEXT,
  A_PART_CEL,
  A_PART_RANGE_OUTLINE
};

Timeline::Timeline()
  : Widget(kGenericWidget)
  , m_timelineStyle(get_style(kTimeline))
  , m_timelineBoxStyle(get_style(kTimelineBox))
  , m_timelineOpenEyeStyle(get_style(kTimelineOpenEye))
  , m_timelineClosedEyeStyle(get_style(kTimelineClosedEye))
  , m_timelineOpenPadlockStyle(get_style(kTimelineOpenPadlock))
  , m_timelineClosedPadlockStyle(get_style(kTimelineClosedPadlock))
  , m_timelineLayerStyle(get_style(kTimelineLayer))
  , m_timelineEmptyFrameStyle(get_style(kTimelineEmptyFrame))
  , m_timelineKeyframeStyle(get_style(kTimelineKeyframe))
  , m_timelineFromLeftStyle(get_style(kTimelineFromLeft))
  , m_timelineFromRightStyle(get_style(kTimelineFromRight))
  , m_timelineFromBothStyle(get_style(kTimelineFromBoth))
  , m_timelineGearStyle(get_style(kTimelineGear))
  , m_timelineOnionskinStyle(get_style(kTimelineOnionskin))
  , m_timelineOnionskinRangeStyle(get_style(kTimelineOnionskinRange))
  , m_timelinePaddingStyle(get_style(kTimelinePadding))
  , m_timelinePaddingTrStyle(get_style(kTimelinePaddingTr))
  , m_timelinePaddingBlStyle(get_style(kTimelinePaddingBl))
  , m_timelinePaddingBrStyle(get_style(kTimelinePaddingBr))
  , m_timelineSelectedCelStyle(get_style(kTimelineSelectedCelStyle))
  , m_timelineRangeOutlineStyle(get_style(kTimelineRangeOutlineStyle))
  , m_timelineDropLayerDecoStyle(get_style(kTimelineDropLayerDecoStyle))
  , m_timelineDropFrameDecoStyle(get_style(kTimelineDropFrameDecoStyle))
  , m_timelineLoopRangeStyle(get_style(kTimelineLoopRangeStyle))
  , m_context(UIContext::instance())
  , m_editor(NULL)
  , m_document(NULL)
  , m_scroll_x(0)
  , m_scroll_y(0)
  , m_separator_x(100 * guiscale())
  , m_separator_w(1)
  , m_confPopup(NULL)
  , m_clipboard_timer(100, this)
  , m_offset_count(0)
  , m_scroll(false)
{
  m_ctxConn = m_context->AfterCommandExecution.connect(&Timeline::onAfterCommandExecution, this);
  m_context->documents().addObserver(this);

  setDoubleBuffered(true);
}

Timeline::~Timeline()
{
  m_clipboard_timer.stop();

  detachDocument();
  m_context->documents().removeObserver(this);
  delete m_confPopup;
}

void Timeline::updateUsingEditor(Editor* editor)
{
  // As a sprite editor was selected, it looks like the user wants to
  // execute commands targetting the editor instead of the
  // timeline. Here we disable the selected range, so commands like
  // Clear, Copy, Cut, etc. don't target the Timeline and they are
  // sent to the active sprite editor.
  m_range.disableRange();
  invalidate();

  detachDocument();

  // We always update the editor. In this way the timeline keeps in
  // sync with the active editor.
  m_editor = editor;

  if (m_editor)
    m_editor->addObserver(this);
  else
    return;                // No editor specified.

  DocumentLocation location;
  DocumentView* view = m_editor->getDocumentView();
  view->getDocumentLocation(&location);

  location.document()->addObserver(this);

  // If we are already in the same position as the "editor", we don't
  // need to update the at all timeline.
  if (m_document == location.document() &&
      m_sprite == location.sprite() &&
      m_layer == location.layer() &&
      m_frame == location.frame())
    return;

  m_document = location.document();
  m_sprite = location.sprite();
  m_layer = location.layer();
  m_frame = location.frame();
  m_state = STATE_STANDBY;
  m_hot_part = A_PART_NOTHING;
  m_clk_part = A_PART_NOTHING;

  setFocusStop(true);
  regenerateLayers();
}

void Timeline::detachDocument()
{
  if (m_document) {
    m_document->removeObserver(this);
    m_document = NULL;
  }

  if (m_editor) {
    m_editor->removeObserver(this);
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

void Timeline::setFrame(frame_t frame)
{
  ASSERT(m_editor != NULL);
  // ASSERT(frame >= 0 && frame < m_sprite->totalFrames());

  if (frame < 0)
    frame = firstFrame();
  else if (frame >= m_sprite->totalFrames())
    frame = frame_t(m_sprite->totalFrames()-1);

  m_frame = frame;
  invalidate();

  if (m_editor->frame() != frame)
    m_editor->setFrame(m_frame);
}

void Timeline::activateClipboardRange()
{
  m_clipboard_timer.start();
  invalidate();
}

bool Timeline::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

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

      // Clicked-part = hot-part.
      m_clk_part = m_hot_part;
      m_clk_layer = m_hot_layer;
      m_clk_frame = m_hot_frame;

      captureMouse();

      switch (m_hot_part) {
        case A_PART_SEPARATOR:
          m_state = STATE_MOVING_SEPARATOR;
          break;
        case A_PART_HEADER_ONIONSKIN_RANGE_LEFT: {
          ISettings* settings = UIContext::instance()->settings();
          IDocumentSettings* docSettings = settings->getDocumentSettings(m_document);
          m_state = STATE_MOVING_ONIONSKIN_RANGE_LEFT;
          m_origFrames = docSettings->getOnionskinPrevFrames();
          break;
        }
        case A_PART_HEADER_ONIONSKIN_RANGE_RIGHT: {
          ISettings* settings = UIContext::instance()->settings();
          IDocumentSettings* docSettings = settings->getDocumentSettings(m_document);
          m_state = STATE_MOVING_ONIONSKIN_RANGE_RIGHT;
          m_origFrames = docSettings->getOnionskinNextFrames();
          break;
        }
        case A_PART_HEADER_FRAME: {
          bool selectFrame = (mouseMsg->left() || !isFrameActive(m_clk_frame));

          if (selectFrame) {
            setFrame(m_clk_frame);

            m_state = STATE_SELECTING_FRAMES;
            m_range.startRange(getLayerIndex(m_layer), m_clk_frame, Range::kFrames);
          }
          break;
        }
        case A_PART_LAYER_TEXT: {
          LayerIndex old_layer = getLayerIndex(m_layer);
          bool selectLayer = (mouseMsg->left() || !isLayerActive(m_clk_layer));

          if (selectLayer) {
            // Did the user select another layer?
            if (old_layer != m_clk_layer) {
              setLayer(m_layers[m_clk_layer]);
              invalidate();
            }
          }

          // Change the scroll to show the new selected layer/cel.
          showCel(m_clk_layer, m_frame);

          if (selectLayer) {
            m_state = STATE_SELECTING_LAYERS;
            m_range.startRange(m_clk_layer, m_frame, Range::kLayers);
          }
          break;
        }
        case A_PART_LAYER_EYE_ICON:
          break;
        case A_PART_LAYER_PADLOCK_ICON:
          break;
        case A_PART_CEL: {
          LayerIndex old_layer = getLayerIndex(m_layer);
          bool selectCel = (mouseMsg->left()
            || !isLayerActive(m_clk_layer)
            || !isFrameActive(m_clk_frame));
          frame_t old_frame = m_frame;

          // Select the new clicked-part.
          if (old_layer != m_clk_layer
            || old_frame != m_clk_frame) {
            setLayer(m_layers[m_clk_layer]);
            setFrame(m_clk_frame);
            invalidate();
          }

          // Change the scroll to show the new selected cel.
          showCel(m_clk_layer, m_frame);

          if (selectCel) {
            m_state = STATE_SELECTING_CELS;
            m_range.startRange(m_clk_layer, m_clk_frame, Range::kCels);
          }
          invalidate();
          break;
        }
        case A_PART_RANGE_OUTLINE:
          m_state = STATE_MOVING_RANGE;
          break;
      }

      // Redraw the new clicked part (header, layer or cel).
      invalidatePart(m_clk_part, m_clk_layer, m_clk_frame);
      break;
    }

    case kMouseMoveMessage: {
      if (!m_document)
        break;

      int hot_part;
      LayerIndex hot_layer;
      frame_t hot_frame;
      gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position()
        - getBounds().getOrigin();

      updateHot(msg, mousePos, hot_part, hot_layer, hot_frame);

      if (hasCapture()) {
        switch (m_state) {

          case STATE_SCROLLING: {
            gfx::Point absMousePos = static_cast<MouseMessage*>(msg)->position();
            setScroll(
              m_scroll_x - (absMousePos.x - m_oldPos.x),
              m_scroll_y - (absMousePos.y - m_oldPos.y));

            m_oldPos = absMousePos;
            return true;
          }

          case STATE_MOVING_ONIONSKIN_RANGE_LEFT: {
            ISettings* settings = UIContext::instance()->settings();
            IDocumentSettings* docSettings = settings->getDocumentSettings(m_document);
            int newValue = m_origFrames + (m_clk_frame - hot_frame);
            docSettings->setOnionskinPrevFrames(MAX(0, newValue));
            invalidate();
            return true;
          }

          case STATE_MOVING_ONIONSKIN_RANGE_RIGHT:
            ISettings* settings = UIContext::instance()->settings();
            IDocumentSettings* docSettings = settings->getDocumentSettings(m_document);
            int newValue = m_origFrames - (m_clk_frame - hot_frame);
            docSettings->setOnionskinNextFrames(MAX(0, newValue));
            invalidate();
            return true;
        }

        // If the mouse pressed the mouse's button in the separator,
        // we shouldn't change the hot (so the separator can be
        // tracked to the mouse's released).
        if (m_clk_part == A_PART_SEPARATOR) {
          m_separator_x = MAX(0, mousePos.x);
          invalidate();
          return true;
        }
      }

      updateDropRange(mousePos);

      if (hasCapture()) {
        switch (m_state) {

          case STATE_SELECTING_LAYERS: {
            if (m_layer != m_layers[hot_layer]) {
              m_range.endRange(hot_layer, m_frame);
              setLayer(m_layers[m_clk_layer = hot_layer]);
            }
            break;
          }

          case STATE_SELECTING_FRAMES: {
            m_range.endRange(getLayerIndex(m_layer), hot_frame);
            setFrame(m_clk_frame = hot_frame);
            break;
          }

          case STATE_SELECTING_CELS:
            if ((m_layer != m_layers[hot_layer])
              || (m_frame != hot_frame)) {
              m_range.endRange(hot_layer, hot_frame);
              setLayer(m_layers[m_clk_layer = hot_layer]);
              setFrame(m_clk_frame = hot_frame);
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

        switch (m_hot_part) {

          case A_PART_NOTHING:
          case A_PART_SEPARATOR:
          case A_PART_HEADER_LAYER:
            // Do nothing.
            break;

          case A_PART_HEADER_EYE: {
            bool newVisibleState = !allLayersVisible();
            for (size_t i=0; i<m_layers.size(); i++)
              m_layers[i]->setVisible(newVisibleState);

            // Redraw all views.
            m_document->notifyGeneralUpdate();
            break;
          }

          case A_PART_HEADER_PADLOCK: {
            bool newEditableState = !allLayersUnlocked();
            for (size_t i=0; i<m_layers.size(); i++)
              m_layers[i]->setEditable(newEditableState);
            break;
          }

          case A_PART_HEADER_GEAR: {
            gfx::Rect gearBounds =
              getPartBounds(A_PART_HEADER_GEAR).offset(getBounds().getOrigin());

            if (!m_confPopup) {
              ConfigureTimelinePopup* popup =
                new ConfigureTimelinePopup();

              popup->remapWindow();
              m_confPopup = popup;
            }

            if (!m_confPopup->isVisible()) {
              m_confPopup->moveWindow(gfx::Rect(
                  gearBounds.x,
                  gearBounds.y-m_confPopup->getBounds().h,
                  m_confPopup->getBounds().w,
                  m_confPopup->getBounds().h));
              m_confPopup->openWindow();
            }
            else
              m_confPopup->closeWindow(NULL);
            break;
          }

          case A_PART_HEADER_ONIONSKIN: {
            ISettings* settings = UIContext::instance()->settings();
            IDocumentSettings* docSettings = settings->getDocumentSettings(m_document);
            if (docSettings)
              docSettings->setUseOnionskin(!docSettings->getUseOnionskin());
            break;
          }

          case A_PART_HEADER_FRAME:
            // Show the frame pop-up menu.
            if (mouseMsg->right()) {
              if (m_clk_frame == m_hot_frame) {
                Menu* popup_menu = AppMenus::instance()->getFramePopupMenu();
                if (popup_menu)
                  popup_menu->showPopup(mouseMsg->position());
              }
            }
            break;

          case A_PART_LAYER_TEXT:
            // Show the layer pop-up menu.
            if (mouseMsg->right()) {
              if (m_clk_layer == m_hot_layer) {
                Menu* popup_menu = AppMenus::instance()->getLayerPopupMenu();
                if (popup_menu != NULL)
                  popup_menu->showPopup(mouseMsg->position());
              }
            }
            break;

          case A_PART_LAYER_EYE_ICON:
            // Hide/show layer.
            if (m_hot_layer == m_clk_layer && validLayer(m_hot_layer)) {
              Layer* layer = m_layers[m_clk_layer];
              ASSERT(layer != NULL);
              layer->setVisible(!layer->isVisible());

              // Redraw all views.
              m_document->notifyGeneralUpdate();
            }
            break;

          case A_PART_LAYER_PADLOCK_ICON:
            // Lock/unlock layer.
            if (m_hot_layer == m_clk_layer && validLayer(m_hot_layer)) {
              Layer* layer = m_layers[m_clk_layer];
              ASSERT(layer != NULL);
              layer->setEditable(!layer->isEditable());
            }
            break;

          case A_PART_CEL: {
            // Show the cel pop-up menu.
            if (mouseMsg->right()) {
              Menu* popup_menu =
                (m_state == STATE_MOVING_RANGE &&
                 m_range.type() == Range::kCels) ?
                  AppMenus::instance()->getCelMovementPopupMenu():
                  AppMenus::instance()->getCelPopupMenu();

              if (popup_menu)
                popup_menu->showPopup(mouseMsg->position());
            }
            break;
          }

        }

        if (mouseMsg->left() &&
            m_state == STATE_MOVING_RANGE &&
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
          invalidatePart(m_hot_part, m_hot_layer, m_hot_frame);

        // Restore the cursor.
        m_state = STATE_STANDBY;
        setCursor(msg, mouseMsg->position());

        releaseMouse();
        updateStatusBar(msg);
        return true;
      }
      break;

    case kDoubleClickMessage:
      switch (m_hot_part) {

        case A_PART_LAYER_TEXT: {
          Command* command = CommandsModule::instance()
            ->getCommandByName(CommandId::LayerProperties);

          UIContext::instance()->executeCommand(command);
          return true;
        }

        case A_PART_HEADER_FRAME: {
          Command* command = CommandsModule::instance()
            ->getCommandByName(CommandId::FrameProperties);
          Params params;
          params.set("frame", "current");

          UIContext::instance()->executeCommand(command, &params);
          return true;
        }

        case A_PART_CEL: {
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
          used = true;
          break;

        case kKeySpace: {
          m_scroll = true;
          used = true;
          break;
        }
      }

      updateHotByMousePos(msg,
        ui::get_mouse_position() - getBounds().getOrigin());

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

      updateHotByMousePos(msg,
        ui::get_mouse_position() - getBounds().getOrigin());

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

        setScroll(m_scroll_x+dx,
                  m_scroll_y+dy);
      }
      break;

    case kSetCursorMessage:
      if (m_document) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        setCursor(msg, mousePos);
        return true;
      }
      break;
  }

  return Widget::onProcessMessage(msg);
}

void Timeline::onPreferredSize(PreferredSizeEvent& ev)
{
  // This doesn't matter, the AniEditor'll use the entire screen anyway.
  ev.setPreferredSize(Size(32, 32));
}

void Timeline::onPaint(ui::PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  bool noDoc = (m_document == NULL);
  if (noDoc)
    goto paintNoDoc;

  try {
    // Lock the sprite to read/render it.
    const DocumentReader documentReader(m_document);
    
    LayerIndex layer, first_layer, last_layer;
    frame_t frame, first_frame, last_frame;

    getDrawableLayers(g, &first_layer, &last_layer);
    getDrawableFrames(g, &first_frame, &last_frame);

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
            NULL, m_timelineOnionskinRangeStyle,
            false, false, false);
        }
      }
    }

    // Draw each visible layer.
    for (layer=last_layer; layer>=first_layer; --layer) {
      {
        IntersectClip clip(g, getLayerHeadersBounds());
        if (clip)
          drawLayer(g, layer);
      }

      // Get the first CelIterator to be drawn (it is the first cel with cel->frame >= first_frame)
      CelIterator it, end;
      Layer* layerPtr = m_layers[layer];
      if (layerPtr->isImage()) {
        it = static_cast<LayerImage*>(layerPtr)->getCelBegin();
        end = static_cast<LayerImage*>(layerPtr)->getCelEnd();
        for (; it != end && (*it)->frame() < first_frame; ++it)
          ;
      }

      IntersectClip clip(g, getCelsBounds());
      if (!clip)
        continue;

      // Draw every visible cel for each layer.
      for (frame=first_frame; frame<=last_frame; ++frame) {
        Cel* cel = (layerPtr->isImage() && it != end && (*it)->frame() == frame ? *it: NULL);
        drawCel(g, layer, frame, cel);

        if (cel)
          ++it;               // Go to next cel
      }
    }

    drawPaddings(g);
    drawLoopRange(g);
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
  }

paintNoDoc:;
  if (noDoc)
    drawPart(g, getClientBounds(), NULL, m_timelinePaddingStyle);
}

void Timeline::onAfterCommandExecution(Command* command)
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
  setFrame(ev.frame());

  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

void Timeline::onRemoveFrame(doc::DocumentEvent& ev)
{
  // Adjust current frame of all editors that are in a frame more
  // advanced that the removed one.
  if (getFrame() > ev.frame()) {
    setFrame(getFrame()-1);
  }
  // If the editor was in the previous "last frame" (current value of
  // totalFrames()), we've to adjust it to the new last frame
  // (lastFrame())
  else if (getFrame() >= sprite()->totalFrames()) {
    setFrame(sprite()->lastFrame());
  }

  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

void Timeline::onSelectionChanged(doc::DocumentEvent& ev)
{
  m_range.disableRange();

  clearClipboardRange();
  invalidate();
}

void Timeline::onAfterFrameChanged(Editor* editor)
{
  setFrame(editor->frame());

  if (!hasCapture())
    m_range.disableRange();

  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

void Timeline::onAfterLayerChanged(Editor* editor)
{
  setLayer(editor->layer());

  if (!hasCapture())
    m_range.disableRange();

  showCurrentCel();
  clearClipboardRange();
  invalidate();
}

void Timeline::setCursor(ui::Message* msg, const gfx::Point& mousePos)
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
  else if (m_hot_part == A_PART_HEADER_ONIONSKIN_RANGE_LEFT
    || m_state == STATE_MOVING_ONIONSKIN_RANGE_LEFT) {
    ui::set_mouse_cursor(kSizeWCursor);
  }
  else if (m_hot_part == A_PART_HEADER_ONIONSKIN_RANGE_RIGHT
    || m_state == STATE_MOVING_ONIONSKIN_RANGE_RIGHT) {
    ui::set_mouse_cursor(kSizeECursor);
  }
  else if (m_hot_part == A_PART_RANGE_OUTLINE) {
    ui::set_mouse_cursor(kMoveCursor);
  }
  else if (m_hot_part == A_PART_SEPARATOR) {
    ui::set_mouse_cursor(kSizeWECursor);
  }
  else {
    ui::set_mouse_cursor(kArrowCursor);
  }
}

void Timeline::getDrawableLayers(ui::Graphics* g, LayerIndex* first_layer, LayerIndex* last_layer)
{
  int hpx = (getClientBounds().h - HDRSIZE);
  LayerIndex i = lastLayer() - LayerIndex((m_scroll_y+hpx) / LAYSIZE);
  i = MID(firstLayer(), i, lastLayer());

  LayerIndex j = i + LayerIndex(hpx / LAYSIZE);
  if (!m_layers.empty())
    j = MID(firstLayer(), j, lastLayer());
  else
    j = LayerIndex::NoLayer;

  *first_layer = i;
  *last_layer = j;
}

void Timeline::getDrawableFrames(ui::Graphics* g, frame_t* first_frame, frame_t* last_frame)
{
  *first_frame = frame_t((m_separator_w + m_scroll_x) / FRMSIZE);
  *last_frame = *first_frame
    + frame_t((getClientBounds().w - m_separator_w) / FRMSIZE);
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
  g->drawRect(0, getRangeBounds(clipboard_range));
}

void Timeline::drawHeader(ui::Graphics* g)
{
  ISettings* settings = UIContext::instance()->settings();
  IDocumentSettings* docSettings = settings->getDocumentSettings(m_document);
  bool allInvisible = allLayersInvisible();
  bool allLocked = allLayersLocked();

  drawPart(g, getPartBounds(A_PART_HEADER_EYE),
    NULL,
    allInvisible ? m_timelineClosedEyeStyle: m_timelineOpenEyeStyle,
    m_clk_part == A_PART_HEADER_EYE,
    m_hot_part == A_PART_HEADER_EYE,
    m_clk_part == A_PART_HEADER_EYE);

  drawPart(g, getPartBounds(A_PART_HEADER_PADLOCK),
    NULL,
    allLocked ? m_timelineClosedPadlockStyle: m_timelineOpenPadlockStyle,
    m_clk_part == A_PART_HEADER_PADLOCK,
    m_hot_part == A_PART_HEADER_PADLOCK,
    m_clk_part == A_PART_HEADER_PADLOCK);

  drawPart(g, getPartBounds(A_PART_HEADER_GEAR),
    NULL, m_timelineGearStyle,
    false,
    m_hot_part == A_PART_HEADER_GEAR,
    m_clk_part == A_PART_HEADER_GEAR);

  drawPart(g, getPartBounds(A_PART_HEADER_ONIONSKIN),
    NULL, m_timelineOnionskinStyle,
    docSettings->getUseOnionskin(),
    m_hot_part == A_PART_HEADER_ONIONSKIN,
    m_clk_part == A_PART_HEADER_ONIONSKIN);

  // Empty header space.
  drawPart(g, getPartBounds(A_PART_HEADER_LAYER),
    NULL, m_timelineBoxStyle, false, false, false);
}

void Timeline::drawHeaderFrame(ui::Graphics* g, frame_t frame)
{
  bool is_active = isFrameActive(frame);
  bool is_hover = (m_hot_part == A_PART_HEADER_FRAME && m_hot_frame == frame);
  bool is_clicked = (m_clk_part == A_PART_HEADER_FRAME && m_clk_frame == frame);
  gfx::Rect bounds = getPartBounds(A_PART_HEADER_FRAME, firstLayer(), frame);
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  // Draw the header for the layers.
  char buf[256];
  std::sprintf(buf, "%d", (frame+1)%100); // Draw only the first two digits.

  she::Font* oldFont = g->getFont();
  g->setFont(((SkinTheme*)getTheme())->getMiniFont());
  drawPart(g, bounds, buf, m_timelineBoxStyle, is_active, is_hover, is_clicked);
  g->setFont(oldFont);
}

void Timeline::drawLayer(ui::Graphics* g, LayerIndex layerIdx)
{
  Layer* layer = m_layers[layerIdx];
  bool is_active = isLayerActive(layerIdx);
  bool hotlayer = (m_hot_layer == layerIdx);
  bool clklayer = (m_clk_layer == layerIdx);
  gfx::Rect bounds = getPartBounds(A_PART_LAYER, layerIdx, firstFrame());
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  // Draw the eye (visible flag).
  bounds = getPartBounds(A_PART_LAYER_EYE_ICON, layerIdx);
  drawPart(g, bounds, NULL,
    layer->isVisible() ? m_timelineOpenEyeStyle: m_timelineClosedEyeStyle,
    is_active,
    (hotlayer && m_hot_part == A_PART_LAYER_EYE_ICON),
    (clklayer && m_clk_part == A_PART_LAYER_EYE_ICON));

  // Draw the padlock (editable flag).
  bounds = getPartBounds(A_PART_LAYER_PADLOCK_ICON, layerIdx);
  drawPart(g, bounds, NULL,
    layer->isEditable() ? m_timelineOpenPadlockStyle: m_timelineClosedPadlockStyle,
    is_active,
    (hotlayer && m_hot_part == A_PART_LAYER_PADLOCK_ICON),
    (clklayer && m_clk_part == A_PART_LAYER_PADLOCK_ICON));

  // Draw the layer's name.
  bounds = getPartBounds(A_PART_LAYER_TEXT, layerIdx);
  drawPart(g, bounds, layer->name().c_str(), m_timelineLayerStyle,
    is_active,
    (hotlayer && m_hot_part == A_PART_LAYER_TEXT),
    (clklayer && m_clk_part == A_PART_LAYER_TEXT));

  // If this layer wasn't clicked but there are another layer clicked,
  // we have to draw some indicators to show that the user can move
  // layers.
  if (hotlayer && !is_active && m_clk_part == A_PART_LAYER_TEXT) {
    // TODO this should be skinneable
    g->fillRect(get_color_by_id(kTimelineActiveColor),
                gfx::Rect(bounds.x, bounds.y, bounds.w, 2));
  }
}

void Timeline::drawCel(ui::Graphics* g, LayerIndex layerIndex, frame_t frame, Cel* cel)
{
  Image* image = (cel ? cel->image(): NULL);
  bool is_hover = (m_hot_part == A_PART_CEL &&
    m_hot_layer == layerIndex &&
    m_hot_frame == frame);
  bool is_active = (isLayerActive(layerIndex) || isFrameActive(frame));
  bool is_empty = (image == NULL);
  gfx::Rect bounds = getPartBounds(A_PART_CEL, layerIndex, frame);
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  if (layerIndex == getLayerIndex(m_layer) && frame == m_frame)
    drawPart(g, bounds, NULL, m_timelineSelectedCelStyle, false, false, true);
  else
    drawPart(g, bounds, NULL, m_timelineBoxStyle, is_active, is_hover);

  skin::Style* style;
  if (is_empty) {
    style = m_timelineEmptyFrameStyle;
  }
  else {
#if 0 // TODO We must find a fast-way to compare keyframes. Cels could
      // share images until the user draw over them. Also, we could
      // calculate a hash for each image (and recalculate it when the
      // user draw over it), so then we can compare hashes.  Other
      // option is to use a thread to calculate differences, but I
      // think it's too much for just UI stuff.
    Layer* layer = m_layers[layerIndex];
    Cel* left = (layer->isImage() ? static_cast<LayerImage*>(layer)->getCel(frame-1): NULL);
    Cel* right = (layer->isImage() ? static_cast<LayerImage*>(layer)->getCel(frame+1): NULL);
    Image* leftImg = (left ? m_sprite->stock()->getImage(left->getImage()): NULL);
    Image* rightImg = (right ? m_sprite->stock()->getImage(right->getImage()): NULL);
    bool fromLeft = (leftImg && count_diff_between_images(image, leftImg) == 0);
    bool fromRight = (rightImg && count_diff_between_images(image, rightImg) == 0);

    if (fromLeft && fromRight)
      style = m_timelineFromBothStyle;
    else if (fromLeft)
      style = m_timelineFromLeftStyle;
    else if (fromRight)
      style = m_timelineFromRightStyle;
    else
#endif
      style = m_timelineKeyframeStyle;
  }
  drawPart(g, bounds, NULL, style, is_active, is_hover);
}

void Timeline::drawLoopRange(ui::Graphics* g)
{
  ISettings* settings = UIContext::instance()->settings();
  IDocumentSettings* docSettings = settings->getDocumentSettings(m_document);
  if (!docSettings->getLoopAnimation())
    return;

  frame_t begin, end;
  docSettings->getLoopRange(&begin, &end);
  if (begin > end)
    return;

  gfx::Rect bounds1 = getPartBounds(A_PART_HEADER_FRAME, firstLayer(), begin);
  gfx::Rect bounds2 = getPartBounds(A_PART_HEADER_FRAME, firstLayer(), end);
  gfx::Rect bounds = bounds1.createUnion(bounds2);

  IntersectClip clip(g, bounds);
  if (!clip)
    return;
  
  drawPart(g, bounds, NULL, m_timelineLoopRangeStyle);
}

void Timeline::drawRangeOutline(ui::Graphics* g)
{
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
  if (m_hot_part == A_PART_RANGE_OUTLINE) state += Style::hover();

  gfx::Rect bounds = getPartBounds(A_PART_RANGE_OUTLINE);
  m_timelineRangeOutlineStyle->paint(g, bounds, NULL, state);

  Range drop = m_dropRange;
  gfx::Rect dropBounds = getRangeBounds(drop);

  switch (drop.type()) {

    case Range::kCels: {
      dropBounds = dropBounds.enlarge(OUTLINE_WIDTH);
      m_timelineRangeOutlineStyle->paint(g, dropBounds, NULL, Style::active());
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

      m_timelineDropFrameDecoStyle->paint(g, dropBounds, NULL, Style::State());
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

      m_timelineDropLayerDecoStyle->paint(g, dropBounds, NULL, Style::State());
      break;
    }
  }
}

void Timeline::drawPaddings(ui::Graphics* g)
{
  gfx::Rect client = getClientBounds();
  gfx::Rect bottomLayer;
  gfx::Rect lastFrame;

  if (!m_layers.empty()) {
    bottomLayer = getPartBounds(A_PART_LAYER, firstLayer());
    lastFrame = getPartBounds(A_PART_CEL, firstLayer(), this->lastFrame());
  }
  else {
    bottomLayer = getPartBounds(A_PART_HEADER_LAYER);
    lastFrame = getPartBounds(A_PART_HEADER_FRAME, firstLayer(), this->lastFrame());
  }

  drawPart(g,
    gfx::Rect(lastFrame.x+lastFrame.w, client.y,
      client.w - (lastFrame.x+lastFrame.w),
      bottomLayer.y+bottomLayer.h),
    NULL, m_timelinePaddingTrStyle);

  drawPart(g,
    gfx::Rect(client.x, bottomLayer.y+bottomLayer.h,
      lastFrame.x+lastFrame.w - client.x, client.h - (bottomLayer.y+bottomLayer.h)),
    NULL, m_timelinePaddingBlStyle);

  drawPart(g,
    gfx::Rect(lastFrame.x+lastFrame.w, bottomLayer.y+bottomLayer.h,
      client.w - (lastFrame.x+lastFrame.w),
      client.h - (bottomLayer.y+bottomLayer.h)),
    NULL, m_timelinePaddingBrStyle);
}

gfx::Rect Timeline::getLayerHeadersBounds() const
{
  gfx::Rect rc = getClientBounds();
  rc.w = m_separator_x;
  rc.y += HDRSIZE;
  rc.h -= HDRSIZE;
  return rc;
}

gfx::Rect Timeline::getFrameHeadersBounds() const
{
  gfx::Rect rc = getClientBounds();
  rc.x += m_separator_x;
  rc.w -= m_separator_x;
  rc.h = HDRSIZE;
  return rc;
}

gfx::Rect Timeline::getOnionskinFramesBounds() const
{
  ISettings* settings = UIContext::instance()->settings();
  IDocumentSettings* docSettings = settings->getDocumentSettings(m_document);
  if (docSettings->getUseOnionskin()) {
    frame_t firstFrame = m_frame - docSettings->getOnionskinPrevFrames();
    frame_t lastFrame = m_frame + docSettings->getOnionskinNextFrames();

    if (firstFrame < this->firstFrame())
      firstFrame = this->firstFrame();

    if (lastFrame > this->lastFrame())
      lastFrame = this->lastFrame();

    return getPartBounds(A_PART_HEADER_FRAME, firstLayer(), firstFrame)
      .createUnion(getPartBounds(A_PART_HEADER_FRAME, firstLayer(), lastFrame));
  }
  return gfx::Rect();
}

gfx::Rect Timeline::getCelsBounds() const
{
  gfx::Rect rc = getClientBounds();
  rc.x += m_separator_x;
  rc.w -= m_separator_x;
  rc.y += HDRSIZE;
  rc.h -= HDRSIZE;
  return rc;
}

gfx::Rect Timeline::getPartBounds(int part, LayerIndex layer, frame_t frame) const
{
  const gfx::Rect bounds = getClientBounds();

  switch (part) {

    case A_PART_NOTHING:
      break;

    case A_PART_SEPARATOR:
      return gfx::Rect(m_separator_x, 0,
        m_separator_x + m_separator_w, bounds.h);

    case A_PART_HEADER_EYE:
      return gfx::Rect(FRMSIZE*0, 0, FRMSIZE, HDRSIZE);

    case A_PART_HEADER_PADLOCK:
      return gfx::Rect(FRMSIZE*1, 0, FRMSIZE, HDRSIZE);

    case A_PART_HEADER_GEAR:
      return gfx::Rect(FRMSIZE*2, 0, FRMSIZE, HDRSIZE);

    case A_PART_HEADER_ONIONSKIN:
      return gfx::Rect(FRMSIZE*3, 0, FRMSIZE, HDRSIZE);

    case A_PART_HEADER_LAYER:
      return gfx::Rect(FRMSIZE*4, 0,
        m_separator_x - FRMSIZE*4, HDRSIZE);

    case A_PART_HEADER_FRAME:
      if (validFrame(frame)) {
        return gfx::Rect(m_separator_x + m_separator_w - 1 + FRMSIZE*frame - m_scroll_x,
          0, FRMSIZE, HDRSIZE);
      }
      break;

    case A_PART_LAYER:
      if (validLayer(layer)) {
        return gfx::Rect(0,
          HDRSIZE + LAYSIZE*(lastLayer()-layer) - m_scroll_y,
          m_separator_x, LAYSIZE);
      }
      break;

    case A_PART_LAYER_EYE_ICON:
      if (validLayer(layer)) {
        return gfx::Rect(0,
          HDRSIZE + LAYSIZE*(lastLayer()-layer) - m_scroll_y,
          FRMSIZE, LAYSIZE);
      }
      break;

    case A_PART_LAYER_PADLOCK_ICON:
      if (validLayer(layer)) {
        return gfx::Rect(FRMSIZE,
          HDRSIZE + LAYSIZE*(lastLayer()-layer) - m_scroll_y,
          FRMSIZE, LAYSIZE);
      }
      break;

    case A_PART_LAYER_TEXT:
      if (validLayer(layer)) {
        int x = FRMSIZE*2;
        return gfx::Rect(x,
          HDRSIZE + LAYSIZE*(lastLayer()-layer) - m_scroll_y,
          m_separator_x - x, LAYSIZE);
      }
      break;

    case A_PART_CEL:
      if (validLayer(layer) && frame >= frame_t(0)) {
        return gfx::Rect(
          m_separator_x + m_separator_w - 1 + FRMSIZE*frame - m_scroll_x,
          HDRSIZE + LAYSIZE*(lastLayer()-layer) - m_scroll_y,
          FRMSIZE, LAYSIZE);
      }
      break;

    case A_PART_RANGE_OUTLINE: {
      gfx::Rect rc = getRangeBounds(m_range);
      int s = OUTLINE_WIDTH;
      rc.enlarge(s);
      if (rc.x < bounds.x) rc.offset(s, 0).inflate(-s, 0);
      if (rc.y < bounds.y) rc.offset(0, s).inflate(0, -s);
      return rc;
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
      rc = getPartBounds(A_PART_CEL, range.layerBegin(), range.frameBegin()).createUnion(
        getPartBounds(A_PART_CEL, range.layerEnd(), range.frameEnd()));
      break;
    case Range::kFrames:
      rc = getPartBounds(A_PART_HEADER_FRAME, firstLayer(), range.frameBegin()).createUnion(
        getPartBounds(A_PART_HEADER_FRAME, firstLayer(), range.frameEnd()));
      break;
    case Range::kLayers:
      rc = getPartBounds(A_PART_LAYER, range.layerBegin()).createUnion(
        getPartBounds(A_PART_LAYER, range.layerEnd()));
      break;
  }
  return rc;
}

void Timeline::invalidatePart(int part, LayerIndex layer, frame_t frame)
{
  invalidateRect(getPartBounds(part, layer, frame).offset(getOrigin()));
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
}

void Timeline::updateHotByMousePos(ui::Message* msg, const gfx::Point& mousePos)
{
  int hot_part;
  LayerIndex hot_layer;
  frame_t hot_frame;
  updateHot(msg, mousePos, hot_part, hot_layer, hot_frame);

  if (hasMouseOver())
    setCursor(msg, mousePos);
}

void Timeline::updateHot(ui::Message* msg, const gfx::Point& mousePos, int& hot_part, LayerIndex& hot_layer, frame_t& hot_frame)
{
  hot_part = A_PART_NOTHING;
  hot_layer = LayerIndex::NoLayer;
  hot_frame = frame_t((mousePos.x
      - m_separator_x
      - m_separator_w
      + m_scroll_x) / FRMSIZE);

  if (!m_document)
    return;

  if (m_clk_part == A_PART_SEPARATOR) {
    hot_part = A_PART_SEPARATOR;
  }
  else {
    hot_layer = lastLayer() - LayerIndex(
      (mousePos.y
        - HDRSIZE
        + m_scroll_y) / LAYSIZE);

    hot_frame = frame_t((mousePos.x
        - m_separator_x
        - m_separator_w
        + m_scroll_x) / FRMSIZE);

    if (hasCapture()) {
      hot_layer = MID(firstLayer(), hot_layer, lastLayer());
      if (isMovingCel())
        hot_frame = MAX(firstFrame(), hot_frame);
      else
        hot_frame = MID(firstFrame(), hot_frame, lastFrame());
    }
    else {
      if (hot_layer > lastLayer()) hot_layer = LayerIndex::NoLayer;
      if (hot_frame > lastFrame()) hot_frame = frame_t(-1);
    }

    // Is the mouse over onionskin handles?
    gfx::Rect bounds = getOnionskinFramesBounds();
    if (!bounds.isEmpty() && gfx::Rect(bounds.x, bounds.y, 3, bounds.h).contains(mousePos)) {
      hot_part = A_PART_HEADER_ONIONSKIN_RANGE_LEFT;
    }
    else if (!bounds.isEmpty() && gfx::Rect(bounds.x+bounds.w-3, bounds.y, 3, bounds.h).contains(mousePos)) {
      hot_part = A_PART_HEADER_ONIONSKIN_RANGE_RIGHT;
    }
    // Is the mouse on the separator.
    else if (mousePos.x > m_separator_x-4
          && mousePos.x <= m_separator_x)  {
      hot_part = A_PART_SEPARATOR;
    }
    // Is the mouse on the headers?
    else if (mousePos.y < HDRSIZE) {
      if (mousePos.x < m_separator_x) {
        if (getPartBounds(A_PART_HEADER_EYE).contains(mousePos))
          hot_part = A_PART_HEADER_EYE;
        else if (getPartBounds(A_PART_HEADER_PADLOCK).contains(mousePos))
          hot_part = A_PART_HEADER_PADLOCK;
        else if (getPartBounds(A_PART_HEADER_GEAR).contains(mousePos))
          hot_part = A_PART_HEADER_GEAR;
        else if (getPartBounds(A_PART_HEADER_ONIONSKIN).contains(mousePos))
          hot_part = A_PART_HEADER_ONIONSKIN;
        else if (getPartBounds(A_PART_HEADER_LAYER).contains(mousePos))
          hot_part = A_PART_HEADER_LAYER;
      }
      else {
        hot_part = A_PART_HEADER_FRAME;
      }
    }
    else {
      // Is the mouse on a layer's label?
      if (mousePos.x < m_separator_x) {
        if (getPartBounds(A_PART_LAYER_EYE_ICON, hot_layer).contains(mousePos))
          hot_part = A_PART_LAYER_EYE_ICON;
        else if (getPartBounds(A_PART_LAYER_PADLOCK_ICON, hot_layer).contains(mousePos))
          hot_part = A_PART_LAYER_PADLOCK_ICON;
        else if (getPartBounds(A_PART_LAYER_TEXT, hot_layer).contains(mousePos))
          hot_part = A_PART_LAYER_TEXT;
        else
          hot_part = A_PART_LAYER;
      }
      else if (validLayer(hot_layer) && validFrame(hot_frame)) {
        hot_part = A_PART_CEL;
      }
      else
        hot_part = A_PART_NOTHING;
    }

    if (!hasCapture()) {
      gfx::Rect outline = getPartBounds(A_PART_RANGE_OUTLINE);
      if (outline.contains(mousePos)) {
        // With Ctrl and Alt key we can drag the range from any place (not necessary from the outline.
        if (isCopyKeyPressed(msg) ||
            !gfx::Rect(outline).shrink(2*OUTLINE_WIDTH).contains(mousePos)) {
          hot_part = A_PART_RANGE_OUTLINE;
        }
      }
    }
  }

  hotThis(hot_part, hot_layer, hot_frame);
}

void Timeline::hotThis(int hot_part, LayerIndex hot_layer, frame_t hot_frame)
{
  // If the part, layer or frame change.
  if (hot_part != m_hot_part ||
      hot_layer != m_hot_layer ||
      hot_frame != m_hot_frame) {
    // Invalidate the whole control.
    if (m_state == STATE_MOVING_RANGE ||
        hot_part == A_PART_RANGE_OUTLINE ||
        m_hot_part == A_PART_RANGE_OUTLINE) {
      invalidate();
    }
    // Invalidate the old and new 'hot' thing.
    else {
      invalidatePart(m_hot_part, m_hot_layer, m_hot_frame);
      invalidatePart(hot_part, hot_layer, hot_frame);
    }

    // Draw the new 'hot' thing.
    m_hot_part = hot_part;
    m_hot_layer = hot_layer;
    m_hot_frame = hot_frame;
  }
}

void Timeline::updateStatusBar(ui::Message* msg)
{
  StatusBar* sb = StatusBar::instance();

  if (m_state == STATE_MOVING_RANGE) {
    const char* verb = isCopyKeyPressed(msg) ? "Copy": "Move";

    switch (m_range.type()) {

      case Range::kCels:
        sb->setStatusText(0, "%s cels", verb);
        break;

      case Range::kFrames:
        if (validFrame(m_hot_frame)) {
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
    Layer* layer = (validLayer(m_hot_layer) ? m_layers[m_hot_layer]: NULL);

    switch (m_hot_part) {

      case A_PART_HEADER_ONIONSKIN: {
        ISettings* settings = UIContext::instance()->settings();
        IDocumentSettings* docSettings = settings->getDocumentSettings(m_document);
        if (docSettings) {
          sb->setStatusText(0, "Onionskin is %s",
            docSettings->getUseOnionskin() ? "enabled": "disabled");
          return;
        }
        break;
      }

      case A_PART_LAYER_TEXT:
        if (layer != NULL) {
          sb->setStatusText(0, "Layer '%s' [%s%s]",
            layer->name().c_str(),
            layer->isVisible() ? "visible": "hidden",
            layer->isEditable() ? "": " locked");
          return;
        }
        break;

      case A_PART_LAYER_EYE_ICON:
        if (layer != NULL) {
          sb->setStatusText(0, "Layer '%s' is %s",
            layer->name().c_str(),
            layer->isVisible() ? "visible": "hidden");
          return;
        }
        break;

      case A_PART_LAYER_PADLOCK_ICON:
        if (layer != NULL) {
          sb->setStatusText(0, "Layer '%s' is %s",
            layer->name().c_str(),
            layer->isEditable() ? "unlocked (editable)": "locked (read-only)");
          return;
        }
        break;

      case A_PART_HEADER_FRAME:
        if (validFrame(m_hot_frame)) {
          sb->setStatusText(0,
            "Frame %d [%d msecs]",
            (int)m_hot_frame+1,
            m_sprite->frameDuration(m_hot_frame));
          return;
        }
        break;

      case A_PART_CEL:
        if (layer) {
          Cel* cel = (layer->isImage() ? layer->cel(m_hot_frame): NULL);
          StatusBar::instance()->setStatusText(0,
            "%s at frame %d",
            cel ? "Cel": "Empty cel",
            (int)m_hot_frame+1);
          return;
        }
        break;
    }
  }

  sb->clearText();
}

void Timeline::centerCel(LayerIndex layer, frame_t frame)
{
  int target_x = (getBounds().x + m_separator_x + m_separator_w + getBounds().x2())/2 - FRMSIZE/2;
  int target_y = (getBounds().y + HDRSIZE + getBounds().y2())/2 - LAYSIZE/2;
  int scroll_x = getBounds().x + m_separator_x + m_separator_w + FRMSIZE*frame - target_x;
  int scroll_y = getBounds().y + HDRSIZE + LAYSIZE*(lastLayer() - layer) - target_y;

  setScroll(scroll_x, scroll_y);
}

void Timeline::showCel(LayerIndex layer, frame_t frame)
{
  int scroll_x, scroll_y;
  int x1, y1, x2, y2;

  x1 = getBounds().x + m_separator_x + m_separator_w + FRMSIZE*frame - m_scroll_x;
  y1 = getBounds().y + HDRSIZE + LAYSIZE*(lastLayer() - layer) - m_scroll_y;
  x2 = x1 + FRMSIZE - 1;
  y2 = y1 + LAYSIZE - 1;

  scroll_x = m_scroll_x;
  scroll_y = m_scroll_y;

  if (x1 < getBounds().x + m_separator_x + m_separator_w) {
    scroll_x -= (getBounds().x + m_separator_x + m_separator_w) - (x1);
  }
  else if (x2 > getBounds().x2()-1) {
    scroll_x += (x2) - (getBounds().x2()-1);
  }

  if (y1 < getBounds().y + HDRSIZE) {
    scroll_y -= (getBounds().y + HDRSIZE) - (y1);
  }
  else if (y2 > getBounds().y2()-1) {
    scroll_y += (y2) - (getBounds().y2()-1);
  }

  if (scroll_x != m_scroll_x ||
      scroll_y != m_scroll_y)
    setScroll(scroll_x, scroll_y);
}

void Timeline::showCurrentCel()
{
  LayerIndex layer = getLayerIndex(m_layer);
  if (layer >= firstLayer())
    showCel(layer, m_frame);
}

void Timeline::cleanClk()
{
  int clk_part = m_clk_part;
  m_clk_part = A_PART_NOTHING;

  invalidatePart(clk_part, m_clk_layer, m_clk_frame);
}

void Timeline::setScroll(int x, int y)
{
  int max_scroll_x = m_sprite->totalFrames() * FRMSIZE - getBounds().w/2;
  int max_scroll_y = m_layers.size() * LAYSIZE - getBounds().h/2;
  max_scroll_x = MAX(0, max_scroll_x);
  max_scroll_y = MAX(0, max_scroll_y);

  m_scroll_x = MID(0, x, max_scroll_x);
  m_scroll_y = MID(0, y, max_scroll_y);

  invalidate();
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

  int activeRelativeLayer = getLayerIndex(m_layer) - m_range.layerBegin();
  frame_t activeRelativeFrame = m_frame - m_range.frameBegin();

  try {
    if (copy)
      newFromRange = copy_range(m_document, m_range, m_dropRange, place);
    else
      newFromRange = move_range(m_document, m_range, m_dropRange, place);

    regenerateLayers();

    m_range = newFromRange;
    if (m_range.layerBegin() >= LayerIndex(0))
      setLayer(m_layers[m_range.layerBegin() + activeRelativeLayer]);
    if (m_range.frameBegin() >= frame_t(0))
      setFrame(m_range.frameBegin() + activeRelativeFrame);
  }
  catch (const std::exception& e) {
    ui::Alert::show("Problem<<%s||&OK", e.what());
  }

  // If we drop a cel in the same frame (but in another layer),
  // document views are not updated, so we are forcing the updating of
  // all views.
  m_document->notifyGeneralUpdate();

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
      frame_t dx = m_hot_frame - m_clk_frame;
      LayerIndex dy = m_hot_layer - m_clk_layer;
      LayerIndex layerIdx;
      frame_t frame;

      layerIdx = dy+m_range.layerBegin();
      layerIdx = MID(firstLayer(), layerIdx, LayerIndex(m_layers.size() - m_range.layers()));

      frame = dx+m_range.frameBegin();
      frame = MAX(firstFrame(), frame);

      m_dropRange.startRange(layerIdx, frame, m_range.type());
      m_dropRange.endRange(
        layerIdx+LayerIndex(m_range.layers()-1),
        frame+m_range.frames()-1);
      break;
    }

    case Range::kFrames: {
      frame_t frame = m_hot_frame;
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
      LayerIndex layer = m_hot_layer;
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

} // namespace app
