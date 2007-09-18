/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_ACCEL_H
#define JINETE_ACCEL_H

#include "jinete/base.h"

JI_BEGIN_DECLS

JAccel jaccel_new(void);
void jaccel_free(JAccel accel);

void jaccel_add_key(JAccel accel, int shifts, int ascii, int scancode);
void jaccel_add_keys_from_string(JAccel accel, const char *string);

bool jaccel_is_empty(JAccel accel);
void jaccel_to_string(JAccel accel, char *buf);

bool jaccel_check(JAccel accel, int shifts, int ascii, int scancode);

JI_END_DECLS

#endif /* JINETE_ACCEL_H */
