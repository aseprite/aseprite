// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_SLIDER_H_INCLUDED
#define GUI_SLIDER_H_INCLUDED

#include "gui/jbase.h"

JWidget jslider_new(int min, int max, int value);

void jslider_set_range(JWidget slider, int min, int max);

void jslider_set_value(JWidget slider, int value);
int jslider_get_value(JWidget slider);

/* for themes */
void jtheme_slider_info(JWidget slider, int *min, int *max, int *value);

#endif
