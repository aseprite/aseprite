// Aseprite
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
#include "app/context.h"
#include "app/pref/preferences.h"
#include "filters/tiled_mode.h"

namespace app {

class TiledModeCommand : public Command {
public:
  TiledModeCommand();

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;

  filters::TiledMode m_mode;
};

TiledModeCommand::TiledModeCommand()
  : Command(CommandId::TiledMode(), CmdUIOnlyFlag)
  , m_mode(filters::TiledMode::NONE)
{
}

void TiledModeCommand::onLoadParams(const Params& params)
{
  m_mode = filters::TiledMode::NONE;

  std::string mode = params.get("axis");
  if (mode == "both") m_mode = filters::TiledMode::BOTH;
  else if (mode == "x") m_mode = filters::TiledMode::X_AXIS;
  else if (mode == "y") m_mode = filters::TiledMode::Y_AXIS;
}

bool TiledModeCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                         ContextFlags::HasActiveSprite);
}

bool TiledModeCommand::onChecked(Context* ctx)
{
  const Doc* doc = ctx->activeDocument();
  return (Preferences::instance().document(doc).tiled.mode() == m_mode);
}

void TiledModeCommand::onExecute(Context* ctx)
{
  const Doc* doc = ctx->activeDocument();
  Preferences::instance().document(doc).tiled.mode(m_mode);
}

Command* CommandFactory::createTiledModeCommand()
{
  return new TiledModeCommand;
}

} // namespace app
