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
#include "app/doc.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/ui_context.h"
#include "app/ui/context_bar.h"
#include "app/modules/gui.h"

namespace app {

class SymmetryModeCommand : public Command {
public:
  SymmetryModeCommand();
  Command* clone() const override { return new SymmetryModeCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
    app::gen::SymmetryMode m_mode = app::gen::SymmetryMode::NONE;
};

SymmetryModeCommand::SymmetryModeCommand()
  : Command(CommandId::SymmetryMode(), CmdUIOnlyFlag)
{
}

std::string SymmetryModeCommand::onGetFriendlyName() const
{
  switch (m_mode) {
    case app::gen::SymmetryMode::HORIZONTAL:
      return Strings::symmetry_toggle_horizontal();
    case app::gen::SymmetryMode::VERTICAL:
      return Strings::symmetry_toggle_vertical();
    default:
      return Strings::symmetry_toggle();
  }
}

void SymmetryModeCommand::onLoadParams(const Params& params)
{
  std::string mode = params.get("orientation");
  if (mode == "vertical") m_mode = app::gen::SymmetryMode::VERTICAL;
  else if (mode == "horizontal") m_mode = app::gen::SymmetryMode::HORIZONTAL;
  else m_mode = app::gen::SymmetryMode::NONE;
}

bool SymmetryModeCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                         ContextFlags::HasActiveSprite);
}

bool SymmetryModeCommand::onChecked(Context* ctx)
{
  return Preferences::instance().symmetryMode.enabled();
}

void SymmetryModeCommand::onExecute(Context* ctx)
{
  auto& enabled = Preferences::instance().symmetryMode.enabled;
  if (m_mode == app::gen::SymmetryMode::NONE) {
    enabled(!enabled());
  }
  else {
    Doc* document = ctx->activeDocument();
    DocumentPreferences& docPref = Preferences::instance().document(ctx->activeDocument());
    app::gen::SymmetryMode actual = docPref.symmetry.mode();
    docPref.symmetry.mode(app::gen::SymmetryMode(int(m_mode) ^ int(actual)));
    enabled(true);
    document->notifyGeneralUpdate();
    App::instance()->contextBar()->updateForActiveTool();
  }
}

Command* CommandFactory::createSymmetryModeCommand()
{
  return new SymmetryModeCommand;
}

} // namespace app
