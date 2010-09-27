// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JPANEL_H_INCLUDED
#define GUI_JPANEL_H_INCLUDED

#include "gui/jbase.h"

JWidget jpanel_new(int align);

double jpanel_get_pos(JWidget panel);
void jpanel_set_pos(JWidget panel, double pos);

#endif
