// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/ui/input_chain.h"

#include <memory>

namespace app {

class PasteCommand : public Command {
public:
  PasteCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;

private:
  std::unique_ptr<gfx::Point> m_position;
};

PasteCommand::PasteCommand() : Command(CommandId::Paste(), CmdUIOnlyFlag)
{
}

void PasteCommand::onLoadParams(const Params& params)
{
  m_position.reset();
  if (params.has_param("x") || params.has_param("y")) {
    m_position.reset(new gfx::Point);
    m_position->x = params.get_as<int>("x");
    m_position->y = params.get_as<int>("y");
  }
}

bool PasteCommand::onEnabled(Context* ctx)
{
  return App::instance()->inputChain().canPaste(ctx);
}

void PasteCommand::onExecute(Context* ctx)
{
  App::instance()->inputChain().paste(ctx, m_position.get());
}

Command* CommandFactory::createPasteCommand()
{
  return new PasteCommand;
}

} // namespace app
