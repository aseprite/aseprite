/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "app_menus.h"
#include "base/compiler_specific.h"
#include "base/memory.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "commands/params.h"
#include "console.h"
#include "document.h"
#include "document_undo.h"
#include "document_wrappers.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "raster/raster.h"
#include "skin/skin_theme.h"
#include "ui/gui.h"
#include "ui_context.h"
#include "undo_transaction.h"
#include "util/celmove.h"
#include "util/thmbnail.h"

#include <allegro.h>

using namespace gfx;
using namespace ui;

/*
   Animator Editor

                        Frames ...

   --------------------+-----+-----+-----+---
            Layers     |msecs|msecs|msecs|...
   --------------------+-----+-----+-----+---
    [1] [2] Layer 1    | Cel | Cel | Cel |...
   --------------------+-----+-----+-----+---
    [1] [2] Background | Cel | Cel | Cel |...
   --------------------+-----+-----+-----+---

   [1] Eye-icon
   [2] Padlock-icon
 */

// Size of the thumbnail in the screen (width x height), the really
// size of the thumbnail bitmap is specified in the
// 'generate_thumbnail' routine.
#define THUMBSIZE       (32*jguiscale())

// Height of the headers.
#define HDRSIZE         (3 + text_height(this->getFont())*2 + 3 + 3)

// Width of the frames.
#define FRMSIZE         (3 + THUMBSIZE + 3)

// Height of the layers.
#define LAYSIZE         (3 + MAX(text_height(this->getFont()), THUMBSIZE) + 4)

// Space between icons and other information in the layer.
#define ICONSEP         (2*jguiscale())

// Space between the icon-bitmap and the edge of the surrounding button.
#define ICONBORDER      (4*jguiscale())

enum {
  A_PART_NOTHING,
  A_PART_SEPARATOR,
  A_PART_HEADER_LAYER,
  A_PART_HEADER_FRAME,
  A_PART_LAYER,
  A_PART_LAYER_EYE_ICON,
  A_PART_LAYER_LOCK_ICON,
  A_PART_CEL
};

class AnimationEditor : public Widget
{
public:
  enum State {
    STATE_STANDBY,
    STATE_SCROLLING,
    STATE_MOVING_SEPARATOR,
    STATE_MOVING_LAYER,
    STATE_MOVING_CEL,
    STATE_MOVING_FRAME,
  };

  AnimationEditor(const Document* document, const Sprite* sprite);
  ~AnimationEditor();

  void centerCurrentCel();
  State getState() const { return m_state; }

protected:
  bool onProcessMessage(Message* msg) OVERRIDE;

private:
  void setCursor(int x, int y);
  void getDrawableLayers(JRect clip, int* first_layer, int* last_layer);
  void getDrawableFrames(JRect clip, FrameNumber* first_frame, FrameNumber* last_frame);
  void drawHeader(JRect clip);
  void drawHeaderFrame(JRect clip, FrameNumber frame);
  void drawHeaderPart(JRect clip, int x1, int y1, int x2, int y2,
                      bool is_hot, bool is_clk,
                      const char* line1, int align1,
                      const char* line2, int align2);
  void drawSeparator(JRect clip);
  void drawLayer(JRect clip, int layer_index);
  void drawLayerPadding();
  void drawCel(JRect clip, int layer_index, FrameNumber frame, Cel* cel);
  bool drawPart(int part, int layer, FrameNumber frame);
  void regenerateLayers();
  void hotThis(int hot_part, int hot_layer, FrameNumber hotFrame);
  void centerCel(int layer, FrameNumber frame);
  void showCel(int layer, FrameNumber frame);
  void showCurrentCel();
  void cleanClk();
  void setScroll(int x, int y, bool use_refresh_region);
  int getLayerIndex(const Layer* layer);

  const Document* m_document;
  const Sprite* m_sprite;
  State m_state;
  Layer** m_layers;
  int m_nlayers;
  int m_scroll_x;
  int m_scroll_y;
  int m_separator_x;
  int m_separator_w;
  // The 'hot' part is where the mouse is on top of
  int m_hot_part;
  int m_hot_layer;
  FrameNumber m_hot_frame;
  // The 'clk' part is where the mouse's button was pressed (maybe for a drag & drop operation)
  int m_clk_part;
  int m_clk_layer;
  FrameNumber m_clk_frame;
  // Keys
  bool m_space_pressed;
};

static AnimationEditor* current_anieditor = NULL;

static void icon_rect(BITMAP* icon_normal, BITMAP* icon_selected, int x1, int y1, int x2, int y2,
                      bool is_selected, bool is_hot, bool is_clk);

bool animation_editor_is_movingcel()
{
  return
    current_anieditor != NULL &&
    current_anieditor->getState() == AnimationEditor::STATE_MOVING_CEL;
}

