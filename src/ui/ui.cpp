// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/base.h"
#include "ui/overlay_manager.h"
#include "ui/theme.h"

namespace ui {

int init_widgets();
void exit_widgets();

int init_system();
void exit_system();

GuiSystem::GuiSystem()
{
  // initialize system
  init_system();
  init_widgets();
}

GuiSystem::~GuiSystem()
{
  OverlayManager::destroyInstance();

  // finish theme
  CurrentTheme::set(NULL);

  // shutdown system
  exit_widgets();
  exit_system();
}

} // namespace ui
