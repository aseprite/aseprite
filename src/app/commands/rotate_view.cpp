// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/command.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "app/ui/editor/editor.h"
#include "base/convert_to.h"

namespace app {

struct RotateViewParams : public NewParams {
  Param<double> angle{ this, 0.0, "angle" };
  Param<std::string> action{ this, "set", "action" }; // set, add, subtract
  Param<bool> preview{ this, false, "preview" };
};

class RotateViewCommand : public CommandWithNewParams<RotateViewParams> {
public:
  RotateViewCommand();

protected:
  bool onNeedsParams() const override { return true; }
  void onExecute(Context* ctx) override;
  std::string onGetFriendlyName() const override;
};

RotateViewCommand::RotateViewCommand()
  : CommandWithNewParams<RotateViewParams>(CommandId::RotateView())
{
}

void RotateViewCommand::onExecute(Context* ctx)
{
  if (!ctx->isUIAvailable())
    return;

  if (auto* editor = Editor::activeEditor()) {
    auto opts = editor->viewOptions();
    const double a = params().angle();
    if (params().action() == "add")
      opts.angle += a;
    else if (params().action() == "subtract")
      opts.angle -= a;
    else
      opts.angle = a;
    editor->setViewOptions(opts);
  }
}

std::string RotateViewCommand::onGetFriendlyName() const
{
  std::string value;
  if (params().action() == "add")
    value = "+";
  else if (params().action() == "subtract")
    value = "-";
  else
    value = "=";
  value += base::convert_to<std::string>(params().angle());
  value += "º";
  return Strings::commands_RotateView(value);
}

Command* CommandFactory::createRotateViewCommand()
{
  return new RotateViewCommand;
}

} // namespace app
