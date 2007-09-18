/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_CLIPBRD_H
#define JINETE_CLIPBRD_H

#include "jinete/base.h"

JI_BEGIN_DECLS

const char *jclipboard_get_text(void);
void jclipboard_set_text(const char *text);

JI_END_DECLS

#endif /* JINETE_CLIPBRD_H */
