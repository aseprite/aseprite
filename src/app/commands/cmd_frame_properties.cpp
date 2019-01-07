// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "base/convert_to.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "frame_properties.xml.h"

namespace app {

using namespace ui;

class FramePropertiesCommand : public Command {
public:
  FramePropertiesCommand();

protected:
  bool onNeedsParams() const override { return true; }
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
  : Command(CommandId::FrameProperties(), CmdUIOnlyFlag)
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
    m_frame = frame_t(base::convert_to<int>(frame));
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
  auto& docPref = Preferences::instance().document(context->activeDocument());
  int base = docPref.timeline.firstFrame();
  app::gen::FrameProperties window;
  SelectedFrames selFrames;

  switch (m_target) {

    case ALL_FRAMES:
      selFrames.insert(0, sprite->lastFrame());
      break;

    case CURRENT_RANGE: {
      Site site = context->activeSite();
      if (site.inTimeline()) {
        selFrames = site.selectedFrames();
      }
      else {
        selFrames.insert(site.frame());
      }
      break;
    }

    case SPECIFIC_FRAME:
      selFrames.insert(m_frame-base);
      break;
  }

  ASSERT(!selFrames.empty());
  if (selFrames.empty())
    return;

  if (selFrames.size() == 1)
    window.frame()->setTextf("%d", selFrames.firstFrame()+base);
  else if (selFrames.ranges() == 1) {
    window.frame()->setTextf("[%d...%d]",
                             selFrames.firstFrame()+base,
                             selFrames.lastFrame()+base);
  }
  else
    window.frame()->setTextf("Multiple Frames");

  window.frlen()->setTextf(
    "%d", sprite->frameDuration(selFrames.firstFrame()));

  window.openWindowInForeground();
  if (window.closer() == window.ok()) {
    int newMsecs = window.frlen()->textInt();

    ContextWriter writer(reader);
    Tx tx(writer.context(), "Frame Duration");
    DocApi api = writer.document()->getApi(tx);

    for (frame_t frame : selFrames)
      api.setFrameDuration(writer.sprite(), frame, newMsecs);

    tx.commit();
  }
}

Command* CommandFactory::createFramePropertiesCommand()
{
  return new FramePropertiesCommand;
}

} // namespace app