// Shows the animation editor for the current sprite.
void switch_between_animation_and_sprite_editor()
{
  const Document* document = UIContext::instance()->getActiveDocument();
  const Sprite* sprite = document->getSprite();

  // Create the window & the animation-editor
  {
    UniquePtr<Window> window(new Window(true, NULL));
    AnimationEditor* anieditor = new AnimationEditor(document, sprite);
    current_anieditor = anieditor;

    window->addChild(anieditor);
    window->remap_window();

    anieditor->centerCurrentCel();

    // Show the window
    window->openWindowInForeground();

    // No more animation editor
    current_anieditor = NULL;
  }

  // Destroy thumbnails
  destroy_thumbnails();

  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// The Animation Editor

AnimationEditor::AnimationEditor(const Document* document, const Sprite* sprite)
  : Widget(JI_WIDGET)
{
  m_document = document;
  m_sprite = sprite;
  m_state = STATE_STANDBY;
  m_layers = NULL;
  m_nlayers = 0;
  m_scroll_x = 0;
  m_scroll_y = 0;
  m_separator_x = 100 * jguiscale();
  m_separator_w = 1;
  m_hot_part = A_PART_NOTHING;
  m_clk_part = A_PART_NOTHING;
  m_space_pressed = false;

  this->setFocusStop(true);

  regenerateLayers();
}

AnimationEditor::~AnimationEditor()
{
  if (m_layers)
    base_free(m_layers);
}

bool AnimationEditor::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      // This doesn't matter, the AniEditor'll use the entire screen
      // anyway.
      msg->reqsize.w = 32;
      msg->reqsize.h = 32;
      return true;

    case JM_DRAW: {
      JRect clip = &msg->draw.rect;
      int layer, first_layer, last_layer;
      FrameNumber frame, first_frame, last_frame;

      getDrawableLayers(clip, &first_layer, &last_layer);
      getDrawableFrames(clip, &first_frame, &last_frame);

      // Draw the header for layers.
      drawHeader(clip);

      // Draw the header for each visible frame.
      for (frame=first_frame; frame<=last_frame; ++frame)
        drawHeaderFrame(clip, frame);

      // Draw the separator.
      drawSeparator(clip);

      // Draw each visible layer.
      for (layer=first_layer; layer<=last_layer; layer++) {
        drawLayer(clip, layer);

        // Get the first CelIterator to be drawn (it is the first cel with cel->frame >= first_frame)
        CelIterator it, end;
        Layer* layerPtr = m_layers[layer];
        if (layerPtr->is_image()) {
          it = static_cast<LayerImage*>(layerPtr)->getCelBegin();
          end = static_cast<LayerImage*>(layerPtr)->getCelEnd();
          for (; it != end && (*it)->getFrame() < first_frame; ++it)
            ;
        }

        // Draw every visible cel for each layer.
        for (frame=first_frame; frame<=last_frame; ++frame) {
          Cel* cel = (layerPtr->is_image() && it != end && (*it)->getFrame() == frame ? *it: NULL);

          drawCel(clip, layer, frame, cel);

          if (cel)
            ++it;               // Go to next cel
        }
      }

      drawLayerPadding();

      return true;
    }

    case JM_TIMER:
      break;

    case JM_MOUSEENTER:
      if (key[KEY_SPACE]) m_space_pressed = true;
      break;

    case JM_MOUSELEAVE:
      if (m_space_pressed) m_space_pressed = false;
      break;

    case JM_BUTTONPRESSED:
      if (msg->mouse.middle || m_space_pressed) {
        captureMouse();
        m_state = STATE_SCROLLING;
        return true;
      }

      // Clicked-part = hot-part.
      m_clk_part = m_hot_part;
      m_clk_layer = m_hot_layer;
      m_clk_frame = m_hot_frame;

      switch (m_hot_part) {
        case A_PART_NOTHING:
          // Do nothing.
          break;
        case A_PART_SEPARATOR:
          captureMouse();
          m_state = STATE_MOVING_SEPARATOR;
          break;
        case A_PART_HEADER_LAYER:
          // Do nothing.
          break;
        case A_PART_HEADER_FRAME:
          {
            const DocumentReader document(const_cast<Document*>(m_document));
            DocumentWriter document_writer(document);
            document_writer->getSprite()->setCurrentFrame(m_clk_frame);
          }
          invalidate(); // TODO Replace this by redrawing old current frame and new current frame
          captureMouse();
          m_state = STATE_MOVING_FRAME;
          break;
        case A_PART_LAYER: {
          const DocumentReader document(const_cast<Document*>(m_document));
          const Sprite* sprite = m_sprite;
          int old_layer = getLayerIndex(sprite->getCurrentLayer());
          FrameNumber frame = m_sprite->getCurrentFrame();

          // Did the user select another layer?
          if (old_layer != m_clk_layer) {
            {
              DocumentWriter document_writer(document);
              Sprite* sprite_writer = const_cast<Sprite*>(m_sprite);
              sprite_writer->setCurrentLayer(m_layers[m_clk_layer]);
            }

            jmouse_hide();
            // Redraw the old & new selected cel.
            drawPart(A_PART_CEL, old_layer, frame);
            drawPart(A_PART_CEL, m_clk_layer, frame);
            // Redraw the old selected layer.
            drawPart(A_PART_LAYER, old_layer, frame);
            jmouse_show();
          }

          // Change the scroll to show the new selected cel.
          showCel(m_clk_layer, sprite->getCurrentFrame());
          captureMouse();
          m_state = STATE_MOVING_LAYER;
          break;
        }
        case A_PART_LAYER_EYE_ICON:
          captureMouse();
          break;
        case A_PART_LAYER_LOCK_ICON:
          captureMouse();
          break;
        case A_PART_CEL: {
          const DocumentReader document(const_cast<Document*>(m_document));
          const Sprite* sprite = document->getSprite();
          int old_layer = getLayerIndex(sprite->getCurrentLayer());
          FrameNumber old_frame = sprite->getCurrentFrame();

          // Select the new clicked-part.
          if (old_layer != m_clk_layer ||
              old_frame != m_clk_frame) {
            {
              DocumentWriter document_writer(document);
              Sprite* sprite_writer = document_writer->getSprite();
              sprite_writer->setCurrentLayer(m_layers[m_clk_layer]);
              sprite_writer->setCurrentFrame(m_clk_frame);
            }

            jmouse_hide();
            // Redraw the old & new selected layer.
            if (old_layer != m_clk_layer) {
              drawPart(A_PART_LAYER, old_layer, old_frame);
              drawPart(A_PART_LAYER, m_clk_layer, m_clk_frame);
            }
            // Redraw the old selected cel.
            drawPart(A_PART_CEL, old_layer, old_frame);
            jmouse_show();
          }

          // Change the scroll to show the new selected cel.
          showCel(m_clk_layer, sprite->getCurrentFrame());

          // Capture the mouse (to move the cel).
          captureMouse();
          m_state = STATE_MOVING_CEL;
          break;
        }
      }

      // Redraw the new selected part (header, layer or cel).
      jmouse_hide();
      drawPart(m_clk_part,
               m_clk_layer,
               m_clk_frame);
      jmouse_show();
      break;

    case JM_MOTION: {
      int hot_part = A_PART_NOTHING;
      int hot_layer = -1;
      FrameNumber hot_frame(-1);
      int mx = msg->mouse.x - rc->x1;
      int my = msg->mouse.y - rc->y1;

      if (hasCapture()) {
        if (m_state == STATE_SCROLLING) {
          setScroll(m_scroll_x+jmouse_x(1)-jmouse_x(0),
                    m_scroll_y+jmouse_y(1)-jmouse_y(0), true);

          jmouse_control_infinite_scroll(getBounds());
          return true;
        }
        // If the mouse pressed the mouse's button in the separator,
        // we shouldn't change the hot (so the separator can be
        // tracked to the mouse's released).
        else if (m_clk_part == A_PART_SEPARATOR) {
          hot_part = m_clk_part;
          m_separator_x = mx;
          invalidate();
          return true;
        }
      }

      // Is the mouse on the separator.
      if (mx > m_separator_x-4 && mx < m_separator_x+4)  {
        hot_part = A_PART_SEPARATOR;
      }
      // Is the mouse on the headers?
      else if (my < HDRSIZE) {
        // Is on the layers' header?
        if (mx < m_separator_x)
          hot_part = A_PART_HEADER_LAYER;
        // Is on a frame header?
        else {
          hot_part = A_PART_HEADER_FRAME;
          hot_frame = FrameNumber((mx
                                   - m_separator_x
                                   - m_separator_w
                                   + m_scroll_x) / FRMSIZE);
        }
      }
      else {
        hot_layer = (my
                     - HDRSIZE
                     + m_scroll_y) / LAYSIZE;

        // Is the mouse on a layer's label?
        if (mx < m_separator_x) {
          SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
          BITMAP* icon1 = theme->get_part(PART_LAYER_VISIBLE);
          BITMAP* icon2 = theme->get_part(PART_LAYER_EDITABLE);
          int x1, y1, x2, y2, y_mid;

          x1 = 0;
          y1 = HDRSIZE + LAYSIZE*hot_layer - m_scroll_y;
          x2 = x1 + m_separator_x - 1;
          y2 = y1 + LAYSIZE - 1;
          y_mid = (y1+y2) / 2;

          if (mx >= x1+2 &&
              mx <= x1+ICONSEP+ICONBORDER+icon1->w+ICONBORDER-1 &&
              my >= y_mid-icon1->h/2-ICONBORDER &&
              my <= y_mid+icon1->h/2+ICONBORDER) {
            hot_part = A_PART_LAYER_EYE_ICON;
          }
          else if (mx >= x1+ICONSEP+ICONBORDER+icon1->w+ICONBORDER &&
                   mx <= x1+ICONSEP+ICONBORDER+icon1->w+ICONBORDER+ICONBORDER+icon2->w+ICONBORDER-1 &&
                   my >= y_mid-icon2->h/2-ICONBORDER &&
                   my <= y_mid+icon2->h/2+ICONBORDER) {
            hot_part = A_PART_LAYER_LOCK_ICON;
          }
          else
            hot_part = A_PART_LAYER;
        }
        else {
          hot_part = A_PART_CEL;
          hot_frame = FrameNumber((mx
                                   - m_separator_x
                                   - m_separator_w
                                   + m_scroll_x) / FRMSIZE);
        }
      }

      // Set the new 'hot' thing.
      hotThis(hot_part, hot_layer, hot_frame);
      return true;
    }

    case JM_BUTTONRELEASED:
      if (hasCapture()) {
        releaseMouse();

        if (m_state == STATE_SCROLLING) {
          m_state = STATE_STANDBY;
          return true;
        }

        switch (m_hot_part) {
          case A_PART_NOTHING:
            // Do nothing.
            break;
          case A_PART_SEPARATOR:
            // Do nothing.
            break;
          case A_PART_HEADER_LAYER:
            // Do nothing.
            break;
          case A_PART_HEADER_FRAME:
            // Show the frame pop-up menu.
            if (msg->mouse.right) {
              if (m_clk_frame == m_hot_frame) {
                Menu* popup_menu = AppMenus::instance()->getFramePopupMenu();
                if (popup_menu != NULL) {
                  popup_menu->showPopup(msg->mouse.x, msg->mouse.y);

                  destroy_thumbnails();
                  invalidate();
                }
              }
            }
            // Show the frame's properties dialog.
            else if (msg->mouse.left) {
              if (m_clk_frame == m_hot_frame) {
                // Execute FrameProperties command for current frame.
                Command* command = CommandsModule::instance()
                  ->getCommandByName(CommandId::FrameProperties);
                Params params;
                params.set("frame", "current");

                UIContext::instance()->executeCommand(command, &params);
              }
              else {
                const DocumentReader document(const_cast<Document*>(m_document));
                const Sprite* sprite = m_sprite;

                if (m_hot_frame >= 0 &&
                    m_hot_frame < sprite->getTotalFrames() &&
                    m_hot_frame != m_clk_frame+1) {
                  {
                    DocumentWriter document_writer(document);
                    UndoTransaction undoTransaction(document_writer, "Move Frame");
                    undoTransaction.moveFrameBefore(m_clk_frame, m_hot_frame);
                    undoTransaction.commit();
                  }
                  invalidate();
                }
              }
            }
            break;
          case A_PART_LAYER:
            // Show the layer pop-up menu.
            if (msg->mouse.right) {
              if (m_clk_layer == m_hot_layer) {
                Menu* popup_menu = AppMenus::instance()->getLayerPopupMenu();
                if (popup_menu != NULL) {
                  popup_menu->showPopup(msg->mouse.x, msg->mouse.y);

                  destroy_thumbnails();
                  invalidate();
                  regenerateLayers();
                }
              }
            }
            // Move a layer.
            else if (msg->mouse.left) {
              if (m_hot_layer >= 0 &&
                  m_hot_layer < m_nlayers &&
                  m_hot_layer != m_clk_layer &&
                  m_hot_layer != m_clk_layer+1) {
                if (!m_layers[m_clk_layer]->is_background()) {
                  // move the clicked-layer after the hot-layer
                  try {
                    const DocumentReader document(const_cast<Document*>(m_document));
                    DocumentWriter document_writer(document);
                    Sprite* sprite_writer = const_cast<Sprite*>(m_sprite);

                    UndoTransaction undoTransaction(document_writer, "Move Layer");
                    undoTransaction.moveLayerAfter(m_layers[m_clk_layer],
                                                   m_layers[m_hot_layer]);
                    undoTransaction.commit();

                    // Select the new layer.
                    sprite_writer->setCurrentLayer(m_layers[m_clk_layer]);
                  }
                  catch (LockedDocumentException& e) {
                    Console::showException(e);
                  }

                  invalidate();
                  regenerateLayers();
                }
                else {
                  Alert::show(PACKAGE "<<You can't move the `Background' layer.||&OK");
                }
              }
            }
            break;
          case A_PART_LAYER_EYE_ICON:
            // Hide/show layer.
            if (m_hot_layer == m_clk_layer &&
                m_hot_layer >= 0 &&
                m_hot_layer < m_nlayers) {
              Layer* layer = m_layers[m_clk_layer];
              ASSERT(layer != NULL);
              layer->set_readable(!layer->is_readable());
            }
            break;
          case A_PART_LAYER_LOCK_ICON:
            // Lock/unlock layer.
            if (m_hot_layer == m_clk_layer &&
                m_hot_layer >= 0 &&
                m_hot_layer < m_nlayers) {
              Layer* layer = m_layers[m_clk_layer];
              ASSERT(layer != NULL);
              layer->set_writable(!layer->is_writable());
            }
            break;
          case A_PART_CEL: {
            bool movement =
              m_clk_part == A_PART_CEL &&
              m_hot_part == A_PART_CEL &&
              (m_clk_layer != m_hot_layer ||
               m_clk_frame != m_hot_frame);

            if (movement) {
              set_frame_to_handle
                (// Source cel.
                 m_layers[m_clk_layer],
                 m_clk_frame,
                 // Destination cel.
                 m_layers[m_hot_layer],
                 m_hot_frame);
            }

            // Show the cel pop-up menu.
            if (msg->mouse.right) {
              Menu* popup_menu = movement ? AppMenus::instance()->getCelMovementPopupMenu():
                                            AppMenus::instance()->getCelPopupMenu();
              if (popup_menu != NULL) {
                popup_menu->showPopup(msg->mouse.x, msg->mouse.y);

                destroy_thumbnails();
                regenerateLayers();
                invalidate();
              }
            }
            // Move the cel.
            else if (msg->mouse.left) {
              if (movement) {
                {
                  const DocumentReader document(const_cast<Document*>(m_document));
                  DocumentWriter document_writer(document);
                  move_cel(document_writer);
                }

                destroy_thumbnails();
                regenerateLayers();
                invalidate();
              }
            }
            break;
          }
        }

        // Clean the clicked-part & redraw the hot-part.
        jmouse_hide();
        cleanClk();
        drawPart(m_hot_part,
                 m_hot_layer,
                 m_hot_frame);
        jmouse_show();

        // Restore the cursor.
        m_state = STATE_STANDBY;
        setCursor(msg->mouse.x, msg->mouse.y);
        return true;
      }
      break;

    case JM_KEYPRESSED: {
      Command* command = NULL;
      Params* params = NULL;
      get_command_from_key_message(msg, &command, &params);

      // Close animation editor.
      if ((command && (strcmp(command->short_name(), CommandId::FilmEditor) == 0)) ||
          (msg->key.scancode == KEY_ESC)) {
        closeWindow();
        return true;
      }

      // Undo or redo.
      if (command && (strcmp(command->short_name(), CommandId::Undo) == 0 ||
                      strcmp(command->short_name(), CommandId::Redo) == 0)) {
        if (command->isEnabled(UIContext::instance())) {
          UIContext::instance()->executeCommand(command, params);

          destroy_thumbnails();
          regenerateLayers();
          showCurrentCel();
          invalidate();
        }
        return true;
      }

      // New_frame, remove_frame, new_cel, remove_cel.
      if (command != NULL) {
        if (strcmp(command->short_name(), CommandId::NewFrame) == 0 ||
            strcmp(command->short_name(), CommandId::RemoveCel) == 0 ||
            strcmp(command->short_name(), CommandId::RemoveFrame) == 0 ||
            strcmp(command->short_name(), CommandId::GotoFirstFrame) == 0 ||
            strcmp(command->short_name(), CommandId::GotoPreviousFrame) == 0 ||
            strcmp(command->short_name(), CommandId::GotoPreviousLayer) == 0 ||
            strcmp(command->short_name(), CommandId::GotoNextFrame) == 0 ||
            strcmp(command->short_name(), CommandId::GotoNextLayer) == 0 ||
            strcmp(command->short_name(), CommandId::GotoLastFrame) == 0) {
          // execute the command
          UIContext::instance()->executeCommand(command, params);

          showCurrentCel();
          invalidate();
          return true;
        }

        if (strcmp(command->short_name(), CommandId::NewLayer) == 0 ||
            strcmp(command->short_name(), CommandId::RemoveLayer) == 0) {
          // execute the command
          UIContext::instance()->executeCommand(command);

          regenerateLayers();
          showCurrentCel();
          invalidate();
          return true;
        }
      }

      switch (msg->key.scancode) {
        case KEY_SPACE:
          m_space_pressed = true;
          setCursor(jmouse_x(0), jmouse_y(0));
          return true;
      }

      break;
    }

    case JM_KEYRELEASED:
      switch (msg->key.scancode) {

        case KEY_SPACE:
          if (m_space_pressed) {
            // We have to clear all the KEY_SPACE in buffer.
            clear_keybuf();

            m_space_pressed = false;
            setCursor(jmouse_x(0), jmouse_y(0));
            return true;
          }
          break;
      }
      break;

    case JM_WHEEL: {
      int dz = jmouse_z(1) - jmouse_z(0);
      int dx = 0;
      int dy = 0;

      if ((msg->any.shifts & KB_CTRL_FLAG) == KB_CTRL_FLAG)
        dx = dz * FRMSIZE;
      else
        dy = dz * LAYSIZE;

      if ((msg->any.shifts & KB_SHIFT_FLAG) == KB_SHIFT_FLAG) {
        dx *= 3;
        dy *= 3;
      }

      setScroll(m_scroll_x+dx,
                m_scroll_y+dy, true);
      break;
    }

    case JM_SETCURSOR:
      setCursor(msg->mouse.x, msg->mouse.y);
      return true;

  }

  return Widget::onProcessMessage(msg);
}

