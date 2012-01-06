// ASE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_HOOK_H_INCLUDED
#define GUI_HOOK_H_INCLUDED

#include "gui/base.h"

/**
 * A hook is a way to intercept messages which are sent to a @ref jwidget.
 *
 * Because each widget could has various hooks, each one has a unique
 * type-id (you can use the @ref ji_register_widget_type routine to
 * get one), and an associated routine to process messages
 * (MessageFunc).
 *
 * Also, the hook can have extra associated data to be used by the
 * routine @c msg_proc.
 *
 * @see ji_register_widget_type, MessageFunc
 */
struct jhook
{
  int type;
  MessageFunc msg_proc;
  void *data;
};

JHook jhook_new();
void jhook_free(JHook hook);

#endif
