// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JQUICKMENU_H_INCLUDED
#define GUI_JQUICKMENU_H_INCLUDED

#include "gui/jbase.h"

struct jquickmenu
{
  int level;
  const char *text;
  const char *accel;
  void (*quick_handler)(JWidget widget, int user_data);
  int user_data;
};

JWidget jmenubar_new_quickmenu(JQuickMenu quick_menu);
JWidget jmenubox_new_quickmenu(JQuickMenu quick_menu);

#endif