void AnimationEditor::setCursor(int x, int y)
{
  int mx = x - rc->x1;
//int my = y - this->rc->y1;

  // Is the mouse in the separator.
  if (mx > m_separator_x-2 && mx < m_separator_x+2)  {
    jmouse_set_cursor(JI_CURSOR_SIZE_L);
  }
  // Scrolling.
  else if (m_state == STATE_SCROLLING ||
           m_space_pressed) {
    jmouse_set_cursor(JI_CURSOR_SCROLL);
  }
  // Moving a frame.
  else if (m_state == STATE_MOVING_FRAME &&
           m_clk_part == A_PART_HEADER_FRAME &&
           m_hot_part == A_PART_HEADER_FRAME &&
           m_clk_frame != m_hot_frame) {
    jmouse_set_cursor(JI_CURSOR_MOVE);
  }
  // Moving a layer.
  else if (m_state == STATE_MOVING_LAYER &&
           m_clk_part == A_PART_LAYER &&
           m_hot_part == A_PART_LAYER &&
           m_clk_layer != m_hot_layer) {
    if (m_layers[m_clk_layer]->is_background())
      jmouse_set_cursor(JI_CURSOR_FORBIDDEN);
    else
      jmouse_set_cursor(JI_CURSOR_MOVE);
  }
  // Moving a cel.
  else if (m_state == STATE_MOVING_CEL &&
           m_clk_part == A_PART_CEL &&
           m_hot_part == A_PART_CEL &&
           (m_clk_frame != m_hot_frame ||
            m_clk_layer != m_hot_layer)) {
    jmouse_set_cursor(JI_CURSOR_MOVE);
  }
  // Normal state.
  else {
    jmouse_set_cursor(JI_CURSOR_NORMAL);
  }
}

