// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/timeline/ani_controls.h"

#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/i18n/strings.h"
#include "app/ui/editor/editor.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "ui/tooltips.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace app {

using namespace app::skin;
using namespace ui;

enum AniAction { ACTION_FIRST, ACTION_PREV, ACTION_PLAY, ACTION_NEXT, ACTION_LAST, ACTIONS };

AniControls::AniControls(TooltipManager* tooltipManager) : ButtonSet(5)
{
  auto* theme = SkinTheme::get(this);

  addItem(theme->parts.aniFirst(), theme->styles.aniButton());
  addItem(theme->parts.aniPrevious(), theme->styles.aniButton());
  addItem(theme->parts.aniPlay(), theme->styles.aniButton());
  addItem(theme->parts.aniNext(), theme->styles.aniButton());
  addItem(theme->parts.aniLast(), theme->styles.aniButton());
  ItemChange.connect([this] { onClickButton(); });

  setTriggerOnMouseUp(true);
  setTransparent(true);

  for (int i = 0; i < ACTIONS; ++i)
    tooltipManager->addTooltipFor(getItem(i), getTooltipFor(i), BOTTOM);

  getItem(ACTION_PLAY)->enableFlags(CTRL_RIGHT_CLICK);

  InitTheme.connect([this] {
    auto theme = SkinTheme::get(this);
    setBgColor(theme->colors.workspace());
  });
}

void AniControls::updateUsingEditor(Editor* editor)
{
  auto theme = SkinTheme::get(this);
  getItem(ACTION_PLAY)
    ->setIcon((editor && editor->isPlaying() ? theme->parts.aniStop() : theme->parts.aniPlay()));
}

void AniControls::onClickButton()
{
  int item = selectedItem();
  deselectItems();

  Command* cmd = Commands::instance()->byId(getCommandId(item));
  if (cmd) {
    auto editor = Editor::activeEditor();

    UIContext::instance()->executeCommandFromMenuOrShortcut(cmd);
    updateUsingEditor(editor);
  }
}

void AniControls::onRightClick(Item* item)
{
  ButtonSet::onRightClick(item);

  auto editor = Editor::activeEditor();
  if (item == getItem(ACTION_PLAY) && editor) {
    editor->showAnimationSpeedMultiplierPopup();
  }
}

const char* AniControls::getCommandId(int index) const
{
  switch (index) {
    case ACTION_FIRST: return CommandId::GotoFirstFrame();
    case ACTION_PREV:  return CommandId::GotoPreviousFrame();
    case ACTION_PLAY:  return CommandId::PlayAnimation();
    case ACTION_NEXT:  return CommandId::GotoNextFrame();
    case ACTION_LAST:  return CommandId::GotoLastFrame();
  }
  ASSERT(false);
  return nullptr;
}

std::string AniControls::getTooltipFor(int index) const
{
  std::string tooltip;

  Command* cmd = Commands::instance()->byId(getCommandId(index));
  if (cmd) {
    tooltip = cmd->friendlyName();

    KeyPtr key = KeyboardShortcuts::instance()->command(cmd->id().c_str());
    if (!key || key->accels().empty())
      key = KeyboardShortcuts::instance()->command(cmd->id().c_str(), Params(), KeyContext::Normal);
    if (key && !key->accels().empty()) {
      tooltip += "\n\n" + Strings::ani_controls_shortcut() + " ";
      tooltip += key->accels().front().toString();
    }

    if (index == ACTION_PLAY) {
      tooltip += "\n\n" + Strings::ani_controls_right_click();
    }
  }

  return tooltip;
}

} // namespace app
