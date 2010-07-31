/* ASE - Allegro Sprite Editor
 * Copyright (C) 2008-2010  David Capello
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

#include "commands/command.h"
#include "modules/gui.h"
#include "modules/editors.h"
#include "raster/sprite.h"
#include "widgets/editor.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// goto_first_frame

class GotoFirstFrameCommand : public Command
{
public:
  GotoFirstFrameCommand();
  Command* clone() { return new GotoFirstFrameCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

GotoFirstFrameCommand::GotoFirstFrameCommand()
  : Command("goto_first_frame",
	    "Goto First Frame",
	    CmdRecordableFlag)
{
}

bool GotoFirstFrameCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return sprite != NULL;
}

void GotoFirstFrameCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  sprite->setCurrentFrame(0);

  update_screen_for_sprite(sprite);
  current_editor->editor_update_statusbar_for_standby();
}

//////////////////////////////////////////////////////////////////////
// goto_previous_frame

class GotoPreviousFrameCommand : public Command

{
public:
  GotoPreviousFrameCommand();
  Command* clone() { return new GotoPreviousFrameCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

GotoPreviousFrameCommand::GotoPreviousFrameCommand()
  : Command("goto_previous_frame",
	    "Goto Previous Frame",
	    CmdRecordableFlag)
{
}

bool GotoPreviousFrameCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void GotoPreviousFrameCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  int frame = sprite->getCurrentFrame();

  if (frame > 0)
    sprite->setCurrentFrame(frame-1);
  else
    sprite->setCurrentFrame(sprite->getTotalFrames()-1);

  update_screen_for_sprite(sprite);
  current_editor->editor_update_statusbar_for_standby();
}

//////////////////////////////////////////////////////////////////////
// goto_next_frame

class GotoNextFrameCommand : public Command

{
public:
  GotoNextFrameCommand();
  Command* clone() { return new GotoNextFrameCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

GotoNextFrameCommand::GotoNextFrameCommand()
  : Command("goto_next_frame",
	    "Goto Next Frame",
	    CmdRecordableFlag)
{
}

bool GotoNextFrameCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void GotoNextFrameCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  int frame = sprite->getCurrentFrame();

  if (frame < sprite->getTotalFrames()-1)
    sprite->setCurrentFrame(frame+1);
  else
    sprite->setCurrentFrame(0);

  update_screen_for_sprite(sprite);
  current_editor->editor_update_statusbar_for_standby();
}

//////////////////////////////////////////////////////////////////////
// goto_last_frame

class GotoLastFrameCommand : public Command

{
public:
  GotoLastFrameCommand();
  Command* clone() { return new GotoLastFrameCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

GotoLastFrameCommand::GotoLastFrameCommand()
  : Command("goto_last_frame",
	    "Goto Last Frame",
	    CmdRecordableFlag)
{
}

bool GotoLastFrameCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return
    sprite != NULL;
}

void GotoLastFrameCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  sprite->setCurrentFrame(sprite->getTotalFrames()-1);

  update_screen_for_sprite(sprite);
  current_editor->editor_update_statusbar_for_standby();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_goto_first_frame_command()
{
  return new GotoFirstFrameCommand;
}

Command* CommandFactory::create_goto_previous_frame_command()
{
  return new GotoPreviousFrameCommand;
}

Command* CommandFactory::create_goto_next_frame_command()
{
  return new GotoNextFrameCommand;
}

Command* CommandFactory::create_goto_last_frame_command()
{
  return new GotoLastFrameCommand;
}
