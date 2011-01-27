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

#include "gui/jinete.h"

#include "base/convert_to.h"
#include "commands/command.h"
#include "commands/params.h"
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
  void onLoadParams(Params* params);
  bool onEnabled(Context* context);
  void onExecute(Context* context);

private:
  enum {
    CURRENT_FRAME = 0,
    ALL_FRAMES = -1,
  };

  // Frame to be shown. It can be ALL_FRAMES, CURRENT_FRAME, or a
  // number indicating a specific frame (1 is the first frame).
  int m_frame;
};

FramePropertiesCommand::FramePropertiesCommand()
  : Command("FrameProperties",
	    "Frame Properties",
	    CmdUIOnlyFlag)
{
}

void FramePropertiesCommand::onLoadParams(Params* params)
{
  std::string frame = params->get("frame");
  if (frame == "all") {
    m_frame = ALL_FRAMES;
  }
  else if (frame == "current") {
    m_frame = CURRENT_FRAME;
  }
  else {
    m_frame = base::convert_to<int>(frame);
  }
}

bool FramePropertiesCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return sprite != NULL;
}

void FramePropertiesCommand::onExecute(Context* context)
{
  const CurrentSpriteReader sprite(context);
  JWidget frame, frlen, ok;

  FramePtr window(load_widget("frame_duration.xml", "frame_duration"));
  get_widgets(window,
	      "frame", &frame,
	      "frlen", &frlen,
	      "ok", &ok, NULL);

  int sprite_frame = 0;
  switch (m_frame) {

    case ALL_FRAMES:
      break;

    case CURRENT_FRAME:
      sprite_frame = sprite->getCurrentFrame()+1;
      break;

    default:
      sprite_frame = m_frame;
      break;
  }

  if (m_frame == ALL_FRAMES)
    frame->setText("All");
  else
    frame->setTextf("%d", sprite_frame);

  frlen->setTextf("%d", sprite->getFrameDuration(sprite->getCurrentFrame()));

  window->open_window_fg();
  if (window->get_killer() == ok) {
    int num = strtol(frlen->getText(), NULL, 10);

    if (m_frame == ALL_FRAMES) {
      if (Alert::show("Warning"
		      "<<Do you want to change the duration of all frames?"
		      "||&Yes||&No") == 1) {
	SpriteWriter sprite_writer(sprite);
	Undoable undoable(sprite_writer, "Constant Frame-Rate");
	undoable.setConstantFrameRate(num);
	undoable.commit();
      }
    }
    else {
      SpriteWriter sprite_writer(sprite);
      Undoable undoable(sprite_writer, "Frame Duration");
      undoable.setFrameDuration(sprite_frame-1, num);
      undoable.commit();
    }
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createFramePropertiesCommand()
{
  return new FramePropertiesCommand;
}
