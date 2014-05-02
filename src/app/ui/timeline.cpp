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
#include "app/document_event.h"
#include "app/document_undo.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/ui/document_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/undo_transaction.h"
#include "base/compiler_specific.h"
#include "base/memory.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "raster/raster.h"
#include "ui/ui.h"

#include <allegro.h>
#include <cstdio>
#include <vector>

// Size of the thumbnail in the screen (width x height), the really
// size of the thumbnail bitmap is specified in the
// 'generate_thumbnail' routine.
#define THUMBSIZE       (12*jguiscale())

// Height of the headers.
#define HDRSIZE         THUMBSIZE

// Width of the frames.
#define FRMSIZE         THUMBSIZE

// Height of the layers.
#define LAYSIZE         THUMBSIZE

// Space between icons and other information in the layer.
#define ICONSEP         (2*jguiscale())

// Space between the icon-bitmap and the edge of the surrounding button.
#define ICONBORDER      0

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace raster;
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
  , m_context(UIContext::instance())
  , m_editor(NULL)
  , m_document(NULL)
  , m_scroll_x(0)
  , m_scroll_y(0)
  , m_separator_x(100 * jguiscale())
  , m_separator_w(1)
{
  m_context->addObserver(this);

  setDoubleBuffered(true);
}

Timeline::~Timeline()
{
  detachDocument();

  m_context->removeObserver(this);
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
  return (m_state == Timeline::STATE_MOVING_RANGE &&
          m_range.type() == Range::kCels);
}

void Timeline::setLayer(Layer* layer)
{
  ASSERT(m_editor != NULL);

  m_layer = layer;
  invalidate();

  if (m_editor->getLayer() != layer)
    m_editor->setLayer(m_layer);
}

void Timeline::setFrame(FrameNumber frame)
{
  ASSERT(m_editor != NULL);
  // ASSERT(frame >= 0 && frame < m_sprite->getTotalFrames());

  if (frame < 0)
    frame = FrameNumber(0);
  else if (frame >= m_sprite->getTotalFrames())
    frame = FrameNumber(m_sprite->getTotalFrames()-1);

  m_frame = frame;
  invalidate();

  if (m_editor->getFrame() != frame)
    m_editor->setFrame(m_frame);
}