void AnimationEditor::getDrawableLayers(JRect clip, int* first_layer, int* last_layer)
{
  *first_layer = 0;
  *last_layer = m_nlayers-1;
}

void AnimationEditor::getDrawableFrames(JRect clip, FrameNumber* first_frame, FrameNumber* last_frame)
{
  *first_frame = FrameNumber(0);
  *last_frame = m_sprite->getLastFrame();
}

void AnimationEditor::drawHeader(JRect clip)
{
  // bool is_hot = (m_hot_part == A_PART_HEADER_LAYER);
  // bool is_clk = (m_clk_part == A_PART_HEADER_LAYER);
  int x1, y1, x2, y2;

  x1 = this->rc->x1;
  y1 = this->rc->y1;
  x2 = x1 + m_separator_x - 1;
  y2 = y1 + HDRSIZE - 1;

  // Draw the header for the layers.
  drawHeaderPart(clip, x1, y1, x2, y2,
                 // is_hot, is_clk,
                 false, false,
                 "Frames >>", 1,
                 "Layers", -1);
}

void AnimationEditor::drawHeaderFrame(JRect clip, FrameNumber frame)
{
  bool is_hot = (m_hot_part == A_PART_HEADER_FRAME &&
                 m_hot_frame == frame);
  bool is_clk = (m_clk_part == A_PART_HEADER_FRAME &&
                 m_clk_frame == frame);
  int x1, y1, x2, y2;
  int cx1, cy1, cx2, cy2;
  char buf1[256];
  char buf2[256];

  get_clip_rect(ji_screen, &cx1, &cy1, &cx2, &cy2);

  x1 = this->rc->x1 + m_separator_x + m_separator_w
    + FRMSIZE*frame - m_scroll_x;
  y1 = this->rc->y1;
  x2 = x1 + FRMSIZE - 1;
  y2 = y1 + HDRSIZE - 1;

  add_clip_rect(ji_screen,
                this->rc->x1 + m_separator_x + m_separator_w,
                y1, this->rc->x2-1, y2);

  // Draw the header for the layers.
  usprintf(buf1, "%d", frame+1);
  usprintf(buf2, "%d", m_sprite->getFrameDuration(frame));
  drawHeaderPart(clip, x1, y1, x2, y2,
                 is_hot, is_clk,
                 buf1, 0,
                 buf2, 0);

  // If this header wasn't clicked but there are another frame's
  // header clicked, we have to draw some indicators to show that the
  // user can move frames.
  if (is_hot && !is_clk &&
      m_clk_part == A_PART_HEADER_FRAME) {
    rectfill(ji_screen, x1+1, y1+1, x1+4, y2-1, ji_color_selected());
  }

  // Padding in the right side.
  if (frame == m_sprite->getTotalFrames()-1) {
    if (x2+1 <= this->rc->x2-1) {
      // Right side.
      vline(ji_screen, x2+1, y1, y2, ji_color_foreground());
      if (x2+2 <= this->rc->x2-1)
        rectfill(ji_screen, x2+2, y1, this->rc->x2-1, y2, ji_color_face());
    }
  }

  set_clip_rect(ji_screen, cx1, cy1, cx2, cy2);
}

