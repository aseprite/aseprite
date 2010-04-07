/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "undoable.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// frame_properties

class FramePropertiesCommand : public Command
{
public:
  FramePropertiesCommand();
  Command* clone() { return new FramePropertiesCommand(*this); }

protected:
  bool enabled(Context* context);
  void execute(Context* context);
};

class SpriteReader;
void dialogs_frame_length(const SpriteReader& sprite, int sprite_frame);

FramePropertiesCommand::FramePropertiesCommand()
  : Command("frame_properties",
	    "Frame Properties",
	    CmdUIOnlyFlag)
{
}

bool FramePropertiesCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void FramePropertiesCommand::execute(Context* context)
{
  const CurrentSpriteReader sprite(context);
  dialogs_frame_length(sprite, sprite->getCurrentFrame());
}

/* if sprite_frame < 0, set the frame length of all frames */
void dialogs_frame_length(const SpriteReader& sprite, int sprite_frame)
{
  JWidget frame, frlen, ok;
  char buf[64];

  FramePtr window(load_widget("frame_duration.xml", "frame_duration"));
  get_widgets(window,
	      "frame", &frame,
	      "frlen", &frlen,
	      "ok", &ok, NULL);

  if (sprite_frame < 0)
    strcpy(buf, "All");
  else
    sprintf(buf, "%d", sprite_frame+1);
  frame->setText(buf);

  frlen->setTextf("%d", sprite->getFrameDuration(sprite->getCurrentFrame()));

  window->open_window_fg();
  if (window->get_killer() == ok) {
    int num = strtol(frlen->getText(), NULL, 10);

    if (sprite_frame < 0) {
      if (jalert("Warning"
		 "<<Do you want to change the duration of all frames?"
		 "||&Yes||&No") == 1) {
	SpriteWriter sprite_writer(sprite);
	Undoable undoable(sprite_writer, "Constant Frame-Rate");
	undoable.set_constant_frame_rate(num);
	undoable.commit();
      }
    }
    else {
      SpriteWriter sprite_writer(sprite);
      Undoable undoable(sprite_writer, "Frame Duration");
      undoable.set_frame_duration(sprite_frame, num);
      undoable.commit();
    }
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_frame_properties_command()
{
  return new FramePropertiesCommand;
}