bool Timeline::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kTimerMessage:
      break;

    case kMouseDownMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

      if (!m_document)
        break;

      if (mouseMsg->middle() || key[KEY_SPACE]) {
        captureMouse();
        m_state = STATE_SCROLLING;
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
          ISettings* m_settings = UIContext::instance()->getSettings();
          IDocumentSettings* docSettings = m_settings->getDocumentSettings(m_document);
          m_state = STATE_MOVING_ONIONSKIN_RANGE_LEFT;
          m_origFrames = docSettings->getOnionskinPrevFrames();
          break;
        }
        case A_PART_HEADER_ONIONSKIN_RANGE_RIGHT: {
          ISettings* m_settings = UIContext::instance()->getSettings();
          IDocumentSettings* docSettings = m_settings->getDocumentSettings(m_document);
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
          const DocumentReader document(const_cast<Document*>(m_document));
          const Sprite* sprite = m_sprite;
          int old_layer = getLayerIndex(m_layer);
          bool selectLayer = (mouseMsg->left() || !isLayerActive(m_clk_layer));
          FrameNumber frame = m_frame;

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
          const DocumentReader document(const_cast<Document*>(m_document));
          const Sprite* sprite = document->getSprite();
          int old_layer = getLayerIndex(m_layer);
          bool selectCel = (mouseMsg->left()
            || !isLayerActive(m_clk_layer)
            || !isFrameActive(m_clk_frame));
          FrameNumber old_frame = m_frame;

          // Select the new clicked-part.
          if (old_layer != m_clk_layer
            || old_frame != m_clk_frame) {
            setLayer(m_layers[m_clk_layer]);
            setFrame(m_clk_frame);
            invalidate();
          }

          // Change the scroll to show the new selected cel.
          showCel(m_clk_layer, m_frame);

          m_state = STATE_SELECTING_CELS;
          if (selectCel)
            m_range.startRange(m_clk_layer, m_clk_frame, Range::kCels);
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

      int hot_part = A_PART_NOTHING;
      int hot_layer = -1;
      gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position()
        - getBounds().getOrigin();

      FrameNumber hot_frame =
        FrameNumber((mousePos.x
            - m_separator_x
            - m_separator_w
            + m_scroll_x) / FRMSIZE);

      if (hasCapture()) {
        switch (m_state) {

          case STATE_SCROLLING: {
            gfx::Point absMousePos = static_cast<MouseMessage*>(msg)->position();
            setScroll(
              m_scroll_x + m_oldPos.x - absMousePos.x,
              m_scroll_y + m_oldPos.y - absMousePos.y);

            m_oldPos = ui::control_infinite_scroll(this, getBounds(), absMousePos);
            return true;
          }

          case STATE_MOVING_ONIONSKIN_RANGE_LEFT: {
            ISettings* m_settings = UIContext::instance()->getSettings();
            IDocumentSettings* docSettings = m_settings->getDocumentSettings(m_document);
            int newValue = m_origFrames + (m_clk_frame - hot_frame);
            docSettings->setOnionskinPrevFrames(MAX(0, newValue));
            invalidate();
            return true;
          }

          case STATE_MOVING_ONIONSKIN_RANGE_RIGHT:
            ISettings* m_settings = UIContext::instance()->getSettings();
            IDocumentSettings* docSettings = m_settings->getDocumentSettings(m_document);
            int newValue = m_origFrames - (m_clk_frame - hot_frame);
            docSettings->setOnionskinNextFrames(MAX(0, newValue));
            invalidate();
            return true;
        }

        // If the mouse pressed the mouse's button in the separator,
        // we shouldn't change the hot (so the separator can be
        // tracked to the mouse's released).
        if (m_clk_part == A_PART_SEPARATOR) {
          hot_part = m_clk_part;
          m_separator_x = MAX(0, mousePos.x);
          invalidate();
          return true;
        }
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
        && mousePos.x < m_separator_x+4)  {
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
          hot_frame = FrameNumber((mousePos.x
              - m_separator_x
              - m_separator_w
              + m_scroll_x) / FRMSIZE);
        }
      }
      else {
        hot_layer = (mousePos.y
                     - HDRSIZE
                     + m_scroll_y) / LAYSIZE;

        if (hot_layer >= (int)m_layers.size()) {
          if (hasCapture())
            hot_layer = m_layers.size()-1;
          else
            hot_layer = -1;
        }
        else if (hasCapture() && hot_layer < 0)
          hot_layer = 0;

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
        else if (hot_layer >= 0 && hot_layer < (int)m_layers.size() &&
                 hot_frame >= FrameNumber(0) && hot_frame <= m_sprite->getLastFrame()) {
          hot_part = A_PART_CEL;
        }
        else
          hot_part = A_PART_NOTHING;
      }

      if (hasCapture()) {
        hot_layer = MID(0, hot_layer, (int)m_layers.size()-1);
        hot_frame = MID(FrameNumber(0), hot_frame, m_sprite->getLastFrame());
      }
      else {
        if (hot_layer >= (int)m_layers.size()) hot_layer = -1;
        if (hot_frame > m_sprite->getLastFrame()) hot_frame = FrameNumber(-1);

        gfx::Rect outline = getPartBounds(A_PART_RANGE_OUTLINE);
        if (outline.contains(mousePos) &&
            !gfx::Rect(outline).shrink(4*jguiscale()).contains(mousePos)) {
          hot_part = A_PART_RANGE_OUTLINE;
        }
      }

      // Set the new 'hot' thing.
      hotThis(hot_part, hot_layer, hot_frame);

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
      updateStatusBar();
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
            bool newReadableState = !allLayersVisible();
            for (size_t i=0; i<m_layers.size(); i++)
              m_layers[i]->setReadable(newReadableState);

            // Redraw all views.
            m_document->notifyGeneralUpdate();
            break;
          }

          case A_PART_HEADER_PADLOCK: {
            bool newWritableState = !allLayersUnlocked();
            for (size_t i=0; i<m_layers.size(); i++)
              m_layers[i]->setWritable(newWritableState);
            break;
          }

          case A_PART_HEADER_GEAR:
            // TODO show timeline/onionskin configuration
            break;

          case A_PART_HEADER_ONIONSKIN: {
            ISettings* m_settings = UIContext::instance()->getSettings();
            IDocumentSettings* docSettings =
              m_settings->getDocumentSettings(m_document);
            if (docSettings)
              docSettings->setUseOnionskin(!docSettings->getUseOnionskin());
            break;
          }
          case A_PART_HEADER_FRAME:
            // Show the frame pop-up menu.
            if (mouseMsg->right()) {
              if (m_clk_frame == m_hot_frame) {
                Menu* popup_menu = AppMenus::instance()->getFramePopupMenu();
                if (popup_menu != NULL) {
                  gfx::Point mousePos = mouseMsg->position();
                  popup_menu->showPopup(mousePos.x, mousePos.y);
                }
              }
            }
            break;
          case A_PART_LAYER_TEXT:
            // Show the layer pop-up menu.
            if (mouseMsg->right()) {
              if (m_clk_layer == m_hot_layer) {
                Menu* popup_menu = AppMenus::instance()->getLayerPopupMenu();
                if (popup_menu != NULL) {
                  gfx::Point mousePos = mouseMsg->position();
                  popup_menu->showPopup(mousePos.x, mousePos.y);
                }
              }
            }
            break;
          case A_PART_LAYER_EYE_ICON:
            // Hide/show layer.
            if (m_hot_layer == m_clk_layer &&
                m_hot_layer >= 0 &&
                m_hot_layer < (int)m_layers.size()) {
              Layer* layer = m_layers[m_clk_layer];
              ASSERT(layer != NULL);
              layer->setReadable(!layer->isReadable());

              // Redraw all views.
              m_document->notifyGeneralUpdate();
            }
            break;
          case A_PART_LAYER_PADLOCK_ICON:
            // Lock/unlock layer.
            if (m_hot_layer == m_clk_layer &&
                m_hot_layer >= 0 &&
                m_hot_layer < (int)m_layers.size()) {
              Layer* layer = m_layers[m_clk_layer];
              ASSERT(layer != NULL);
              layer->setWritable(!layer->isWritable());
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

              if (popup_menu != NULL) {
                gfx::Point mousePos = mouseMsg->position();
                popup_menu->showPopup(mousePos.x, mousePos.y);
              }
            }
            break;
          }
        }

        if (mouseMsg->left() && m_state == STATE_MOVING_RANGE) {
          dropRange(mouseMsg->ctrlPressed() ?
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
        setCursor(mouseMsg->position().x,
                  mouseMsg->position().y);

        releaseMouse();
        updateStatusBar();
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

    case kKeyDownMessage:
      switch (static_cast<KeyMessage*>(msg)->scancode()) {

        case kKeySpace: {
          setCursor(jmouse_x(0), jmouse_y(0));
          return true;
        }
      }
      break;

    case kKeyUpMessage:
      switch (static_cast<KeyMessage*>(msg)->scancode()) {

        case kKeyEsc:
          m_range.disableRange();
          invalidate();
          break;

        case kKeySpace: {
          // We have to clear all the KEY_SPACE in buffer.
          clear_keybuf();

          setCursor(jmouse_x(0), jmouse_y(0));
          return true;
        }
      }
      break;

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
        setCursor(mousePos.x, mousePos.y);
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
    
    int layer, first_layer, last_layer;
    FrameNumber frame, first_frame, last_frame;

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
    for (layer=first_layer; layer<=last_layer; layer++) {
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
        for (; it != end && (*it)->getFrame() < first_frame; ++it)
          ;
      }

      IntersectClip clip(g, getCelsBounds());
      if (!clip)
        continue;

      // Draw every visible cel for each layer.
      for (frame=first_frame; frame<=last_frame; ++frame) {
        Cel* cel = (layerPtr->isImage() && it != end && (*it)->getFrame() == frame ? *it: NULL);
        drawCel(g, layer, frame, cel);

        if (cel)
          ++it;               // Go to next cel
      }
    }

    drawPaddings(g);
    drawRangeOutline(g);
  }
  catch (const LockedDocumentException&) {
    noDoc = true;
  }

