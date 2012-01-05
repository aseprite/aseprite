/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "gui/gui.h"

#include "commands/command.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "widgets/editor/editor.h"
#include "document_wrappers.h"

//////////////////////////////////////////////////////////////////////
// play_animation

class PlayAnimationCommand : public Command
{
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
  ActiveDocumentWriter document(context);
  Sprite* sprite(document->getSprite());
  int old_frame, msecs;
  bool done = false;
  bool onionskin_state = context->getSettings()->getUseOnionskin();
  Palette *oldpal, *newpal;
  PALETTE rgbpal;

  if (sprite->getTotalFrames() < 2)
    return;

  // desactivate the onionskin
  context->getSettings()->setUseOnionskin(false);

  jmouse_hide();

  old_frame = sprite->getCurrentFrame();

  LOCK_VARIABLE(speed_timer);
  LOCK_FUNCTION(speed_timer_callback);

  clear_keybuf();

  // Clear all the screen
  clear_bitmap(ji_screen);

  // Clear extras (e.g. pen preview)
  document->destroyExtraCel();

  // Do animation
  oldpal = NULL;
  speed_timer = 0;
  while (!done) {
    msecs = sprite->getFrameDuration(sprite->getCurrentFrame());
    install_int_ex(speed_timer_callback, MSEC_TO_TIMER(msecs));

    newpal = sprite->getPalette(sprite->getCurrentFrame());
    if (oldpal != newpal) {
      newpal->toAllegro(rgbpal);
      set_palette(rgbpal);
      oldpal = newpal;
    }

    current_editor->drawSpriteSafe(0, 0, sprite->getWidth(), sprite->getHeight());

    do {
      poll_mouse();
      poll_keyboard();
      if (keypressed() || mouse_b)
        done = true;
      gui_feedback();
    } while (!done && (speed_timer <= 0));

    if (!done) {
      int frame = sprite->getCurrentFrame()+1;
      if (frame >= sprite->getTotalFrames())
        frame = 0;
      sprite->setCurrentFrame(frame);

      speed_timer--;
    }
    gui_feedback();
  }

  // restore onionskin flag
  context->getSettings()->setUseOnionskin(onionskin_state);

  /* if right-click or ESC */
  if (mouse_b == 2 || (keypressed() && (readkey()>>8) == KEY_ESC))
    /* return to the old frame position */
    sprite->setCurrentFrame(old_frame);

  /* refresh all */
  newpal = sprite->getPalette(sprite->getCurrentFrame());
  set_current_palette(newpal, true);
  jmanager_refresh_screen();
  gui_feedback();

  while (mouse_b)
    poll_mouse();

  clear_keybuf();
  remove_int(speed_timer_callback);

  jmouse_show();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createPlayAnimationCommand()
{
  return new PlayAnimationCommand;
}
