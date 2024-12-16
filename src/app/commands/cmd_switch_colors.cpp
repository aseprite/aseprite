// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "ui/base.h"

namespace app {

class SwitchColorsCommand : public Command {
public:
  SwitchColorsCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

SwitchColorsCommand::SwitchColorsCommand() : Command(CommandId::SwitchColors(), CmdUIOnlyFlag)
{
}

bool SwitchColorsCommand::onEnabled(Context* context)
{
  return (Editor::activeEditor() ? true : false);
}

void SwitchColorsCommand::onExecute(Context* context)
{
  auto editor = Editor::activeEditor();
  ASSERT(editor);
  if (!editor)
    return;

  tools::Tool* tool = editor->getCurrentEditorTool();
  if (tool) {
    const auto& toolPref(Preferences::instance().tool(tool));
    if (toolPref.ink() == tools::InkType::SHADING) {
      App::instance()->contextBar()->reverseShadeColors();
    }
  }

  DisableColorBarEditMode disable;
  Site site = context->activeSite();
  if (site.tilemapMode() == TilemapMode::Tiles) {
    auto& pref = Preferences::instance();
    doc::tile_t fg = pref.colorBar.fgTile();
    doc::tile_t bg = pref.colorBar.bgTile();
    pref.colorBar.bgTile(fg);
    pref.colorBar.fgTile(bg);
  }
  else {
    ColorBar* colorbar = ColorBar::instance();
    app::Color fg = colorbar->getFgColor();
    app::Color bg = colorbar->getBgColor();

    // Change the background and then the foreground color so the color
    // spectrum and color wheel shows the foreground color as the
    // selected one.
    colorbar->setBgColor(fg);
    colorbar->setFgColor(bg);
  }
}

Command* CommandFactory::createSwitchColorsCommand()
{
  return new SwitchColorsCommand;
}

} // namespace app
