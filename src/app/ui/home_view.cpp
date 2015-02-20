// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/home_view.h"

#include "app/ui/skin/skin_theme.h"
#include "app/ui/workspace.h"
#include "ui/label.h"
#include "ui/textbox.h"
#include "ui/view.h"

namespace app {

using namespace ui;
using namespace skin;

HomeView::HomeView()
  : Box(JI_VERTICAL)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  setBgColor(theme->colors.workspace());

  child_spacing = 8 * guiscale();

  Label* label = new Label("Welcome to " PACKAGE " v" VERSION);
  label->setTextColor(theme->colors.workspaceText());
  addChild(label);
}

HomeView::~HomeView()
{
}

std::string HomeView::getTabText()
{
  return "Home";
}

TabIcon HomeView::getTabIcon()
{
  return TabIcon::HOME;
}

WorkspaceView* HomeView::cloneWorkspaceView()
{
  return NULL;                  // This view cannot be cloned
}

void HomeView::onClonedFrom(WorkspaceView* from)
{
  ASSERT(false);                // Never called
}

void HomeView::onCloseView(Workspace* workspace)
{
  workspace->removeView(this);
}

void HomeView::onWorkspaceViewSelected()
{
}

} // namespace app
