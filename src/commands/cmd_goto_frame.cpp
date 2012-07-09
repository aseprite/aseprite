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

#include "app/find_widget.h"
#include "app/load_widget.h"
#include "commands/command.h"
#include "commands/params.h"
#include "document_wrappers.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "ui/frame.h"
#include "widgets/editor/editor.h"

#include <allegro/unicode.h>

using namespace ui;

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
  : Command("GotoFirstFrame",
            "Goto First Frame",
            CmdRecordableFlag)
{
}

bool GotoFirstFrameCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void GotoFirstFrameCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite = document->getSprite();

  sprite->setCurrentFrame(FrameNumber(0));

  update_screen_for_document(document);
  current_editor->updateStatusBar();
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
  : Command("GotoPreviousFrame",
            "Goto Previous Frame",
            CmdRecordableFlag)
{
}

bool GotoPreviousFrameCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void GotoPreviousFrameCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite = document->getSprite();
  FrameNumber frame = sprite->getCurrentFrame();

  if (frame > FrameNumber(0))
    sprite->setCurrentFrame(frame.previous());
  else
    sprite->setCurrentFrame(sprite->getLastFrame());

  update_screen_for_document(document);
  current_editor->updateStatusBar();
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
  : Command("GotoNextFrame",
            "Goto Next Frame",
            CmdRecordableFlag)
{
}

bool GotoNextFrameCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void GotoNextFrameCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite = document->getSprite();
  FrameNumber frame = sprite->getCurrentFrame();

  if (frame < sprite->getLastFrame())
    sprite->setCurrentFrame(frame.next());
  else
    sprite->setCurrentFrame(FrameNumber(0));

  update_screen_for_document(document);
  current_editor->updateStatusBar();
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
  : Command("GotoLastFrame",
            "Goto Last Frame",
            CmdRecordableFlag)
{
}

bool GotoLastFrameCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void GotoLastFrameCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite = document->getSprite();
  sprite->setCurrentFrame(sprite->getLastFrame());

  update_screen_for_document(document);
  current_editor->updateStatusBar();
}

//////////////////////////////////////////////////////////////////////
// goto_frame

class GotoFrameCommand : public Command

{
public:
  GotoFrameCommand();
  Command* clone() { return new GotoFrameCommand(*this); }

protected:
  void onLoadParams(Params* params) OVERRIDE;
  bool onEnabled(Context* context);
  void onExecute(Context* context);

private:
  // The frame to go. 0 is "show the UI dialog", another value is the
  // frame (1 is the first name for the user).
  int m_frame;
};

GotoFrameCommand::GotoFrameCommand()
  : Command("GotoFrame",
            "Goto Frame",
            CmdRecordableFlag)
  , m_frame(0)
{
}

void GotoFrameCommand::onLoadParams(Params* params)
{
  std::string frame = params->get("frame");
  if (!frame.empty()) m_frame = ustrtol(frame.c_str(), NULL, 10);
  else m_frame = 0;
}

bool GotoFrameCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void GotoFrameCommand::onExecute(Context* context)
{
  if (m_frame == 0 && context->isUiAvailable()) {
    UniquePtr<Frame> window(app::load_widget<Frame>("goto_frame.xml", "goto_frame"));
    Widget* frame = app::find_widget<Widget>(window, "frame");
    Widget* ok = app::find_widget<Widget>(window, "ok");

    frame->setTextf("%d", context->getActiveDocument()->getSprite()->getCurrentFrame()+1);

    window->open_window_fg();
    if (window->get_killer() != ok)
      return;

    m_frame = strtol(frame->getText(), NULL, 10);
  }

  ActiveDocumentWriter document(context);
  Sprite* sprite = document->getSprite();
  FrameNumber newFrame(MID(0, m_frame-1, sprite->getLastFrame()));

  sprite->setCurrentFrame(newFrame);

  update_screen_for_document(document);
  current_editor->updateStatusBar();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createGotoFirstFrameCommand()
{
  return new GotoFirstFrameCommand;
}

Command* CommandFactory::createGotoPreviousFrameCommand()
{
  return new GotoPreviousFrameCommand;
}

Command* CommandFactory::createGotoNextFrameCommand()
{
  return new GotoNextFrameCommand;
}

Command* CommandFactory::createGotoLastFrameCommand()
{
  return new GotoLastFrameCommand;
}

Command* CommandFactory::createGotoFrameCommand()
{
  return new GotoFrameCommand;
}
