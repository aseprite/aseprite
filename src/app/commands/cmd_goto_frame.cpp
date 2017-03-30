// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/loop_tag.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "doc/frame_tag.h"
#include "doc/sprite.h"
#include "ui/window.h"

#include "goto_frame.xml.h"

namespace app {

using namespace ui;
using namespace doc;

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
    return 0;
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
    frame_t last = editor->sprite()->lastFrame();

    return (frame > 0 ? frame-1: last);
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
    frame_t last = editor->sprite()->lastFrame();

    return (frame < last ? frame+1: 0);
  }
};

class GotoNextFrameWithSameTagCommand : public GotoCommand {
public:
  GotoNextFrameWithSameTagCommand() : GotoCommand("GotoNextFrameWithSameTag",
                                                  "Go to Next Frame with same tag") { }
  Command* clone() const override { return new GotoNextFrameWithSameTagCommand(*this); }

protected:
  frame_t onGetFrame(Editor* editor) override {
    frame_t frame = editor->frame();
    FrameTag* tag = editor
      ->getCustomizationDelegate()
      ->getFrameTagProvider()
      ->getFrameTagByFrame(frame);
    frame_t first = (tag ? tag->fromFrame(): 0);
    frame_t last = (tag ? tag->toFrame(): editor->sprite()->lastFrame());

    return (frame < last ? frame+1: first);
  }
};

class GotoPreviousFrameWithSameTagCommand : public GotoCommand {
public:
  GotoPreviousFrameWithSameTagCommand() : GotoCommand("GotoPreviousFrameWithSameTag",
                                                      "Go to Previous Frame with same tag") { }
  Command* clone() const override { return new GotoPreviousFrameWithSameTagCommand(*this); }

protected:
  frame_t onGetFrame(Editor* editor) override {
    frame_t frame = editor->frame();
    FrameTag* tag = editor
      ->getCustomizationDelegate()
      ->getFrameTagProvider()
      ->getFrameTagByFrame(frame);
    frame_t first = (tag ? tag->fromFrame(): 0);
    frame_t last = (tag ? tag->toFrame(): editor->sprite()->lastFrame());

    return (frame > first ? frame-1: last);
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
                     , m_showUI(true) { }
  Command* clone() const override { return new GotoFrameCommand(*this); }

protected:
  void onLoadParams(const Params& params) override {
    std::string frame = params.get("frame");
    if (!frame.empty()) {
      m_frame = strtol(frame.c_str(), nullptr, 10);
      m_showUI = false;
    }
    else
      m_showUI = true;
  }

  frame_t onGetFrame(Editor* editor) override {
    auto& docPref = editor->docPref();

    if (m_showUI) {
      app::gen::GotoFrame window;
      window.frame()->setTextf(
        "%d", editor->frame()+docPref.timeline.firstFrame());
      window.openWindowInForeground();
      if (window.closer() != window.ok())
        return editor->frame();

      m_frame = window.frame()->textInt();
    }

    return MID(0, m_frame-docPref.timeline.firstFrame(), editor->sprite()->lastFrame());
  }

private:
  bool m_showUI;
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

Command* CommandFactory::createGotoNextFrameWithSameTagCommand()
{
  return new GotoNextFrameWithSameTagCommand;
}

Command* CommandFactory::createGotoPreviousFrameWithSameTagCommand()
{
  return new GotoPreviousFrameWithSameTagCommand;
}

Command* CommandFactory::createGotoFrameCommand()
{
  return new GotoFrameCommand;
}

} // namespace app
