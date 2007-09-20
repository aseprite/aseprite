/* jinete - a GUI library
 * Copyright (C) 2003-2005, 2007 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#ifndef JINETE_SYSTEM_H
#define JINETE_SYSTEM_H

#include "jinete/base.h"

struct BITMAP;

JI_BEGIN_DECLS

/***********************************************************************/
/* screen related */

#define JI_SCREEN_W ((ji_screen == screen) ? SCREEN_W: ji_screen->w)
#define JI_SCREEN_H ((ji_screen == screen) ? SCREEN_H: ji_screen->h)

extern struct BITMAP *ji_screen;

void ji_set_screen(struct BITMAP *bmp);

/***********************************************************************/
/* strings related */

void ji_set_translation_hook(const char *(*gettext) (const char *msgid));
const char *ji_translate_string(const char *msgid);

/***********************************************************************/
/* timer related */

#define JI_TICKS_PER_SEC	1024

extern int volatile ji_clock;

/***********************************************************************/
/* mouse related */

enum {
  JI_CLICK_NOT,
  JI_CLICK_START,
  JI_CLICK_RELEASE,
  JI_CLICK_AGAIN,
};

enum {
  JI_CURSOR_NULL,
  JI_CURSOR_NORMAL,
  JI_CURSOR_NORMAL_ADD,
  JI_CURSOR_HAND,
  JI_CURSOR_MOVE,
  JI_CURSOR_SIZE_TL,
  JI_CURSOR_SIZE_T,
  JI_CURSOR_SIZE_TR,
  JI_CURSOR_SIZE_L,
  JI_CURSOR_SIZE_R,
  JI_CURSOR_SIZE_BL,
  JI_CURSOR_SIZE_B,
  JI_CURSOR_SIZE_BR,
  JI_CURSORS
};

int ji_mouse_get_cursor(void);
int ji_mouse_set_cursor(int type);
void ji_mouse_draw_cursor();

bool ji_mouse_poll(void);
void ji_mouse_set_position(int x, int y);

int ji_mouse_b(int antique);
int ji_mouse_x(int antique);
int ji_mouse_y(int antique);
int ji_mouse_z(int antique);

bool ji_mouse_control_infinite_scroll(JRect rect);

int ji_mouse_get_click_button(void);
int ji_mouse_get_click_level(void);
void ji_mouse_set_click_level(int level);

JI_END_DECLS

#endif /* JINETE_SYSTEM_H */
