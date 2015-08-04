// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_access.h"
#include "app/document_range.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/tools/tool.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button.h"
#include "app/ui/editor/editor.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "app/ui_context.h"
#include "app/util/range_utils.h"
#include "base/bind.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "gfx/size.h"
#include "she/font.h"
#include "she/surface.h"
#include "ui/ui.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace doc;

enum AniAction {
  ACTION_FIRST,
  ACTION_PREV,
  ACTION_PLAY,
  ACTION_NEXT,
  ACTION_LAST,
};

AniControls::AniControls()
  : ButtonSet(5)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());

  addItem(theme->parts.aniFirst());
  addItem(theme->parts.aniPrevious());
  addItem(theme->parts.aniPlay());
  addItem(theme->parts.aniNext());
  addItem(theme->parts.aniLast());
  ItemChange.connect(Bind(&AniControls::onPlayButton, this));

  setTriggerOnMouseUp(true);
  setTransparent(true);
  setBgColor(theme->colors.workspace());
}

void AniControls::updateUsingEditor(Editor* editor)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  getItem(ACTION_PLAY)->setIcon(
    (editor && editor->isPlaying() ?
      theme->parts.aniStop():
      theme->parts.aniPlay()));
}

void AniControls::onPlayButton()
{
  int item = selectedItem();
  deselectItems();

  Command* cmd = nullptr;
  switch (item) {
    case ACTION_FIRST: cmd = CommandsModule::instance()->getCommandByName(CommandId::GotoFirstFrame); break;
    case ACTION_PREV: cmd = CommandsModule::instance()->getCommandByName(CommandId::GotoPreviousFrame); break;
    case ACTION_PLAY: cmd = CommandsModule::instance()->getCommandByName(CommandId::PlayAnimation); break;
    case ACTION_NEXT: cmd = CommandsModule::instance()->getCommandByName(CommandId::GotoNextFrame); break;
    case ACTION_LAST: cmd = CommandsModule::instance()->getCommandByName(CommandId::GotoLastFrame); break;
  }
  if (cmd) {
    UIContext::instance()->executeCommand(cmd);
    updateUsingEditor(current_editor);
  }
}

void AniControls::onRightClick(Item* item)
{
  ButtonSet::onRightClick(item);

  if (item == getItem(ACTION_PLAY) && current_editor)
    current_editor->showAnimationSpeedMultiplierPopup(true);
}

} // namespace app
