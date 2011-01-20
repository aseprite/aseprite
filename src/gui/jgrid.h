// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JGRID_H_INCLUDED
#define GUI_JGRID_H_INCLUDED

#include "gui/jbase.h"

JWidget jgrid_new(int columns, bool same_width_columns);

void jgrid_add_child(JWidget grid, JWidget child,
		     int hspan, int vspan, int align);

#endif
