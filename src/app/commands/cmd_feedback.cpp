// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/modules/gui.h"
#include "app/ui/main_window.h"
#include "fmt/format.h"
#include "ver/info.h"

#include "feedback.xml.h"

namespace app {

using namespace ui;

class FeedbackCommand : public Command {
public:
  FeedbackCommand();

protected:
  void onExecute(Context* context) override;
};

FeedbackCommand::FeedbackCommand()
  : Command(CommandId::Feedback(), CmdUIOnlyFlag)
{
}

void FeedbackCommand::onExecute(Context* context)
{
  gen::Feedback window;
  window.openWindowInForeground();
}

Command* CommandFactory::createFeedbackCommand()
{
  return new FeedbackCommand;
}

} // namespace app
