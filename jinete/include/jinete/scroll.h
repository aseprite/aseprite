/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_SCROLL_H
#define JINETE_SCROLL_H

#include "jinete/base.h"

JI_BEGIN_DECLS

JWidget ji_scrollbar_new(int align);

void ji_scrollbar_set_range(JWidget scrollbar, int min, int max);
void ji_scrollbar_set_pagesize(JWidget scrollbar, int items);
void ji_scrollbar_set_itemsize(JWidget scrollbar, int pixels);
void ji_scrollbar_set_position(JWidget scrollbar, int item);

void ji_scrollbar_get_range(JWidget scrollbar, int *min, int *max);
int ji_scrollbar_get_pagesize(JWidget scrollbar);
int ji_scrollbar_get_itemsize(JWidget scrollbar);
int ji_scrollbar_get_position(JWidget scrollbar);

JI_END_DECLS

#endif /* JINETE_SCROLL_H */