paintNoDoc:;
  if (noDoc)
    drawPart(g, getClientBounds(), NULL, m_timelinePaddingStyle);
}

void Timeline::onCommandAfterExecution(Context* context)
{
  if (!m_document)
    return;

  regenerateLayers();
  showCurrentCel();
  invalidate();
}

void Timeline::onRemoveDocument(Context* context, Document* document)
{
  if (document == m_document)
    detachDocument();
}

void Timeline::onAddLayer(DocumentEvent& ev)
{
  ASSERT(ev.layer() != NULL);

  setLayer(ev.layer());

  regenerateLayers();
  showCurrentCel();
  invalidate();
}

void Timeline::onAfterRemoveLayer(DocumentEvent& ev)
{
  Sprite* sprite = ev.sprite();
  Layer* layer = ev.layer();

  // If the layer that was removed is the selected one
  if (layer == getLayer()) {
    LayerFolder* parent = layer->getParent();
    Layer* layer_select = NULL;

    // Select previous layer, or next layer, or the parent (if it is
    // not the main layer of sprite set).
    if (layer->getPrevious())
      layer_select = layer->getPrevious();
    else if (layer->getNext())
      layer_select = layer->getNext();
    else if (parent != sprite->getFolder())
      layer_select = parent;

    setLayer(layer_select);
  }

  regenerateLayers();
  showCurrentCel();
  invalidate();
}

void Timeline::onAddFrame(DocumentEvent& ev)
{
  setFrame(ev.frame());

  showCurrentCel();
  invalidate();
}

void Timeline::onRemoveFrame(DocumentEvent& ev)
{
  // Adjust current frame of all editors that are in a frame more
  // advanced that the removed one.
  if (getFrame() > ev.frame()) {
    setFrame(getFrame().previous());
  }
  // If the editor was in the previous "last frame" (current value of
  // getTotalFrames()), we've to adjust it to the new last frame
  // (getLastFrame())
  else if (getFrame() >= getSprite()->getTotalFrames()) {
    setFrame(getSprite()->getLastFrame());
  }

  showCurrentCel();
  invalidate();
}

void Timeline::onFrameChanged(Editor* editor)
{
  setFrame(editor->getFrame());

  if (!hasCapture())
    m_range.disableRange();

  showCurrentCel();
}

void Timeline::onLayerChanged(Editor* editor)
{
  setLayer(editor->getLayer());

  if (!hasCapture())
    m_range.disableRange();

  showCurrentCel();
}

void Timeline::setCursor(int x, int y)
{
  int mx = x - getBounds().x;

  // Scrolling.
  if (m_state == STATE_SCROLLING || key[KEY_SPACE]) {
    jmouse_set_cursor(kScrollCursor);
  }
  // Moving.
  else if (m_state == STATE_MOVING_RANGE) {
    if (key[KEY_LCONTROL] || key[KEY_RCONTROL])
      jmouse_set_cursor(kArrowPlusCursor);
    else
      jmouse_set_cursor(kMoveCursor);
  }
  // Normal state.
  else if (m_hot_part == A_PART_HEADER_ONIONSKIN_RANGE_LEFT
    || m_state == STATE_MOVING_ONIONSKIN_RANGE_LEFT) {
    jmouse_set_cursor(kSizeLCursor);
  }
  else if (m_hot_part == A_PART_HEADER_ONIONSKIN_RANGE_RIGHT
    || m_state == STATE_MOVING_ONIONSKIN_RANGE_RIGHT) {
    jmouse_set_cursor(kSizeRCursor);
  }
  else if (m_hot_part == A_PART_RANGE_OUTLINE) {
    jmouse_set_cursor(kMoveCursor);
  }
  else if (mx > m_separator_x-2 && mx < m_separator_x+2)  {
      // Is the mouse in the separator.
    jmouse_set_cursor(kSizeLCursor);
  }
  else {
    jmouse_set_cursor(kArrowCursor);
  }
}

void Timeline::getDrawableLayers(ui::Graphics* g, int* first_layer, int* last_layer)
{
  *first_layer = m_scroll_y / LAYSIZE;
  *first_layer = MID(0, *first_layer, (int)m_layers.size()-1);
  *last_layer = *first_layer + (getClientBounds().h - HDRSIZE) / LAYSIZE;
  *last_layer = MID(0, *last_layer, (int)m_layers.size()-1);

  if (m_layers.empty())
    *last_layer = -1;
}

