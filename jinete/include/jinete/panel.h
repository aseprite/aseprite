/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_PANEL_H
#define JINETE_PANEL_H

#include "jinete/base.h"

JI_BEGIN_DECLS

JWidget ji_panel_new(int align);

double ji_panel_get_pos(JWidget panel);
void ji_panel_set_pos(JWidget panel, double pos);

JI_END_DECLS

#endif /* JINETE_PANEL_H */
