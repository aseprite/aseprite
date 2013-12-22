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

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "app/undo_transaction.h"
#include "base/convert_to.h"
#include "raster/sprite.h"
#include "ui/ui.h"

namespace app {

using namespace ui;

class FramePropertiesCommand : public Command {
public:
  FramePropertiesCommand();
  Command* clone() { return new FramePropertiesCommand(*this); }

protected:
  void onLoadParams(Params* params);
  bool onEnabled(Context* context);
  void onExecute(Context* context);

private:
  enum Target {
    ALL_FRAMES = -1,
    CURRENT_RANGE = 0,
    SPECIFIC_FRAME = 1
  };

  // Frame to be shown. It can be ALL_FRAMES, CURRENT_RANGE, or a
  // number indicating a specific frame (1 is the first frame).
  Target m_target;
  FrameNumber m_frame;
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
    m_target = ALL_FRAMES;
  }
  else if (frame == "current") {
    m_target = CURRENT_RANGE;
  }
  else {
    m_target = SPECIFIC_FRAME;
    m_frame = FrameNumber(base::convert_to<int>(frame)-1);
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

  base::UniquePtr<Window> window(app::load_widget<Window>("frame_duration.xml", "frame_duration"));
  Widget* frame = app::find_widget<Widget>(window, "frame");
  Widget* frlen = app::find_widget<Widget>(window, "frlen");
  Widget* ok = app::find_widget<Widget>(window, "ok");

  FrameNumber firstFrame(0);
  FrameNumber lastFrame(0);

  switch (m_target) {

    case ALL_FRAMES:
      lastFrame = sprite->getLastFrame();
      break;

    case CURRENT_RANGE: {
      // TODO the range of selected frames should be in the DocumentLocation.
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
    frame->setTextf("%d-%d", (int)firstFrame+1, (int)lastFrame+1);
  else
    frame->setTextf("%d", (int)firstFrame+1);

  frlen->setTextf("%d", sprite->getFrameDuration(firstFrame));

  window->openWindowInForeground();
  if (window->getKiller() == ok) {
    int num = frlen->getTextInt();

    ContextWriter writer(reader);
    UndoTransaction undoTransaction(writer.context(), "Frame Duration");
    if (firstFrame != lastFrame)
      writer.document()->getApi().setConstantFrameRate(writer.sprite(), firstFrame, lastFrame, num);
    else
      writer.document()->getApi().setFrameDuration(writer.sprite(), firstFrame, num);
    undoTransaction.commit();
  }
}

Command* CommandFactory::createFramePropertiesCommand()
{
  return new FramePropertiesCommand;
}

} // namespace app
