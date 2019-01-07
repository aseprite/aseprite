// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/ui/input_chain.h"

namespace app {

class CancelCommand : public Command {
public:
  enum Type {
    NoOp,
    All,
  };

  CancelCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;

private:
  Type m_type;
};

CancelCommand::CancelCommand()
  : Command(CommandId::Cancel(), CmdUIOnlyFlag)
  , m_type(NoOp)
{
}

void CancelCommand::onLoadParams(const Params& params)
{
  std::string type = params.get("type");
  if (type == "noop") m_type = NoOp;
  else if (type == "all") m_type = All;
}

void CancelCommand::onExecute(Context* context)
{
  switch (m_type) {

    case NoOp:
      // Do nothing.
      break;

    case All:
      // TODO should the ContextBar be a InputChainElement to intercept onCancel()?
      // Discard brush
      {
        Command* discardBrush = Commands::instance()->byId(
          CommandId::DiscardBrush());
        context->executeCommand(discardBrush);
      }

      App::instance()->inputChain().cancel(context);
      break;
  }
}

Command* CommandFactory::createCancelCommand()
{
  return new CancelCommand;
}

} // namespace app
