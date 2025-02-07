// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/loop_tag.h"
#include "app/match_words.h"
#include "app/modules/gui.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/search_entry.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "ui/combobox.h"
#include "ui/window.h"

#include "goto_frame.xml.h"

namespace app {

using namespace ui;
using namespace doc;

class GotoCommand : public Command {
protected:
  GotoCommand(const char* id) : Command(id, CmdRecordableFlag) {}

  bool onEnabled(Context* context) override { return (Editor::activeEditor() != nullptr); }

  void onExecute(Context* context) override
  {
    auto editor = Editor::activeEditor();
    ASSERT(editor != nullptr);

    editor->setFrame(onGetFrame(editor));
  }

  virtual frame_t onGetFrame(Editor* editor) = 0;
};

class GotoFirstFrameCommand : public GotoCommand {
public:
  GotoFirstFrameCommand() : GotoCommand(CommandId::GotoFirstFrame()) {}

protected:
  frame_t onGetFrame(Editor* editor) override { return 0; }
};

class GotoFirstFrameInTagCommand : public GotoCommand {
public:
  GotoFirstFrameInTagCommand() : GotoCommand(CommandId::GotoFirstFrameInTag()) {}

protected:
  frame_t onGetFrame(Editor* editor) override
  {
    frame_t frame = editor->frame();
    Tag* tag = editor->getCustomizationDelegate()->getTagProvider()->getTagByFrame(frame, false);
    return (tag ? tag->fromFrame() : 0);
  }
};

class GotoPreviousFrameCommand : public GotoCommand {
public:
  GotoPreviousFrameCommand() : GotoCommand(CommandId::GotoPreviousFrame()) {}

protected:
  frame_t onGetFrame(Editor* editor) override
  {
    frame_t frame = editor->frame();
    frame_t last = editor->sprite()->lastFrame();

    return (frame > 0 ? frame - 1 : last);
  }
};

class GotoNextFrameCommand : public GotoCommand {
public:
  GotoNextFrameCommand() : GotoCommand(CommandId::GotoNextFrame()) {}

protected:
  frame_t onGetFrame(Editor* editor) override
  {
    frame_t frame = editor->frame();
    frame_t last = editor->sprite()->lastFrame();

    return (frame < last ? frame + 1 : 0);
  }
};

class GotoNextFrameWithSameTagCommand : public GotoCommand {
public:
  GotoNextFrameWithSameTagCommand() : GotoCommand(CommandId::GotoNextFrameWithSameTag()) {}

protected:
  frame_t onGetFrame(Editor* editor) override
  {
    frame_t frame = editor->frame();
    Tag* tag = editor->getCustomizationDelegate()->getTagProvider()->getTagByFrame(frame, false);
    frame_t first = (tag ? tag->fromFrame() : 0);
    frame_t last = (tag ? tag->toFrame() : editor->sprite()->lastFrame());

    return (frame < last ? frame + 1 : first);
  }
};

class GotoPreviousFrameWithSameTagCommand : public GotoCommand {
public:
  GotoPreviousFrameWithSameTagCommand() : GotoCommand(CommandId::GotoPreviousFrameWithSameTag()) {}

protected:
  frame_t onGetFrame(Editor* editor) override
  {
    frame_t frame = editor->frame();
    Tag* tag = editor->getCustomizationDelegate()->getTagProvider()->getTagByFrame(frame, false);
    frame_t first = (tag ? tag->fromFrame() : 0);
    frame_t last = (tag ? tag->toFrame() : editor->sprite()->lastFrame());

    return (frame > first ? frame - 1 : last);
  }
};

class GotoLastFrameCommand : public GotoCommand {
public:
  GotoLastFrameCommand() : GotoCommand(CommandId::GotoLastFrame()) {}

protected:
  frame_t onGetFrame(Editor* editor) override { return editor->sprite()->lastFrame(); }
};

class GotoLastFrameInTagCommand : public GotoCommand {
public:
  GotoLastFrameInTagCommand() : GotoCommand(CommandId::GotoLastFrameInTag()) {}

protected:
  frame_t onGetFrame(Editor* editor) override
  {
    frame_t frame = editor->frame();
    Tag* tag = editor->getCustomizationDelegate()->getTagProvider()->getTagByFrame(frame, false);
    return (tag ? tag->toFrame() : editor->sprite()->lastFrame());
  }
};

class GotoFrameCommand : public GotoCommand {
public:
  GotoFrameCommand() : GotoCommand(CommandId::GotoFrame()), m_showUI(true) {}

private:
  // TODO this combobox is similar to FileSelector::CustomFileNameEntry
  class TagsEntry : public ComboBox {
  public:
    TagsEntry(Tags& tags) : m_tags(tags)
    {
      setEditable(true);
      getEntryWidget()->Change.connect(&TagsEntry::onEntryChange, this);
      fill(true);
    }

  private:
    void fill(bool all)
    {
      deleteAllItems();

      MatchWords match(getEntryWidget()->text());

      bool matchAny = false;
      for (const auto& tag : m_tags) {
        if (match(tag->name())) {
          matchAny = true;
          break;
        }
      }
      for (const auto& tag : m_tags) {
        if (all || !matchAny || match(tag->name()))
          addItem(tag->name());
      }
    }

    void onEntryChange()
    {
      closeListBox();
      fill(false);
      if (getItemCount() > 0)
        openListBox();
    }

    Tags& m_tags;
  };

  void onLoadParams(const Params& params) override
  {
    std::string frame = params.get("frame");
    if (!frame.empty()) {
      m_frame = strtol(frame.c_str(), nullptr, 10);
      m_showUI = false;
    }
    else
      m_showUI = true;
  }

  frame_t onGetFrame(Editor* editor) override
  {
    auto& docPref = editor->docPref();

    if (m_showUI) {
      app::gen::GotoFrame window;
      TagsEntry combobox(editor->sprite()->tags());

      window.framePlaceholder()->addChild(&combobox);

      combobox.setFocusMagnet(true);
      combobox.getEntryWidget()->setTextf("%d", editor->frame() + docPref.timeline.firstFrame());

      window.openWindowInForeground();
      if (window.closer() != window.ok())
        return editor->frame();

      std::string text = combobox.getEntryWidget()->text();
      frame_t frameNum = base::convert_to<int>(text);
      std::string textFromInt = base::convert_to<std::string>(frameNum);
      if (text == textFromInt) {
        m_frame = frameNum;
      }
      // Search a tag name
      else {
        MatchWords match(text);
        for (const auto& tag : editor->sprite()->tags()) {
          if (match(tag->name())) {
            m_frame = tag->fromFrame() + docPref.timeline.firstFrame();
            break;
          }
        }
      }
    }

    return std::clamp(m_frame - docPref.timeline.firstFrame(), 0, editor->sprite()->lastFrame());
  }

private:
  bool m_showUI;
  int m_frame;
};

Command* CommandFactory::createGotoFirstFrameCommand()
{
  return new GotoFirstFrameCommand;
}

Command* CommandFactory::createGotoFirstFrameInTagCommand()
{
  return new GotoFirstFrameInTagCommand;
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

Command* CommandFactory::createGotoLastFrameInTagCommand()
{
  return new GotoLastFrameInTagCommand;
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
