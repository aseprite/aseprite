/* jinete - a GUI library
 * Copyright (C) 2003-2005, 2007 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include "jinete/hook.h"

/**
 * Creates a new empty hook.
 *
 * You shouldn't use this routine, you should use jwidget_add_hook
 * instead.
 *
 * @see jhook
 */
JHook jhook_new(void)
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

