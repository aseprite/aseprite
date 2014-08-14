// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
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

int _ji_widgets_init();
void _ji_widgets_exit();

int _ji_system_init();
void _ji_system_exit();

GuiSystem::GuiSystem()
{
  // initialize system
  _ji_system_init();
  _ji_widgets_init();
}

GuiSystem::~GuiSystem()
{
  OverlayManager::destroyInstance();

  // finish theme
  CurrentTheme::set(NULL);

  // shutdown system
  _ji_widgets_exit();
  _ji_system_exit();
}

} // namespace ui
