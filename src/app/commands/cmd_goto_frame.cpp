/* Aseprite
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

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/ui/editor/editor.h"
#include "raster/sprite.h"
#include "ui/window.h"

#include <allegro/unicode.h>

namespace app {

using namespace ui;

class GotoCommand : public Command {
protected:
  GotoCommand(const char* short_name, const char* friendly_name)
    : Command(short_name, friendly_name, CmdRecordableFlag) { }

  bool onEnabled(Context* context) OVERRIDE {
    return (current_editor != NULL);
  }

  void onExecute(Context* context) OVERRIDE {
    ASSERT(current_editor != NULL);

    current_editor->setFrame(onGetFrame(current_editor));
  }

  virtual FrameNumber onGetFrame(Editor* editor) = 0;
};



class GotoFirstFrameCommand : public GotoCommand
{
public:
  GotoFirstFrameCommand()
    : GotoCommand("GotoFirstFrame",
                  "Goto First Frame") { }
  Command* clone() { return new GotoFirstFrameCommand(*this); }

protected:
  FrameNumber onGetFrame(Editor* editor) OVERRIDE {
    return FrameNumber(0);
  }
};



class GotoPreviousFrameCommand : public GotoCommand

{
public:
  GotoPreviousFrameCommand()
    : GotoCommand("GotoPreviousFrame",
                  "Goto Previous Frame") { }
  Command* clone() { return new GotoPreviousFrameCommand(*this); }

protected:
  FrameNumber onGetFrame(Editor* editor) OVERRIDE {
    FrameNumber frame = editor->getFrame();

    if (frame > FrameNumber(0))
      return frame.previous();
    else
      return editor->getSprite()->getLastFrame();
  }
};



class GotoNextFrameCommand : public GotoCommand

{
public:
  GotoNextFrameCommand() : GotoCommand("GotoNextFrame",
                                       "Goto Next Frame") { }
  Command* clone() { return new GotoNextFrameCommand(*this); }

protected:
  FrameNumber onGetFrame(Editor* editor) OVERRIDE {
    FrameNumber frame = editor->getFrame();
    if (frame < editor->getSprite()->getLastFrame())
      return frame.next();
    else
      return FrameNumber(0);
  }
};



class GotoLastFrameCommand : public GotoCommand
{
public:
  GotoLastFrameCommand() : GotoCommand("GotoLastFrame",
                                       "Goto Last Frame") { }
  Command* clone() { return new GotoLastFrameCommand(*this); }

protected:
  FrameNumber onGetFrame(Editor* editor) OVERRIDE {
    return editor->getSprite()->getLastFrame();
  }
};



class GotoFrameCommand : public GotoCommand

{
public:
  GotoFrameCommand() : GotoCommand("GotoFrame",
                                   "Goto Frame")
                     , m_frame(0) { }
  Command* clone() { return new GotoFrameCommand(*this); }

protected:
  void onLoadParams(Params* params) OVERRIDE
  {
    std::string frame = params->get("frame");
    if (!frame.empty()) m_frame = ustrtol(frame.c_str(), NULL, 10);
    else m_frame = 0;
  }

  FrameNumber onGetFrame(Editor* editor) OVERRIDE {
    if (m_frame == 0) {
      base::UniquePtr<Window> window(app::load_widget<Window>("goto_frame.xml", "goto_frame"));
      Widget* frame = app::find_widget<Widget>(window, "frame");
      Widget* ok = app::find_widget<Widget>(window, "ok");

      frame->setTextf("%d", editor->getFrame()+1);

      window->openWindowInForeground();
      if (window->getKiller() != ok)
        return editor->getFrame();

      m_frame = strtol(frame->getText(), NULL, 10);
    }

    return MID(FrameNumber(0), FrameNumber(m_frame-1), editor->getSprite()->getLastFrame());
  }

private:
  // The frame to go. 0 is "show the UI dialog", another value is the
  // frame (1 is the first name for the user).
  int m_frame;
};

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

} // namespace app
