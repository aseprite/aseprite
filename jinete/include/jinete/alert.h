/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_ALERT_H
#define JINETE_ALERT_H

#include "jinete/base.h"

JI_BEGIN_DECLS

JWidget jalert_new(const char *format, ...);
int jalert(const char *format, ...);

JI_END_DECLS

#endif /* JINETE_ALERT_H */