void Timeline::getDrawableFrames(ui::Graphics* g, FrameNumber* first_frame, FrameNumber* last_frame)
{
  *first_frame = FrameNumber((m_separator_w + m_scroll_x) / FRMSIZE);
  *last_frame = *first_frame
    + FrameNumber((getClientBounds().w - m_separator_w) / FRMSIZE);
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

void Timeline::drawHeader(ui::Graphics* g)
{
  ISettings* m_settings = UIContext::instance()->getSettings();
  IDocumentSettings* docSettings = m_settings->getDocumentSettings(m_document);
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

void Timeline::drawHeaderFrame(ui::Graphics* g, FrameNumber frame)
{
  bool is_active = isFrameActive(frame);
  bool is_hover = (m_hot_part == A_PART_HEADER_FRAME && m_hot_frame == frame);
  bool is_clicked = (m_clk_part == A_PART_HEADER_FRAME && m_clk_frame == frame);
  gfx::Rect bounds = getPartBounds(A_PART_HEADER_FRAME, 0, frame);
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  // Draw the header for the layers.
  char buf[256];
  std::sprintf(buf, "%d", (frame+1)%100); // Draw only the first two digits.

  FONT* oldFont = g->getFont();
  g->setFont(((SkinTheme*)getTheme())->getMiniFont());
  drawPart(g, bounds, buf, m_timelineBoxStyle,
    is_active, is_hover, is_clicked);
  g->setFont(oldFont);
}

void Timeline::drawLayer(ui::Graphics* g, int layer_index)
{
  Layer* layer = m_layers[layer_index];
  bool is_active = isLayerActive(layer_index);
  bool hotlayer = (m_hot_layer == layer_index);
  bool clklayer = (m_clk_layer == layer_index);
  gfx::Rect bounds = getPartBounds(A_PART_LAYER, layer_index, FrameNumber(0));
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  // Draw the eye (readable flag).
  bounds = getPartBounds(A_PART_LAYER_EYE_ICON, layer_index);
  drawPart(g, bounds, NULL,
    layer->isReadable() ? m_timelineOpenEyeStyle: m_timelineClosedEyeStyle,
    is_active,
    (hotlayer && m_hot_part == A_PART_LAYER_EYE_ICON),
    (clklayer && m_clk_part == A_PART_LAYER_EYE_ICON));

  // Draw the padlock (writable flag).
  bounds = getPartBounds(A_PART_LAYER_PADLOCK_ICON, layer_index);
  drawPart(g, bounds, NULL,
    layer->isWritable() ? m_timelineOpenPadlockStyle: m_timelineClosedPadlockStyle,
    is_active,
    (hotlayer && m_hot_part == A_PART_LAYER_PADLOCK_ICON),
    (clklayer && m_clk_part == A_PART_LAYER_PADLOCK_ICON));

  // Draw the layer's name.
  bounds = getPartBounds(A_PART_LAYER_TEXT, layer_index);
  drawPart(g, bounds, layer->getName().c_str(), m_timelineLayerStyle,
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

void Timeline::drawCel(ui::Graphics* g, int layer_index, FrameNumber frame, Cel* cel)
{
  Layer* layer = m_layers[layer_index];
  Image* image = (cel ? m_sprite->getStock()->getImage(cel->getImage()): NULL);
  bool is_hover = (m_hot_part == A_PART_CEL &&
    m_hot_layer == layer_index &&
    m_hot_frame == frame);
  bool is_active = (isLayerActive(layer_index) || isFrameActive(frame));
  bool is_empty = (image == NULL);
  gfx::Rect bounds = getPartBounds(A_PART_CEL, layer_index, frame);
  IntersectClip clip(g, bounds);
  if (!clip)
    return;

  if (layer_index == getLayerIndex(m_layer) && frame == m_frame)
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
    Cel* left = (layer->isImage() ? static_cast<LayerImage*>(layer)->getCel(frame.previous()): NULL);
    Cel* right = (layer->isImage() ? static_cast<LayerImage*>(layer)->getCel(frame.next()): NULL);
    Image* leftImg = (left ? m_sprite->getStock()->getImage(left->getImage()): NULL);
    Image* rightImg = (right ? m_sprite->getStock()->getImage(right->getImage()): NULL);
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

void Timeline::drawRangeOutline(ui::Graphics* g)
{
  gfx::Rect clipBounds;
  switch (m_range.type()) {
    case Range::kCels: clipBounds = getCelsBounds(); break;
    case Range::kFrames: clipBounds = getFrameHeadersBounds(); break;
    case Range::kLayers: clipBounds = getLayerHeadersBounds(); break;
  }
  IntersectClip clip(g, clipBounds);
  if (!clip)
    return;

  Style::State state;
  if (m_range.enabled()) state += Style::active();
  if (m_hot_part == A_PART_RANGE_OUTLINE) state += Style::hover();

  gfx::Rect bounds = getPartBounds(A_PART_RANGE_OUTLINE);
  m_timelineRangeOutlineStyle->paint(g, bounds, NULL, state);

  Range drop = getDropRange();
  switch (drop.type()) {

    case Range::kCels:
      bounds =
        getPartBounds(A_PART_CEL, drop.layerBegin(), drop.frameBegin()).createUnion(
          getPartBounds(A_PART_CEL, drop.layerEnd(), drop.frameEnd()))
        // TODO theme specific
        .enlarge(2*jguiscale());

      m_timelineRangeOutlineStyle->paint(g, bounds, NULL, Style::active());
      break;

    case Range::kFrames:
      bounds = getPartBounds(A_PART_HEADER_FRAME, 0, drop.frameBegin());
      bounds.w = 5 * jguiscale(); // TODO get height from the skin info
      bounds.x -= bounds.w/2;

      m_timelineDropFrameDecoStyle->paint(g, bounds, NULL, Style::State());
      break;

    case Range::kLayers:
      bounds = getPartBounds(A_PART_LAYER, drop.layerBegin());
      bounds.h = 5 * jguiscale(); // TODO get height from the skin info
      bounds.y -= bounds.h/2;

      m_timelineDropLayerDecoStyle->paint(g, bounds, NULL, Style::State());
      break;
  }
}

void Timeline::drawPaddings(ui::Graphics* g)
{
  gfx::Rect client = getClientBounds();
  gfx::Rect lastLayer;
  gfx::Rect lastFrame;

  if (!m_layers.empty()) {
    lastLayer = getPartBounds(A_PART_LAYER, m_layers.size()-1);
    lastFrame = getPartBounds(A_PART_CEL, 0, m_sprite->getLastFrame());
  }
  else {
    lastLayer = getPartBounds(A_PART_HEADER_LAYER);
    lastFrame = getPartBounds(A_PART_HEADER_FRAME, 0, m_sprite->getLastFrame());
  }

  drawPart(g,
    gfx::Rect(lastFrame.x+lastFrame.w, client.y,
      client.w - (lastFrame.x+lastFrame.w),
      lastLayer.y+lastLayer.h),
    NULL, m_timelinePaddingTrStyle);

  drawPart(g,
    gfx::Rect(client.x, lastLayer.y+lastLayer.h,
      lastFrame.x+lastFrame.w - client.x, client.h - (lastLayer.y+lastLayer.h)),
    NULL, m_timelinePaddingBlStyle);

  drawPart(g,
    gfx::Rect(lastFrame.x+lastFrame.w, lastLayer.y+lastLayer.h,
      client.w - (lastFrame.x+lastFrame.w),
      client.h - (lastLayer.y+lastLayer.h)),
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
  ISettings* m_settings = UIContext::instance()->getSettings();
  IDocumentSettings* docSettings = m_settings->getDocumentSettings(m_document);
  if (docSettings->getUseOnionskin()) {
    FrameNumber firstFrame = m_frame.previous(docSettings->getOnionskinPrevFrames());
    FrameNumber lastFrame = m_frame.next(docSettings->getOnionskinNextFrames());

    if (firstFrame < 0) firstFrame = FrameNumber(0);
    if (lastFrame > m_sprite->getLastFrame()) lastFrame = m_sprite->getLastFrame();

    return getPartBounds(A_PART_HEADER_FRAME, 0, firstFrame)
      .createUnion(getPartBounds(A_PART_HEADER_FRAME, 0, lastFrame));
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

gfx::Rect Timeline::getPartBounds(int part, int layer, FrameNumber frame) const
{
  const gfx::Rect bounds = getBounds();

  switch (part) {

    case A_PART_NOTHING:
      break;

    case A_PART_SEPARATOR:
      return gfx::Rect(m_separator_x, 0,
                       m_separator_x + m_separator_w,
                       bounds.h);

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
                       m_separator_x - FRMSIZE*4,
                       HDRSIZE);

    case A_PART_HEADER_FRAME:
      if (frame >= 0 && frame < m_sprite->getTotalFrames()) {
        return gfx::Rect(m_separator_x + m_separator_w - 1 + FRMSIZE*frame - m_scroll_x,
                         0,
                         FRMSIZE,
                         HDRSIZE);
      }
      break;

    case A_PART_LAYER:
      if (layer >= 0 && layer < (int)m_layers.size()) {
        return gfx::Rect(0,
                         HDRSIZE + LAYSIZE*layer - m_scroll_y,
                         m_separator_x,
                         LAYSIZE);
      }
      break;

    case A_PART_LAYER_EYE_ICON:
      if (layer >= 0 && layer < (int)m_layers.size()) {
        return gfx::Rect(0,
                         HDRSIZE + LAYSIZE*layer - m_scroll_y,
                         FRMSIZE,
                         LAYSIZE);
      }
      break;

    case A_PART_LAYER_PADLOCK_ICON:
      if (layer >= 0 && layer < (int)m_layers.size()) {
        return gfx::Rect(FRMSIZE,
                         HDRSIZE + LAYSIZE*layer - m_scroll_y,
                         FRMSIZE,
                         LAYSIZE);
      }
      break;

    case A_PART_LAYER_TEXT:
      if (layer >= 0 && layer < (int)m_layers.size()) {
        int x = FRMSIZE*2;
        return gfx::Rect(x,
                         HDRSIZE + LAYSIZE*layer - m_scroll_y,
                         m_separator_x - x,
                         LAYSIZE);
      }
      break;

    case A_PART_CEL:
      if (layer >= 0 && layer < (int)m_layers.size() &&
          frame >= 0 && frame < m_sprite->getTotalFrames()) {
        Cel* cel = (m_layers[layer]->isImage() ? static_cast<LayerImage*>(m_layers[layer])->getCel(frame): NULL);

        return gfx::Rect(m_separator_x + m_separator_w - 1 + FRMSIZE*frame - m_scroll_x,
                         HDRSIZE + LAYSIZE*layer - m_scroll_y,
                         FRMSIZE,
                         LAYSIZE);
      }
      break;

    case A_PART_RANGE_OUTLINE:
      switch (m_range.type()) {
        case Range::kNone: break; // Return empty rectangle
        case Range::kCels:
          return
            getPartBounds(A_PART_CEL, m_range.layerBegin(), m_range.frameBegin()).createUnion(
              getPartBounds(A_PART_CEL, m_range.layerEnd(), m_range.frameEnd()));
          // TODO theme specific
          //.enlarge(2*jguiscale());
        case Range::kFrames:
          return
            getPartBounds(A_PART_HEADER_FRAME, 0, m_range.frameBegin()).createUnion(
              getPartBounds(A_PART_HEADER_FRAME, 0, m_range.frameEnd()));
        case Range::kLayers:
          return
            getPartBounds(A_PART_LAYER, m_range.layerBegin()).createUnion(
              getPartBounds(A_PART_LAYER, m_range.layerEnd()));
      }
      break;
  }

  return gfx::Rect();
}

void Timeline::invalidatePart(int part, int layer, FrameNumber frame)
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
    m_layers[c] = m_sprite->indexToLayer(LayerIndex(nlayers-c-1));
}

void Timeline::hotThis(int hot_part, int hot_layer, FrameNumber hot_frame)
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
      invalidatePart(m_hot_part,
        m_hot_layer,
        m_hot_frame);
      invalidatePart(m_hot_part,
        m_hot_layer,
        m_hot_frame);
    }

    // Draw the new 'hot' thing.
    m_hot_part = hot_part;
    m_hot_layer = hot_layer;
    m_hot_frame = hot_frame;

    updateStatusBar();
  }
}

void Timeline::updateStatusBar()
{
  Layer* layer = ((m_hot_layer >= 0
      && m_hot_layer < (int)m_layers.size()) ? m_layers[m_hot_layer]: NULL);

  switch (m_hot_part) {

    case A_PART_HEADER_ONIONSKIN: {
      ISettings* m_settings = UIContext::instance()->getSettings();
      IDocumentSettings* docSettings = m_settings->getDocumentSettings(m_document);
      if (docSettings) {
        StatusBar::instance()->setStatusText(0, "Onionskin is %s",
          docSettings->getUseOnionskin() ? "enabled": "disabled");
      }
      else {
        StatusBar::instance()->clearText();
      }
      break;
    }

    case A_PART_LAYER_TEXT:
      if (layer != NULL) {
        StatusBar::instance()->setStatusText(0, "Layer '%s' [%s%s]",
          layer->getName().c_str(),
          layer->isReadable() ? "visible": "hidden",
          layer->isWritable() ? "": " locked");
      }
      else {
        StatusBar::instance()->clearText();
      }
      break;

    case A_PART_LAYER_EYE_ICON:
      if (layer != NULL) {
        StatusBar::instance()->setStatusText(0, "Layer '%s' is %s",
          layer->getName().c_str(),
          layer->isReadable() ? "visible": "hidden");
      }
      else {
        StatusBar::instance()->clearText();
      }
      break;

    case A_PART_LAYER_PADLOCK_ICON:
      if (layer != NULL) {
        StatusBar::instance()->setStatusText(0, "Layer '%s' is %s",
          layer->getName().c_str(),
          layer->isWritable() ? "unlocked (modifiable)": "locked (read-only)");
      }
      else {
        StatusBar::instance()->clearText();
      }
      break;

    case A_PART_HEADER_FRAME:
      if (m_hot_frame >= FrameNumber(0)
        && m_hot_frame <= m_sprite->getLastFrame()) {
        StatusBar::instance()->setStatusText(0,
          "Frame %d [%d msecs]",
          (int)m_hot_frame+1,
          m_sprite->getFrameDuration(m_hot_frame));
      }
      break;

    case A_PART_CEL:
      if (layer) {
        Cel* cel = (layer->isImage() ? static_cast<LayerImage*>(layer)->getCel(m_hot_frame): NULL);
        StatusBar::instance()->setStatusText(0,
          "%s at frame %d",
          cel ? "Cel": "Empty cel",
          (int)m_hot_frame+1);
      }
      else {
        StatusBar::instance()->clearText();
      }
      break;

    default:
      StatusBar::instance()->clearText();
      break;
  }
}

void Timeline::centerCel(int layer, FrameNumber frame)
{
  int target_x = (getBounds().x + m_separator_x + m_separator_w + getBounds().x2())/2 - FRMSIZE/2;
  int target_y = (getBounds().y + HDRSIZE + getBounds().y2())/2 - LAYSIZE/2;
  int scroll_x = getBounds().x + m_separator_x + m_separator_w + FRMSIZE*frame - target_x;
  int scroll_y = getBounds().y + HDRSIZE + LAYSIZE*layer - target_y;

  setScroll(scroll_x, scroll_y);
}

void Timeline::showCel(int layer, FrameNumber frame)
{
  int scroll_x, scroll_y;
  int x1, y1, x2, y2;

  x1 = getBounds().x + m_separator_x + m_separator_w + FRMSIZE*frame - m_scroll_x;
  y1 = getBounds().y + HDRSIZE + LAYSIZE*layer - m_scroll_y;
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
  int layer = getLayerIndex(m_layer);
  if (layer >= 0)
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
  int max_scroll_x = m_sprite->getTotalFrames() * FRMSIZE - getBounds().w/2;
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
    if (!m_layers[i]->isReadable())
      return false;

  return true;
}

bool Timeline::allLayersInvisible()
{
  for (size_t i=0; i<m_layers.size(); i++)
    if (m_layers[i]->isReadable())
      return false;

  return true;
}

bool Timeline::allLayersLocked()
{
  for (size_t i=0; i<m_layers.size(); i++)
    if (m_layers[i]->isWritable())
      return false;

  return true;
}

bool Timeline::allLayersUnlocked()
{
  for (size_t i=0; i<m_layers.size(); i++)
    if (!m_layers[i]->isWritable())
      return false;

  return true;
}

int Timeline::getLayerIndex(const Layer* layer) const
{
  for (size_t i=0; i<m_layers.size(); i++)
    if (m_layers[i] == layer)
      return i;

  return -1;
}

bool Timeline::isLayerActive(int layer_index) const
{
  if (layer_index == getLayerIndex(m_layer))
    return true;
  else
    return m_range.inRange(layer_index);
}

bool Timeline::isFrameActive(FrameNumber frame) const
{
  if (frame == m_frame)
    return true;
  else
    return m_range.inRange(frame);
}

void Timeline::dropRange(DropOp op)
{
  // "Do nothing" cases. The user drops in the same place. (We don't
  // even add the undo information.)
  Range drop = getDropRange();
  switch (drop.type()) {
    case Range::kCels:
      if (drop == m_range)
        return;
      break;
    case Range::kFrames:
      if (op == Timeline::kMove && drop.frameBegin() == m_range.frameBegin())
        return;
      break;
    case Range::kLayers:
      if (op == Timeline::kMove && drop.layerBegin() == m_range.layerBegin())
        return;
      break;
  }

  const char* undoLabel = NULL;
  switch (op) {
    case Timeline::kMove: undoLabel = "Move Range"; break;
    case Timeline::kCopy: undoLabel = "Copy Range"; break;
  }

  const ContextReader reader(m_context);
  ContextWriter writer(reader);
  UndoTransaction undo(writer.context(), undoLabel, undo::ModifyDocument);
  int activeRelativeLayer = getLayerIndex(m_layer) - m_range.layerBegin();
  FrameNumber activeRelativeFrame = m_frame - m_range.frameBegin();

  switch (drop.type()) {
    case Range::kCels: dropCels(op, drop); break;
    case Range::kFrames: dropFrames(op, drop); break;
    case Range::kLayers: dropLayers(op, drop); break;
  }

  undo.commit();

  regenerateLayers();

  // Adjust "drop" range so we can select the same selected range that
  // the user had selected.
  switch (drop.type()) {

    case Range::kFrames:
      if (op == Timeline::kMove && m_range.frameBegin() < drop.frameBegin()) {
        drop.displace(0, FrameNumber(-m_range.frames()));
      }
      drop.setFrames(m_range.frames());
      break;

    case Range::kLayers:
      if (op == Timeline::kMove && m_range.layerBegin() < drop.layerBegin()) {
        drop.displace(-m_range.layers(), FrameNumber(0));
      }
      drop.setLayers(m_range.layers());
      break;
  }

  setLayer(m_layers[drop.layerBegin() + activeRelativeLayer]);
  setFrame(drop.frameBegin() + activeRelativeFrame);
  m_range = drop;

  invalidate();
}

void Timeline::dropCels(DropOp op, const Range& drop)
{
  ASSERT(drop.layerBegin() >= 0 && drop.layerBegin() < (int)m_layers.size());
  ASSERT(drop.layerEnd() >= 0 && drop.layerEnd() < (int)m_layers.size());
  ASSERT(drop.frameBegin() >= FrameNumber(0) && drop.frameBegin() < m_sprite->getTotalFrames());
  ASSERT(drop.frameEnd() >= FrameNumber(0) && drop.frameEnd() < m_sprite->getTotalFrames());

  int srcLayerBegin, srcLayerStep, srcLayerEnd;
  int dstLayerBegin, dstLayerStep;
  FrameNumber srcFrameBegin, srcFrameStep, srcFrameEnd;
  FrameNumber dstFrameBegin, dstFrameStep;

  if (drop.layerBegin() <= m_range.layerBegin()) {
    srcLayerBegin = m_range.layerBegin();
    srcLayerStep = 1;
    srcLayerEnd = m_range.layerEnd()+1;
    dstLayerBegin = drop.layerBegin();
    dstLayerStep = 1;
  }
  else {
    srcLayerBegin = m_range.layerEnd();
    srcLayerStep = -1;
    srcLayerEnd = m_range.layerBegin()-1;
    dstLayerBegin = drop.layerEnd();
    dstLayerStep = -1;
  }

  if (drop.frameBegin() <= m_range.frameBegin()) {
    srcFrameBegin = m_range.frameBegin();
    srcFrameStep = FrameNumber(1);
    srcFrameEnd = m_range.frameEnd().next();
    dstFrameBegin = drop.frameBegin();
    dstFrameStep = FrameNumber(1);
  }
  else {
    srcFrameBegin = m_range.frameEnd();
    srcFrameStep = FrameNumber(-1);
    srcFrameEnd = m_range.frameBegin().previous();
    dstFrameBegin = drop.frameEnd();
    dstFrameStep = FrameNumber(-1);
  }

  DocumentApi api = m_document->getApi();

  for (int srcLayerIdx = srcLayerBegin,
           dstLayerIdx = dstLayerBegin; srcLayerIdx != srcLayerEnd; ) {
    for (FrameNumber srcFrame = srcFrameBegin,
                     dstFrame = dstFrameBegin; srcFrame != srcFrameEnd; ) {
      LayerImage* srcLayer = static_cast<LayerImage*>(m_layers[srcLayerIdx]);
      LayerImage* dstLayer = static_cast<LayerImage*>(m_layers[dstLayerIdx]);
      color_t bgcolor = app_get_color_to_clear_layer(dstLayer);

      switch (op) {
        case Timeline::kMove: api.moveCel(m_sprite, srcLayer, dstLayer, srcFrame, dstFrame, bgcolor); break;
        case Timeline::kCopy: api.copyCel(m_sprite, srcLayer, dstLayer, srcFrame, dstFrame, bgcolor); break;
      }

      srcFrame += srcFrameStep;
      dstFrame += dstFrameStep;
    }
    srcLayerIdx += srcLayerStep;
    dstLayerIdx += dstLayerStep;
  }
}

void Timeline::dropFrames(DropOp op, const Range& drop)
{
  FrameNumber srcFrameBegin, srcFrameStep, srcFrameEnd;
  FrameNumber dstFrameBegin, dstFrameStep;

  // TODO Try to add the range with just one call to DocumentApi
  // methods, to avoid generating a lot of SetCelFrame undoers (see
  // DocumentApi::setCelFramePosition).

  switch (op) {

    case Timeline::kMove:
      if (drop.frameBegin() <= m_range.frameBegin()) {
        srcFrameBegin = m_range.frameBegin();
        srcFrameStep = FrameNumber(1);
        srcFrameEnd = m_range.frameEnd().next();
        dstFrameBegin = drop.frameBegin();
        dstFrameStep = FrameNumber(1);
      }
      else {
        srcFrameBegin = m_range.frameEnd();
        srcFrameStep = FrameNumber(-1);
        srcFrameEnd = m_range.frameBegin().previous();
        dstFrameBegin = drop.frameEnd();
        dstFrameStep = FrameNumber(-1);
      }
      break;

    case Timeline::kCopy:
      if (drop.frameBegin() <= m_range.frameBegin()) {
        srcFrameBegin = m_range.frameBegin();
        srcFrameStep = FrameNumber(2);
        srcFrameEnd = m_range.frameBegin().next(2*m_range.frames());
        dstFrameBegin = drop.frameBegin();
        dstFrameStep = FrameNumber(1);
      }
      else {
        srcFrameBegin = m_range.frameEnd();
        srcFrameStep = FrameNumber(-1);
        srcFrameEnd = m_range.frameBegin().previous();
        dstFrameBegin = drop.frameBegin();
        dstFrameStep = FrameNumber(0);
      }
      break;
  }

  DocumentApi api = m_document->getApi();

  for (FrameNumber srcFrame = srcFrameBegin,
                   dstFrame = dstFrameBegin; srcFrame != srcFrameEnd; ) {
    switch (op) {
      case Timeline::kMove: api.moveFrame(m_sprite, srcFrame, dstFrame); break;
      case Timeline::kCopy: api.copyFrame(m_sprite, srcFrame, dstFrame); break;
    }
      
    srcFrame += srcFrameStep;
    dstFrame += dstFrameStep;
  }
}

void Timeline::dropLayers(DropOp op, const Range& drop)
{
  if (m_layers[m_clk_layer]->isBackground()) {
    Alert::show(PACKAGE "<<You can't move the `Background' layer.||&OK");
    return;
  }

  Layer* firstLayer = m_layers[m_range.layerBegin()];
  Layer* lastLayer = m_layers[m_range.layerEnd()];
  std::vector<Layer*> layers = m_layers;

  switch (op) {

    case Timeline::kMove:
      for (int i = m_range.layerBegin(); i <= m_range.layerEnd(); ++i) {
        m_document->getApi().restackLayerAfter(
          layers[i], layers[drop.layerBegin()]);
      }
      break;

    case Timeline::kCopy:
      for (int i = m_range.layerBegin(); i <= m_range.layerEnd(); ++i) {
        m_document->getApi().duplicateLayer(
          layers[i], layers[drop.layerBegin()]);
      }
      break;
  }
}

Timeline::Range Timeline::getDropRange() const
{
  Range drop;
  if (m_state != STATE_MOVING_RANGE)
    return drop;

  switch (m_range.type()) {

    case Range::kCels: {
      FrameNumber dx = m_hot_frame - m_clk_frame;
      int dy = m_hot_layer - m_clk_layer;
      int layerIdx;
      FrameNumber frame;

      layerIdx = dy+m_range.layerBegin();
      layerIdx = MID(0, layerIdx, (int)m_layers.size() - m_range.layers());
      frame = dx+m_range.frameBegin();
      frame = MID(FrameNumber(0), frame, m_sprite->getTotalFrames() - m_range.frames());

      drop.startRange(layerIdx, frame, m_range.type());
      drop.endRange(
        layerIdx+m_range.layers()-1,
        (frame+m_range.frames()).previous());
      break;
    }

    case Range::kFrames: {
      FrameNumber frame = m_hot_frame;
      if (frame > m_range.frameBegin() && frame <= m_range.frameEnd())
        frame = m_range.frameBegin();

      int layerIdx = getLayerIndex(m_layer);
      drop.startRange(layerIdx, frame, m_range.type());
      drop.endRange(layerIdx, frame);
      break;
    }

    case Range::kLayers: {
      int layer = m_hot_layer;
      if (layer > m_range.layerBegin() && layer <= m_range.layerEnd())
        layer = m_range.layerBegin();

      drop.startRange(layer, m_frame, m_range.type());
      drop.endRange(layer, m_frame);
      break;
    }
  }

  return drop;
}

void Timeline::Range::startRange(int layer, FrameNumber frame, Type type)
{
  m_type = type;
  m_layerBegin = m_layerEnd = layer;
  m_frameBegin = m_frameEnd = frame;
}

void Timeline::Range::endRange(int layer, FrameNumber frame)
{
  ASSERT(enabled());
  m_layerEnd = layer;
  m_frameEnd = frame;
}

void Timeline::Range::disableRange()
{
  m_type = kNone;
}

bool Timeline::Range::inRange(int layer) const
{
  if (enabled())
    return (layer >= layerBegin() && layer <= layerEnd());
  else
    return false;
}

bool Timeline::Range::inRange(FrameNumber frame) const
{
  if (enabled())
    return (frame >= frameBegin() && frame <= frameEnd());
  else
    return false;
}

bool Timeline::Range::inRange(int layer, FrameNumber frame) const
{
  return inRange(layer) && inRange(frame);
}

void Timeline::Range::setLayers(int layers)
{
  if (m_layerBegin <= m_layerEnd) m_layerEnd = m_layerBegin + layers - 1;
  else m_layerBegin = m_layerEnd + layers - 1;
}

void Timeline::Range::setFrames(FrameNumber frames)
{
  if (m_frameBegin <= m_frameEnd) m_frameEnd = (m_frameBegin + frames).previous();
  else m_frameBegin = (m_frameEnd + frames).previous();
}

void Timeline::Range::displace(int layerDelta, FrameNumber frameDelta)
{
  m_layerBegin += layerDelta;
  m_layerEnd += layerDelta;
  m_frameBegin += frameDelta;
  m_frameEnd += frameDelta;
}

} // namespace app
