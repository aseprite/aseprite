/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include <allegro.h>

#include "ui/ui.h"

#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/ui/editor/editor.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/sprite.h"

namespace app {

class PlayAnimationCommand : public Command {
public:
  PlayAnimationCommand();
  Command* clone() { return new PlayAnimationCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

static int speed_timer;

static void speed_timer_callback()
{
  speed_timer++;
}

END_OF_STATIC_FUNCTION(speed_timer_callback);

PlayAnimationCommand::PlayAnimationCommand()
  : Command("PlayAnimation",
            "Play Animation",
            CmdUIOnlyFlag)
{
}

bool PlayAnimationCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void PlayAnimationCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  int msecs;
  bool done = false;
  IDocumentSettings* docSettings = context->getSettings()->getDocumentSettings(document);
  bool onionskin_state = docSettings->getUseOnionskin();
  Palette *oldpal, *newpal;
  PALETTE rgbpal;

  if (sprite->getTotalFrames() < 2)
    return;

  // desactivate the onionskin
  docSettings->setUseOnionskin(false);

  ui::jmouse_hide();

  FrameNumber oldFrame = current_editor->getFrame();

  LOCK_VARIABLE(speed_timer);
  LOCK_FUNCTION(speed_timer_callback);

  clear_keybuf();

  // Clear all the screen
  clear_bitmap(ui::ji_screen);

  // Clear extras (e.g. pen preview)
  document->destroyExtraCel();

  // Do animation
  oldpal = NULL;
  speed_timer = 0;
  while (!done) {
    msecs = sprite->getFrameDuration(current_editor->getFrame());
    install_int_ex(speed_timer_callback, MSEC_TO_TIMER(msecs));

    newpal = sprite->getPalette(current_editor->getFrame());
    if (oldpal != newpal) {
      newpal->toAllegro(rgbpal);
      set_palette(rgbpal);
      oldpal = newpal;
    }

    current_editor->drawSpriteClipped
      (gfx::Region(gfx::Rect(0, 0, sprite->getWidth(), sprite->getHeight())));

    ui::dirty_display_flag = true;

    do {
      poll_mouse();
      poll_keyboard();
      if (keypressed() || mouse_b)
        done = true;
      gui_feedback();
    } while (!done && (speed_timer <= 0));

    if (!done) {
      FrameNumber frame = current_editor->getFrame().next();
      if (frame > sprite->getLastFrame())
        frame = FrameNumber(0);
      current_editor->setFrame(frame);

      speed_timer--;
    }
    gui_feedback();
  }

  // Restore onionskin flag
  docSettings->setUseOnionskin(onionskin_state);

  // If right-click or ESC
  if (mouse_b == 2 || (keypressed() && (readkey()>>8) == KEY_ESC)) {
    // Return to the old frame position
    current_editor->setFrame(oldFrame);
  }

  // Refresh all
  newpal = sprite->getPalette(current_editor->getFrame());
  set_current_palette(newpal, true);
  ui::Manager::getDefault()->invalidate();
  gui_feedback();

  while (mouse_b)
    poll_mouse();

  clear_keybuf();
  remove_int(speed_timer_callback);

  ui::jmouse_show();
}

Command* CommandFactory::createPlayAnimationCommand()
{
  return new PlayAnimationCommand;
}

} // namespace app
