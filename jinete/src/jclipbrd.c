/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include "jinete/base.h"

static char *clipboard_text = NULL;

const char *jclipboard_get_text (void)
{
  return clipboard_text;
}

void jclipboard_set_text (const char *text)
{
  if (clipboard_text)
    jfree (clipboard_text);

  clipboard_text = text ? jstrdup (text) : NULL;
}
