// ASE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gui/hook.h"

/**
 * Creates a new empty hook.
 *
 * You shouldn't use this routine, you should use jwidget_add_hook
 * instead.
 *
 * @see jhook
 */
JHook jhook_new()
{
  JHook hook = new jhook;

  hook->type = JI_WIDGET;
  hook->msg_proc = NULL;
  hook->data = NULL;

  return hook;
}

/**
 * Destroys the hook.
 *
 * @see jhook
 */
void jhook_free(JHook hook)
{
  delete hook;
}
