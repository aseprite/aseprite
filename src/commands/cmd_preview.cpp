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


#include <allegro.h>

#include "ui/gui.h"

#include "app.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "document_wrappers.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "util/render.h"
#include "widgets/editor/editor.h"
#include "widgets/status_bar.h"

using namespace ui;

#define PREVIEW_TILED           1
#define PREVIEW_FIT_ON_SCREEN   2

//////////////////////////////////////////////////////////////////////
// base class

class PreviewCommand : public Command
{
public:
  PreviewCommand();
  Command* clone() { return new PreviewCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

PreviewCommand::PreviewCommand()
  : Command("Preview",
            "Preview",
            CmdUIOnlyFlag)
{
}

bool PreviewCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}


/**
 * Shows the sprite using the complete screen.
 */
void PreviewCommand::onExecute(Context* context)
{
  Editor* editor = current_editor;

  // Cancel operation if current editor does not have a sprite
  if (!editor || !editor->getSprite())
    return;

  // Do not use DocumentWriter (do not lock the document) because we
  // will call other sub-commands (e.g. previous frame, next frame,
  // etc.).
  Document* document = editor->getDocument();
  Sprite* sprite = document->getSprite();
  const Palette* pal = sprite->getCurrentPalette();
  View* view = View::getView(editor);
  int u, v, x, y;
  int index_bg_color = -1;
  TiledMode tiled = context->getSettings()->getTiledMode();

  // Free mouse
  editor->getManager()->freeMouse();

  // Clear extras (e.g. pen preview)
  document->destroyExtraCel();

  gfx::Rect vp = view->getViewportBounds();
  gfx::Point scroll = view->getViewScroll();

  int old_mouse_x = jmouse_x(0);
  int old_mouse_y = jmouse_y(0);

  jmouse_set_cursor(JI_CURSOR_NULL);
  jmouse_set_position(JI_SCREEN_W/2, JI_SCREEN_H/2);

  int pos_x = - scroll.x + vp.x + editor->getOffsetX();
  int pos_y = - scroll.y + vp.y + editor->getOffsetY();
  int delta_x = 0;
  int delta_y = 0;

  int zoom = editor->getZoom();
  int w = sprite->getWidth() << zoom;
  int h = sprite->getHeight() << zoom;

  bool redraw = true;

  // Render the sprite
  Image* render = NULL;
  Image* doublebuf = Image::create(IMAGE_RGB, JI_SCREEN_W, JI_SCREEN_H);

  do {
    // Update scroll
    if (jmouse_poll()) {
      delta_x += (jmouse_x(0) - JI_SCREEN_W/2);
      delta_y += (jmouse_y(0) - JI_SCREEN_H/2);
      jmouse_set_position(JI_SCREEN_W/2, JI_SCREEN_H/2);
      jmouse_poll();

      redraw = true;
    }

    // Render sprite and leave the result in 'render' variable
    if (render == NULL) {
      render = RenderEngine::renderSprite(document, sprite,
                                          0, 0, sprite->getWidth(), sprite->getHeight(),
                                          sprite->getCurrentFrame(), 0, false);
    }

    // Redraw the screen
    if (redraw) {
      redraw = false;

      x = pos_x + ((delta_x >> zoom) << zoom);
      y = pos_y + ((delta_y >> zoom) << zoom);

      if (tiled & TILED_X_AXIS) x = SGN(x) * (ABS(x)%w);
      if (tiled & TILED_Y_AXIS) y = SGN(y) * (ABS(y)%h);

      if (index_bg_color == -1)
        RenderEngine::renderCheckedBackground(doublebuf, -pos_x, -pos_y, zoom);
      else
        image_clear(doublebuf, pal->getEntry(index_bg_color));

      switch (tiled) {
        case TILED_NONE:
          RenderEngine::renderImage(doublebuf, render, pal, x, y, zoom);
          break;
        case TILED_X_AXIS:
          for (u=x-w; u<JI_SCREEN_W+w; u+=w)
            RenderEngine::renderImage(doublebuf, render, pal, u, y, zoom);
          break;
        case TILED_Y_AXIS:
          for (v=y-h; v<JI_SCREEN_H+h; v+=h)
            RenderEngine::renderImage(doublebuf, render, pal, x, v, zoom);
          break;
        case TILED_BOTH:
          for (v=y-h; v<JI_SCREEN_H+h; v+=h)
            for (u=x-w; u<JI_SCREEN_W+w; u+=w)
              RenderEngine::renderImage(doublebuf, render, pal, u, v, zoom);
          break;
      }

      image_to_allegro(doublebuf, ji_screen, 0, 0, pal);
    }

    // It is necessary in case ji_screen is double-bufferred
    gui_feedback();

    if (keypressed()) {
      int readkey_value = readkey();
      Message* msg = jmessage_new_key_related(JM_KEYPRESSED, readkey_value);
      Command* command = NULL;
      get_command_from_key_message(msg, &command, NULL);
      jmessage_free(msg);

      // Change frame
      if (command != NULL &&
          (strcmp(command->short_name(), CommandId::GotoFirstFrame) == 0 ||
           strcmp(command->short_name(), CommandId::GotoPreviousFrame) == 0 ||
           strcmp(command->short_name(), CommandId::GotoNextFrame) == 0 ||
           strcmp(command->short_name(), CommandId::GotoLastFrame) == 0)) {
        // Execute the command
        context->executeCommand(command);

        // Redraw
        redraw = true;

        // Re-render
        if (render)
          image_free(render);
        render = NULL;
      }
      // Play the animation
      else if (command != NULL &&
               strcmp(command->short_name(), CommandId::PlayAnimation) == 0) {
        // TODO
      }
      // Change background color
      else if ((readkey_value>>8) == KEY_PLUS_PAD ||
               (readkey_value&0xff) == '+') {
        if (index_bg_color == -1 ||
            index_bg_color < pal->size()-1) {
          ++index_bg_color;
          redraw = true;
        }
      }
      else if ((readkey_value>>8) == KEY_MINUS_PAD ||
               (readkey_value&0xff) == '-') {
        if (index_bg_color >= 0) {
          --index_bg_color;     // can be -1 which is the checked background
          redraw = true;
        }
      }
      else
        break;
    }
  } while (!jmouse_b(0));

  if (render) image_free(render);
  if (doublebuf) image_free(doublebuf);

  do {
    jmouse_poll();
    gui_feedback();
  } while (jmouse_b(0));
  clear_keybuf();

  jmouse_set_position(old_mouse_x, old_mouse_y);
  jmouse_set_cursor(JI_CURSOR_NORMAL);

  ui::Manager::getDefault()->invalidate();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createPreviewCommand()
{
  return new PreviewCommand;
}
