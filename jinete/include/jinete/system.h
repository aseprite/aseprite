/* Jinete - a GUI library
 * Copyright (c) 2003, 2004, 2005, 2007, David A. Capello
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the Jinete nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

int jmouse_get_cursor(void);
int jmouse_set_cursor(int type);
void jmouse_draw_cursor();

void jmouse_hide();
void jmouse_show();

bool jmouse_is_hidden();
bool jmouse_is_shown();

bool jmouse_poll(void);
void jmouse_set_position(int x, int y);

int jmouse_b(int antique);
int jmouse_x(int antique);
int jmouse_y(int antique);
int jmouse_z(int antique);

bool jmouse_control_infinite_scroll(JRect rect);

int jmouse_get_click_button(void);
int jmouse_get_click_level(void);
void jmouse_set_click_level(int level);

JI_END_DECLS

#endif /* JINETE_SYSTEM_H */
