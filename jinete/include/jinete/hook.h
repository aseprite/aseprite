/* jinete - a GUI library
 * Copyright (C) 2003-2005, 2007 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_HOOK_H
#define JINETE_HOOK_H

#include "jinete/base.h"

JI_BEGIN_DECLS

/**
 * A hook is a way to intercept messages which are sent to a @ref jwidget.
 *
 * Because each widget could has various hooks, each one has a unique
 * type-id (you can use the @ref ji_register_widget_type routine to
 * get one), and an associated routine to process messages
 * (JMessageFunc).
 *
 * Also, the hook can have extra associated data to be used by the
 * routine @c msg_proc.
 *
 * @see ji_register_widget_type, JMessageFunc
 */
struct jhook
{
  int type;
  JMessageFunc msg_proc;
  void *data;
};

JHook jhook_new(void);
void jhook_free(JHook hook);

JI_END_DECLS

#endif /* JINETE_HOOK_H */