void AnimationEditor::drawHeaderPart(JRect clip, int x1, int y1, int x2, int y2,
                                     bool is_hot, bool is_clk,
                                     const char *line1, int align1,
                                     const char *line2, int align2)
{
  int x, fg, face, facelight, faceshadow;

  if ((x2 < clip->x1) || (x1 >= clip->x2) ||
      (y2 < clip->y1) || (y1 >= clip->y2))
    return;

  fg = !is_hot && is_clk ? ji_color_background(): ji_color_foreground();
  face = is_hot ? ji_color_hotface(): (is_clk ? ji_color_selected():
                                                ji_color_face());
  facelight = is_hot && is_clk ? ji_color_faceshadow(): ji_color_facelight();
  faceshadow = is_hot && is_clk ? ji_color_facelight(): ji_color_faceshadow();

  // Draw the border of this text.
  jrectedge(ji_screen, x1, y1, x2, y2, facelight, faceshadow);

  // Fill the background of the part.
  rectfill(ji_screen, x1+1, y1+1, x2-1, y2-1, face);

  // Draw the text inside this header.
  if (line1 != NULL) {
    if (align1 < 0)
      x = x1+3;
    else if (align1 == 0)
      x = (x1+x2)/2 - text_length(this->getFont(), line1)/2;
    else
      x = x2 - 3 - text_length(this->getFont(), line1);

    jdraw_text(ji_screen, this->getFont(), line1, x, y1+3,
               fg, face, true, jguiscale());
  }

  if (line2 != NULL) {
    if (align2 < 0)
      x = x1+3;
    else if (align2 == 0)
      x = (x1+x2)/2 - text_length(this->getFont(), line2)/2;
    else
      x = x2 - 3 - text_length(this->getFont(), line2);

    jdraw_text(ji_screen, this->getFont(), line2,
               x, y1+3+ji_font_get_size(this->getFont())+3,
               fg, face, true, jguiscale());
  }
}

void AnimationEditor::drawSeparator(JRect clip)
{
  bool is_hot = (m_hot_part == A_PART_SEPARATOR);
  int x1, y1, x2, y2;

  x1 = this->rc->x1 + m_separator_x;
  y1 = this->rc->y1;
  x2 = this->rc->x1 + m_separator_x + m_separator_w - 1;
  y2 = this->rc->y2 - 1;

  if ((x2 < clip->x1) || (x1 >= clip->x2) ||
      (y2 < clip->y1) || (y1 >= clip->y2))
    return;

  vline(ji_screen, x1, y1, y2, is_hot ? ji_color_selected():
                                        ji_color_foreground());
}

