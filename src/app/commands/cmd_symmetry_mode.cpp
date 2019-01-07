// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
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
  // When the m_mode is NONE, it toggles the whole symmetry controls in
  // the context bar.
  if (m_mode == app::gen::SymmetryMode::NONE) {
    enabled(!enabled());
  }
  // In other case it toggles the specific document symmetry specified
  // in m_mode.
  else {
    Doc* doc = ctx->activeDocument();
    DocumentPreferences& docPref = Preferences::instance().document(doc);
    const app::gen::SymmetryMode actual = docPref.symmetry.mode();

    // If the symmetry options are hidden, we'll always show them and
    // activate the m_mode symmetry. We cannot just toggle the
    // symmetry when the options aren't visible in the context bar,
    // because if m_mode is checked, this would cause to show the
    // symmetry options in the context bar but with a unchecked m_mode
    // symmetry.
    if (!enabled()) {
      docPref.symmetry.mode(app::gen::SymmetryMode(int(m_mode) | int(actual)));
      enabled(true);
    }
    // If the symmetry options are visible, we just switch the m_mode
    // symmetry.
    else {
      docPref.symmetry.mode(app::gen::SymmetryMode(int(m_mode) ^ int(actual)));
    }

    // Redraw all editors
    //
    // TODO It looks like only the current editor shows the symmetry,
    //      so it's not necessary to invalidate all editors (only the
    //      current one).
    doc->notifyGeneralUpdate();

    // Redraw the buttons in the context bar.
    //
    // TODO Same with context bar, in the future the context bar could
    //      be listening the DocPref changes to be automatically
    //      invalidated (like it already does with symmetryMode.enabled)
    App::instance()->contextBar()->updateForActiveTool();
  }
}

Command* CommandFactory::createSymmetryModeCommand()
{
  return new SymmetryModeCommand;
}

} // namespace app
