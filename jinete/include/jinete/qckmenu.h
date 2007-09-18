/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_QCKMENU_H
#define JINETE_QCKMENU_H

#include "jinete/base.h"

JI_BEGIN_DECLS

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

JI_END_DECLS

#endif /* JINETE_QCKMENU_H */
