// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_SYSTEM_H_INCLUDED
#define GUI_SYSTEM_H_INCLUDED

#include "gui/base.h"

struct BITMAP;

/***********************************************************************/
/* screen related */

#define JI_SCREEN_W ji_screen_w
#define JI_SCREEN_H ji_screen_h

extern struct BITMAP *ji_screen;
extern JRegion ji_dirty_region;
extern int ji_screen_w;
extern int ji_screen_h;

void ji_set_screen(BITMAP *bmp, int width, int height);

void ji_add_dirty_rect(JRect rect);
void ji_add_dirty_region(JRegion region);
void ji_flip_dirty_region();
void ji_flip_rect(JRect rect);

/***********************************************************************/
/* timer related */

extern int volatile ji_clock;	/* in milliseconds */

/***********************************************************************/
/* mouse related */

enum {
  JI_CURSOR_NULL,
  JI_CURSOR_NORMAL,
  JI_CURSOR_NORMAL_ADD,
  JI_CURSOR_FORBIDDEN,
  JI_CURSOR_HAND,
  JI_CURSOR_SCROLL,
  JI_CURSOR_MOVE,
  JI_CURSOR_SIZE_TL,
  JI_CURSOR_SIZE_T,
  JI_CURSOR_SIZE_TR,
  JI_CURSOR_SIZE_L,
  JI_CURSOR_SIZE_R,
  JI_CURSOR_SIZE_BL,
  JI_CURSOR_SIZE_B,
  JI_CURSOR_SIZE_BR,
  JI_CURSOR_EYEDROPPER,
  JI_CURSORS
};

int jmouse_get_cursor();
int jmouse_set_cursor(int type);
void jmouse_draw_cursor();

void jmouse_hide();
void jmouse_show();

bool jmouse_is_hidden();
bool jmouse_is_shown();

bool jmouse_poll();
void jmouse_set_position(int x, int y);

void jmouse_capture();
void jmouse_release();

int jmouse_b(int antique);
int jmouse_x(int antique);
int jmouse_y(int antique);
int jmouse_z(int antique);

bool jmouse_control_infinite_scroll(JRect rect);

#endif
