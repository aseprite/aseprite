/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_SLIDER_H
#define JINETE_SLIDER_H

#include "jinete/base.h"

JI_BEGIN_DECLS

JWidget jslider_new(int min, int max, int value);

void jslider_set_range(JWidget slider, int min, int max);

void jslider_set_value(JWidget slider, int value);
int jslider_get_value(JWidget slider);

/* for themes */
void jtheme_slider_info(JWidget slider, int *min, int *max, int *value);

JI_END_DECLS

#endif /* JINETE_SLIDER_H */
