// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JALERT_H_INCLUDED
#define GUI_JALERT_H_INCLUDED

class Frame;

Frame* jalert_new(const char *format, ...);
int jalert(const char *format, ...);

#endif

