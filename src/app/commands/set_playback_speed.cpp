// Aseprite
// Copyright (c) 2023-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/new_params.h"
#include "app/i18n/strings.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"

namespace app {

struct SetPlaybackSpeedParams : public NewParams {
  Param<double> multiplier{ this, 1.0, "multiplier" };
};

class SetPlaybackSpeedCommand : public CommandWithNewParams<SetPlaybackSpeedParams> {
public:
  SetPlaybackSpeedCommand();

protected:
  bool onChecked(Context* ctx) override;
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;
  bool isListed(const Params& params) const override { return !params.empty(); }
};

SetPlaybackSpeedCommand::SetPlaybackSpeedCommand()
  : CommandWithNewParams(CommandId::SetPlaybackSpeed(), CmdUIOnlyFlag)
{
}

bool SetPlaybackSpeedCommand::onChecked(Context* ctx)
{
  Editor* editor = nullptr;
  if (ctx->isUIAvailable())
    editor = static_cast<UIContext*>(ctx)->activeEditor();
  if (editor)
    return (params().multiplier() == editor->getAnimationSpeedMultiplier());
  else
    return false;
}

void SetPlaybackSpeedCommand::onExecute(Context* ctx)
{
  Editor* editor = nullptr;
  if (ctx->isUIAvailable())
    editor = static_cast<UIContext*>(ctx)->activeEditor();
  if (editor)
    editor->setAnimationSpeedMultiplier(params().multiplier());
}

std::string SetPlaybackSpeedCommand::onGetFriendlyName() const
{
  return Strings::commands_SetPlaybackSpeed(params().multiplier());
}

Command* CommandFactory::createSetPlaybackSpeedCommand()
{
  return new SetPlaybackSpeedCommand;
}

} // namespace app
