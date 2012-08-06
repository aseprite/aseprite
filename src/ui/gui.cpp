// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/base.h"
#include "ui/clipboard.h"
#include "ui/overlay_manager.h"
#include "ui/theme.h"

namespace ui {

int _ji_widgets_init();
void _ji_widgets_exit();

int _ji_system_init();
void _ji_system_exit();

int _ji_font_init();
void _ji_font_exit();

GuiSystem::GuiSystem()
{
  // initialize system
  _ji_system_init();
  _ji_font_init();
  _ji_widgets_init();
}

GuiSystem::~GuiSystem()
{
  OverlayManager::destroyInstance();

  // finish theme
  CurrentTheme::set(NULL);

  // destroy clipboard
  clipboard::set_text(NULL);

  // shutdown system
  _ji_widgets_exit();
  _ji_font_exit();
  _ji_system_exit();
}

} // namespace ui
