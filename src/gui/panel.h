// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_PANEL_H_INCLUDED
#define GUI_PANEL_H_INCLUDED

#include "gui/base.h"

JWidget jpanel_new(int align);

double jpanel_get_pos(JWidget panel);
void jpanel_set_pos(JWidget panel, double pos);

#endif
