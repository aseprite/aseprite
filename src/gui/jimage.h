// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_JIMAGE_H_INCLUDED
#define GUI_JIMAGE_H_INCLUDED

#include "gui/jbase.h"

struct BITMAP;

JWidget jimage_new(struct BITMAP *bmp, int align);

#endif
