// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JTHREAD_H_INCLUDED
#define GUI_JTHREAD_H_INCLUDED

#include "gui/jbase.h"

JThread jthread_new(void (*proc)(void *data), void *data);
void jthread_join(JThread thread);

#endif
