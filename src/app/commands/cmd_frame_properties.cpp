// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "app/transaction.h"
#include "base/convert_to.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "frame_properties.xml.h"

namespace app {

using namespace ui;

class FramePropertiesCommand : public Command {
public:
  FramePropertiesCommand();
  Command* clone() const override { return new FramePropertiesCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;

private:
  enum Target {
    ALL_FRAMES = -1,
    CURRENT_RANGE = 0,
    SPECIFIC_FRAME = 1
  };

  // Frame to be shown. It can be ALL_FRAMES, CURRENT_RANGE, or a
  // number indicating a specific frame (1 is the first frame).
  Target m_target;
  frame_t m_frame;
};

FramePropertiesCommand::FramePropertiesCommand()
  : Command("FrameProperties",
            "Frame Properties",
            CmdUIOnlyFlag)
{
}

void FramePropertiesCommand::onLoadParams(const Params& params)
{
  std::string frame = params.get("frame");
  if (frame == "all") {
    m_target = ALL_FRAMES;
  }
  else if (frame == "current") {
    m_target = CURRENT_RANGE;
  }
  else {
    m_target = SPECIFIC_FRAME;
    m_frame = frame_t(base::convert_to<int>(frame)-1);
  }
}

bool FramePropertiesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void FramePropertiesCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite = reader.sprite();

  app::gen::FrameProperties window;
  frame_t firstFrame = 0;
  frame_t lastFrame = 0;

  switch (m_target) {

    case ALL_FRAMES:
      lastFrame = sprite->lastFrame();
      break;

    case CURRENT_RANGE: {
      // TODO the range of selected frames should be in doc::Site.
      Timeline::Range range = App::instance()->getMainWindow()->getTimeline()->range();
      if (range.enabled()) {
        firstFrame = range.frameBegin();
        lastFrame = range.frameEnd();
      }
      else {
        firstFrame = lastFrame = reader.frame();
      }
      break;
    }

    case SPECIFIC_FRAME:
      firstFrame = lastFrame = m_frame;
      break;
  }

  if (firstFrame != lastFrame)
    window.frame()->setTextf("[%d...%d]", (int)firstFrame+1, (int)lastFrame+1);
  else
    window.frame()->setTextf("%d", (int)firstFrame+1);

  window.frlen()->setTextf("%d", sprite->frameDuration(firstFrame));

  window.openWindowInForeground();
  if (window.getKiller() == window.ok()) {
    int num = window.frlen()->getTextInt();

    ContextWriter writer(reader);
    Transaction transaction(writer.context(), "Frame Duration");
    DocumentApi api = writer.document()->getApi(transaction);
    if (firstFrame != lastFrame)
      api.setFrameRangeDuration(writer.sprite(), firstFrame, lastFrame, num);
    else
      api.setFrameDuration(writer.sprite(), firstFrame, num);
    transaction.commit();
  }
}

Command* CommandFactory::createFramePropertiesCommand()
{
  return new FramePropertiesCommand;
}

} // namespace app