void AnimationEditor::drawLayer(JRect clip, int layer_index)
{
  Layer *layer = m_layers[layer_index];
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  bool selected_layer = (layer == m_sprite->getCurrentLayer());
  bool is_hot = (m_hot_part == A_PART_LAYER && m_hot_layer == layer_index);
  bool is_clk = (m_clk_part == A_PART_LAYER && m_clk_layer == layer_index);
  int bg = selected_layer ?
    ji_color_selected(): (is_hot ? ji_color_hotface():
                          (is_clk ? ji_color_selected():
                                    ji_color_face()));
  int fg = selected_layer ? ji_color_background(): ji_color_foreground();
  BITMAP* icon1 = theme->get_part(layer->is_readable() ? PART_LAYER_VISIBLE: PART_LAYER_HIDDEN);
  BITMAP* icon2 = theme->get_part(layer->is_writable() ? PART_LAYER_EDITABLE: PART_LAYER_LOCKED);
  BITMAP* icon1_selected = theme->get_part(layer->is_readable() ? PART_LAYER_VISIBLE_SELECTED: PART_LAYER_HIDDEN_SELECTED);
  BITMAP* icon2_selected = theme->get_part(layer->is_writable() ? PART_LAYER_EDITABLE_SELECTED: PART_LAYER_LOCKED_SELECTED);
  int x1, y1, x2, y2, y_mid;
  int cx1, cy1, cx2, cy2;
  int u;

  get_clip_rect(ji_screen, &cx1, &cy1, &cx2, &cy2);

  x1 = this->rc->x1;
  y1 = this->rc->y1 + HDRSIZE + LAYSIZE*layer_index - m_scroll_y;
  x2 = x1 + m_separator_x - 1;
  y2 = y1 + LAYSIZE - 1;
  y_mid = (y1+y2) / 2;

  add_clip_rect(ji_screen,
                this->rc->x1,
                this->rc->y1 + HDRSIZE,
                this->rc->x1 + m_separator_x - 1,
                this->rc->y2-1);

  if (is_hot) {
    jrectedge(ji_screen, x1, y1, x2, y2-1, ji_color_facelight(), ji_color_faceshadow());
    rectfill(ji_screen, x1+1, y1+1, x2-1, y2-2, bg);
  }
  else {
    rectfill(ji_screen, x1, y1, x2, y2-1, bg);
  }
  hline(ji_screen, x1, y2, x2, ji_color_foreground());

  // If this layer wasn't clicked but there are another layer clicked,
  // we have to draw some indicators to show that the user can move
  // layers.
  if (is_hot && !is_clk &&
      m_clk_part == A_PART_LAYER) {
    rectfill(ji_screen, x1+1, y1+1, x2-1, y1+5, ji_color_selected());
  }

  // u = the position where to put the next element (like eye-icon,
  //     lock-icon, layer-name)
  u = x1+ICONSEP;

  // Draw the eye (readable flag).
  icon_rect(icon1, icon1_selected,
            u,
            y_mid-icon1->h/2-ICONBORDER,
            u+ICONBORDER+icon1->w+ICONBORDER-1,
            y_mid-icon1->h/2+icon1->h+ICONBORDER-1,
            selected_layer,
            (m_hot_part == A_PART_LAYER_EYE_ICON &&
             m_hot_layer == layer_index),
            (m_clk_part == A_PART_LAYER_EYE_ICON &&
             m_clk_layer == layer_index));

  u += u+ICONBORDER+icon1->w+ICONBORDER;

  // Draw the padlock (writable flag).
  icon_rect(icon2, icon2_selected,
            u,
            y_mid-icon1->h/2-ICONBORDER,
            u+ICONBORDER+icon2->w+ICONBORDER-1,
            y_mid-icon1->h/2+icon1->h-1+ICONBORDER,
            selected_layer,
            (m_hot_part == A_PART_LAYER_LOCK_ICON &&
             m_hot_layer == layer_index),
            (m_clk_part == A_PART_LAYER_LOCK_ICON &&
             m_clk_layer == layer_index));

  u += ICONBORDER+icon2->w+ICONBORDER+ICONSEP;

  // Draw the layer's name.
  jdraw_text(ji_screen, this->getFont(), layer->getName().c_str(),
             u, y_mid - ji_font_get_size(this->getFont())/2,
             fg, bg, true, jguiscale());

  // The background should be underlined.
  if (layer->is_background()) {
    hline(ji_screen,
          u,
          y_mid - ji_font_get_size(this->getFont())/2 + ji_font_get_size(this->getFont()) + 1,
          u + text_length(this->getFont(), layer->getName().c_str()),
          fg);
  }

  set_clip_rect(ji_screen, cx1, cy1, cx2, cy2);
}

void AnimationEditor::drawLayerPadding()
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  int layer_index = m_nlayers-1;
  int x1, y1, x2, y2;

  x1 = this->rc->x1;
  y1 = this->rc->y1 + HDRSIZE + LAYSIZE*layer_index - m_scroll_y;
  x2 = x1 + m_separator_x - 1;
  y2 = y1 + LAYSIZE - 1;

  // Padding in the bottom side.
  if (y2+1 <= this->rc->y2-1) {
    rectfill(ji_screen, x1, y2+1, x2, this->rc->y2-1,
             theme->get_editor_face_color());
    rectfill(ji_screen,
             x2+1+m_separator_w, y2+1,
             this->rc->x2-1, this->rc->y2-1,
             theme->get_editor_face_color());
  }
}

