// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_ACCEL_H_INCLUDED
#define GUI_ACCEL_H_INCLUDED

#include "gui/base.h"

JAccel jaccel_new();
JAccel jaccel_new_copy(JAccel accel);
void jaccel_free(JAccel accel);

void jaccel_add_key(JAccel accel, int shifts, int ascii, int scancode);
void jaccel_add_keys_from_string(JAccel accel, const char *string);

bool jaccel_is_empty(JAccel accel);
void jaccel_to_string(JAccel accel, char *buf);

bool jaccel_check(JAccel accel, int shifts, int ascii, int scancode);
bool jaccel_check_from_key(JAccel accel);

#endif
