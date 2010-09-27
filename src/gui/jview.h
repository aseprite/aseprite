// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JVIEW_H_INCLUDED
#define GUI_JVIEW_H_INCLUDED

#include "gui/jbase.h"

JWidget jview_new();

bool jview_has_bars(JWidget view);

void jview_attach(JWidget view, JWidget viewable_widget);
void jview_maxsize(JWidget view);
void jview_without_bars(JWidget view);

void jview_set_size(JWidget view, int w, int h);
void jview_set_scroll(JWidget view, int x, int y);
void jview_get_scroll(JWidget view, int *x, int *y);
void jview_get_max_size(JWidget view, int *w, int *h);

void jview_update(JWidget view);

JWidget jview_get_viewport(JWidget view);
JRect jview_get_viewport_position(JWidget view);

/* for themes */
void jtheme_scrollbar_info(JWidget scrollbar, int *pos, int *len);

/* for viewable widgets */
JWidget jwidget_get_view(JWidget viewable_widget);

#endif