void AnimationEditor::drawCel(JRect clip, int layer_index, FrameNumber frame, Cel* cel)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  Layer *layer = m_layers[layer_index];
  bool selected_layer = (layer == m_sprite->getCurrentLayer());
  bool is_hot = (m_hot_part == A_PART_CEL &&
                 m_hot_layer == layer_index &&
                 m_hot_frame == frame);
  bool is_clk = (m_clk_part == A_PART_CEL &&
                 m_clk_layer == layer_index &&
                 m_clk_frame == frame);
  int bg = is_hot ? ji_color_hotface():
                    ji_color_face();
  int x1, y1, x2, y2;
  int cx1, cy1, cx2, cy2;
  BITMAP *thumbnail;

  get_clip_rect(ji_screen, &cx1, &cy1, &cx2, &cy2);

  x1 = this->rc->x1 + m_separator_x + m_separator_w
    + FRMSIZE*frame - m_scroll_x;
  y1 = this->rc->y1 + HDRSIZE
    + LAYSIZE*layer_index - m_scroll_y;
  x2 = x1 + FRMSIZE - 1;
  y2 = y1 + LAYSIZE - 1;

  add_clip_rect(ji_screen,
                this->rc->x1 + m_separator_x + m_separator_w,
                this->rc->y1 + HDRSIZE,
                this->rc->x2-1,
                this->rc->y2-1);

  Rect thumbnail_rect(Point(x1+3, y1+3), Point(x2-2, y2-2));

  // Draw the box for the cel.
  if (selected_layer && frame == m_sprite->getCurrentFrame()) {
    // Current cel.
    if (is_hot)
      jrectedge(ji_screen, x1, y1, x2, y2-1,
                ji_color_facelight(), ji_color_faceshadow());
    else
      rect(ji_screen, x1, y1, x2, y2-1, ji_color_selected());
    rect(ji_screen, x1+1, y1+1, x2-1, y2-2, ji_color_selected());
    rect(ji_screen, x1+2, y1+2, x2-2, y2-3, bg);
  }
  else {
    if (is_hot) {
      jrectedge(ji_screen, x1, y1, x2, y2-1,
                ji_color_facelight(), ji_color_faceshadow());
      rectfill(ji_screen, x1+1, y1+1, x2-1, y2-2, bg);
    }
    else {
      rectfill(ji_screen, x1, y1, x2, y2-1, bg);
    }
  }
  hline(ji_screen, x1, y2, x2, ji_color_foreground());

  // Empty cel?.
  if (cel == NULL ||
      // TODO why a cel can't have an associated image?
      m_sprite->getStock()->getImage(cel->getImage()) == NULL) {

    jdraw_rectfill(thumbnail_rect, bg);
    draw_emptyset_symbol(ji_screen, thumbnail_rect, ji_color_disabled());
  }
  else {
    thumbnail = generate_thumbnail(layer, cel, m_sprite);
    if (thumbnail != NULL) {
      stretch_blit(thumbnail, ji_screen,
                   0, 0, thumbnail->w, thumbnail->h,
                   thumbnail_rect.x, thumbnail_rect.y,
                   thumbnail_rect.w, thumbnail_rect.h);
    }
  }

  // If this cel is hot and other cel was clicked, we have to draw
  // some indicators to show that the user can move cels.
  if (is_hot && !is_clk &&
      m_clk_part == A_PART_CEL) {
    rectfill(ji_screen, x1+1, y1+1, x1+FRMSIZE/3, y1+4, ji_color_selected());
    rectfill(ji_screen, x1+1, y1+5, x1+4, y1+FRMSIZE/3, ji_color_selected());

    rectfill(ji_screen, x2-FRMSIZE/3, y1+1, x2-1, y1+4, ji_color_selected());
    rectfill(ji_screen, x2-4, y1+5, x2-1, y1+FRMSIZE/3, ji_color_selected());

    rectfill(ji_screen, x1+1, y2-4, x1+FRMSIZE/3, y2-1, ji_color_selected());
    rectfill(ji_screen, x1+1, y2-FRMSIZE/3, x1+4, y2-5, ji_color_selected());

    rectfill(ji_screen, x2-FRMSIZE/3, y2-4, x2-1, y2-1, ji_color_selected());
    rectfill(ji_screen, x2-4, y2-FRMSIZE/3, x2-1, y2-5, ji_color_selected());
  }

  // Padding in the right side.
  if (frame == m_sprite->getTotalFrames()-1) {
    if (x2+1 <= this->rc->x2-1) {
      // Right side.
      vline(ji_screen, x2+1, y1, y2, ji_color_foreground());
      if (x2+2 <= this->rc->x2-1)
        rectfill(ji_screen, x2+2, y1, this->rc->x2-1, y2,
                 theme->get_editor_face_color());
    }
  }

  set_clip_rect(ji_screen, cx1, cy1, cx2, cy2);
}

bool AnimationEditor::drawPart(int part, int layer, FrameNumber frame)
{
  switch (part) {
    case A_PART_NOTHING:
      // Do nothing.
      return true;
    case A_PART_SEPARATOR:
      drawSeparator(this->rc);
      return true;
    case A_PART_HEADER_LAYER:
      drawHeader(this->rc);
      return true;
    case A_PART_HEADER_FRAME:
      if (frame >= 0 && frame < m_sprite->getTotalFrames()) {
        drawHeaderFrame(this->rc, frame);
        return true;
      }
      break;
    case A_PART_LAYER:
    case A_PART_LAYER_EYE_ICON:
    case A_PART_LAYER_LOCK_ICON:
      if (layer >= 0 && layer < m_nlayers) {
        drawLayer(this->rc, layer);
        return true;
      }
      break;
    case A_PART_CEL:
      if (layer >= 0 && layer < m_nlayers &&
          frame >= 0 && frame < m_sprite->getTotalFrames()) {
        Cel* cel = (m_layers[layer]->is_image() ? static_cast<LayerImage*>(m_layers[layer])->getCel(frame): NULL);

        drawCel(this->rc, layer, frame, cel);
        return true;
      }
      break;
  }

  return false;
}

void AnimationEditor::regenerateLayers()
{
  int c;

  if (m_layers != NULL) {
    base_free(m_layers);
    m_layers = NULL;
  }

  m_nlayers = m_sprite->countLayers();

  // Here we build an array with all the layers.
  if (m_nlayers > 0) {
    m_layers = (Layer**)base_malloc(sizeof(Layer*) * m_nlayers);

    for (c=0; c<m_nlayers; c++)
      m_layers[c] = (Layer*)m_sprite->indexToLayer(m_nlayers-c-1);
  }
}

void AnimationEditor::hotThis(int hot_part, int hot_layer, FrameNumber hot_frame)
{
  int old_hot_part;

  // If the part, layer or frame change.
  if (hot_part != m_hot_part ||
      hot_layer != m_hot_layer ||
      hot_frame != m_hot_frame) {
    jmouse_hide();

    // Clean the old 'hot' thing.
    old_hot_part = m_hot_part;
    m_hot_part = A_PART_NOTHING;

    drawPart(old_hot_part,
             m_hot_layer,
             m_hot_frame);

    // Draw the new 'hot' thing.
    m_hot_part = hot_part;
    m_hot_layer = hot_layer;
    m_hot_frame = hot_frame;

    if (!drawPart(hot_part, hot_layer, hot_frame)) {
      m_hot_part = A_PART_NOTHING;
    }

    jmouse_show();
  }
}

