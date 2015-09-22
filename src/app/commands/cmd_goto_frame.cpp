// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
#include "doc/sprite.h"
#include "ui/window.h"

#include "goto_frame.xml.h"

namespace app {

using namespace ui;

class GotoCommand : public Command {
protected:
  GotoCommand(const char* short_name, const char* friendly_name)
    : Command(short_name, friendly_name, CmdRecordableFlag) { }

  bool onEnabled(Context* context) override {
    return (current_editor != NULL);
  }

  void onExecute(Context* context) override {
    ASSERT(current_editor != NULL);

    current_editor->setFrame(onGetFrame(current_editor));
  }

  virtual frame_t onGetFrame(Editor* editor) = 0;
};

class GotoFirstFrameCommand : public GotoCommand {
public:
  GotoFirstFrameCommand()
    : GotoCommand("GotoFirstFrame",
                  "Go to First Frame") { }
  Command* clone() const override { return new GotoFirstFrameCommand(*this); }

protected:
  frame_t onGetFrame(Editor* editor) override {
    return frame_t(0);
  }
};

class GotoPreviousFrameCommand : public GotoCommand {
public:
  GotoPreviousFrameCommand()
    : GotoCommand("GotoPreviousFrame",
                  "Go to Previous Frame") { }
  Command* clone() const override { return new GotoPreviousFrameCommand(*this); }

protected:
  frame_t onGetFrame(Editor* editor) override {
    frame_t frame = editor->frame();

    if (frame > frame_t(0))
      return frame-1;
    else
      return editor->sprite()->lastFrame();
  }
};

class GotoNextFrameCommand : public GotoCommand {
public:
  GotoNextFrameCommand() : GotoCommand("GotoNextFrame",
                                       "Go to Next Frame") { }
  Command* clone() const override { return new GotoNextFrameCommand(*this); }

protected:
  frame_t onGetFrame(Editor* editor) override {
    frame_t frame = editor->frame();
    if (frame < editor->sprite()->lastFrame())
      return frame+1;
    else
      return frame_t(0);
  }
};

class GotoLastFrameCommand : public GotoCommand {
public:
  GotoLastFrameCommand() : GotoCommand("GotoLastFrame",
                                       "Go to Last Frame") { }
  Command* clone() const override { return new GotoLastFrameCommand(*this); }

protected:
  frame_t onGetFrame(Editor* editor) override {
    return editor->sprite()->lastFrame();
  }
};

class GotoFrameCommand : public GotoCommand {
public:
  GotoFrameCommand() : GotoCommand("GotoFrame",
                                   "Go to Frame")
                     , m_frame(0) { }
  Command* clone() const override { return new GotoFrameCommand(*this); }

protected:
  void onLoadParams(const Params& params) override
  {
    std::string frame = params.get("frame");
    if (!frame.empty()) m_frame = strtol(frame.c_str(), NULL, 10);
    else m_frame = 0;
  }

  frame_t onGetFrame(Editor* editor) override {
    if (m_frame == 0) {
      app::gen::GotoFrame window;

      window.frame()->setTextf("%d", editor->frame()+1);

      window.openWindowInForeground();
      if (window.getKiller() != window.ok())
        return editor->frame();

      m_frame = window.frame()->getTextInt();
    }

    return MID(0, m_frame-1, editor->sprite()->lastFrame());
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
