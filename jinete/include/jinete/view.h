/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_VIEW_H
#define JINETE_VIEW_H

#include "jinete/base.h"

JI_BEGIN_DECLS

JWidget jview_new(void);

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

JI_END_DECLS

#endif /* JINETE_VIEW_H */