void AnimationEditor::centerCel(int layer, FrameNumber frame)
{
  int target_x = (this->rc->x1 + m_separator_x + m_separator_w + this->rc->x2)/2 - FRMSIZE/2;
  int target_y = (this->rc->y1 + HDRSIZE + this->rc->y2)/2 - LAYSIZE/2;
  int scroll_x = this->rc->x1 + m_separator_x + m_separator_w + FRMSIZE*frame - target_x;
  int scroll_y = this->rc->y1 + HDRSIZE + LAYSIZE*layer - target_y;

  setScroll(scroll_x, scroll_y, false);
}

void AnimationEditor::showCel(int layer, FrameNumber frame)
{
  int scroll_x, scroll_y;
  int x1, y1, x2, y2;

  x1 = this->rc->x1 + m_separator_x + m_separator_w + FRMSIZE*frame - m_scroll_x;
  y1 = this->rc->y1 + HDRSIZE + LAYSIZE*layer - m_scroll_y;
  x2 = x1 + FRMSIZE - 1;
  y2 = y1 + LAYSIZE - 1;

  scroll_x = m_scroll_x;
  scroll_y = m_scroll_y;

  if (x1 < this->rc->x1 + m_separator_x + m_separator_w) {
    scroll_x -= (this->rc->x1 + m_separator_x + m_separator_w) - (x1);
  }
  else if (x2 > this->rc->x2-1) {
    scroll_x += (x2) - (this->rc->x2-1);
  }

  if (y1 < this->rc->y1 + HDRSIZE) {
    scroll_y -= (this->rc->y1 + HDRSIZE) - (y1);
  }
  else if (y2 > this->rc->y2-1) {
    scroll_y += (y2) - (this->rc->y2-1);
  }

  if (scroll_x != m_scroll_x ||
      scroll_y != m_scroll_y)
    setScroll(scroll_x, scroll_y, true);
}

void AnimationEditor::centerCurrentCel()
{
  int layer = getLayerIndex(m_sprite->getCurrentLayer());
  if (layer >= 0)
    centerCel(layer, m_sprite->getCurrentFrame());
}

void AnimationEditor::showCurrentCel()
{
  int layer = getLayerIndex(m_sprite->getCurrentLayer());
  if (layer >= 0)
    showCel(layer, m_sprite->getCurrentFrame());
}

void AnimationEditor::cleanClk()
{
  int clk_part = m_clk_part;
  m_clk_part = A_PART_NOTHING;

  drawPart(clk_part,
           m_clk_layer,
           m_clk_frame);
}

void AnimationEditor::setScroll(int x, int y, bool use_refresh_region)
{
  int old_scroll_x = 0;
  int old_scroll_y = 0;
  int max_scroll_x;
  int max_scroll_y;
  JRegion region = NULL;

  if (use_refresh_region) {
    region = jwidget_get_drawable_region(this, JI_GDR_CUTTOPWINDOWS);
    old_scroll_x = m_scroll_x;
    old_scroll_y = m_scroll_y;
  }

  max_scroll_x = m_sprite->getTotalFrames() * FRMSIZE - jrect_w(this->rc)/2;
  max_scroll_y = m_nlayers * LAYSIZE - jrect_h(this->rc)/2;
  max_scroll_x = MAX(0, max_scroll_x);
  max_scroll_y = MAX(0, max_scroll_y);

  m_scroll_x = MID(0, x, max_scroll_x);
  m_scroll_y = MID(0, y, max_scroll_y);

  if (use_refresh_region) {
    int new_scroll_x = m_scroll_x;
    int new_scroll_y = m_scroll_y;
    int dx = old_scroll_x - new_scroll_x;
    int dy = old_scroll_y - new_scroll_y;
    JRegion reg1 = jregion_new(NULL, 0);
    JRegion reg2 = jregion_new(NULL, 0);
    JRect rect2 = jrect_new(0, 0, 0, 0);

    jmouse_hide();

    // Scroll layers.
    jrect_replace(rect2,
                  this->rc->x1,
                  this->rc->y1 + HDRSIZE,
                  this->rc->x1 + m_separator_x,
                  this->rc->y2);
    jregion_reset(reg2, rect2);
    jregion_copy(reg1, region);
    jregion_intersect(reg1, reg1, reg2);
    this->scrollRegion(reg1, 0, dy);

    // Scroll header-frame.
    jrect_replace(rect2,
                  this->rc->x1 + m_separator_x + m_separator_w,
                  this->rc->y1,
                  this->rc->x2,
                  this->rc->y1 + HDRSIZE);
    jregion_reset(reg2, rect2);
    jregion_copy(reg1, region);
    jregion_intersect(reg1, reg1, reg2);
    this->scrollRegion(reg1, dx, 0);

    // Scroll cels.
    jrect_replace(rect2,
                  this->rc->x1 + m_separator_x + m_separator_w,
                  this->rc->y1 + HDRSIZE,
                  this->rc->x2,
                  this->rc->y2);
    jregion_reset(reg2, rect2);
    jregion_copy(reg1, region);
    jregion_intersect(reg1, reg1, reg2);
    this->scrollRegion(reg1, dx, dy);

    jmouse_show();

    jregion_free(region);
    jregion_free(reg1);
    jregion_free(reg2);
    jrect_free(rect2);
  }
}

int AnimationEditor::getLayerIndex(const Layer* layer)
{
  int i;

  for (i=0; i<m_nlayers; i++)
    if (m_layers[i] == layer)
      return i;

  return -1;
}

// Auxiliary routine to draw an icon in the layer-part.
static void icon_rect(BITMAP* icon_normal, BITMAP* icon_selected, int x1, int y1, int x2, int y2,
                      bool is_selected, bool is_hot, bool is_clk)
{
  int icon_x = x1+ICONBORDER;
  int icon_y = (y1+y2)/2-icon_normal->h/2;
  int facelight = is_hot && is_clk ? ji_color_faceshadow(): ji_color_facelight();
  int faceshadow = is_hot && is_clk ? ji_color_facelight(): ji_color_faceshadow();

  if (is_hot) {
    jrectedge(ji_screen, x1, y1, x2, y2, facelight, faceshadow);

    if (!is_selected)
      rectfill(ji_screen,
               x1+1, y1+1, x2-1, y2-1,
               ji_color_hotface());
  }

  set_alpha_blender();
  if (is_selected)
    draw_trans_sprite(ji_screen, icon_selected, icon_x, icon_y);
  else
    draw_trans_sprite(ji_screen, icon_normal, icon_x, icon_y);
}
