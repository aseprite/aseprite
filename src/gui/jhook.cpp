// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gui/jhook.h"

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
  JHook hook;

  hook = jnew(struct jhook, 1);
  if (!hook)
    return NULL;

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
  jfree(hook);
}

