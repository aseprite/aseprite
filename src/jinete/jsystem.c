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

#include <assert.h>
#include <allegro.h>
#ifdef ALLEGRO_WINDOWS
#include <winalleg.h>
#endif

#include "jinete/jintern.h"
#include "jinete/jmanager.h"
#include "jinete/jrect.h"
#include "jinete/jregion.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"

/* Global output bitmap.  */

BITMAP *ji_screen = NULL;
JRegion ji_dirty_region = NULL;

/* Global timer.  */

volatile int ji_clock = 0;

/* Hook to translate strings.  */

static const char *(*strings_hook)(const char *msgid) = NULL;

/* Current mouse cursor type.  */

static int m_cursor;
static BITMAP *sprite_cursor = NULL;
static int focus_x;
static int focus_y;

/* Mouse information (button and position).  */

static volatile int m_b[2];
static int m_x[2];
static int m_y[2];
static int m_z[2];

static bool moved;
static int mouse_scares = 0;

/* For double click management.  */

static volatile int click_clock = 0;
static volatile int click_level = JI_CLICK_NOT;
static volatile int click_mouse_b = 0;

/* Local routines.  */

static void set_cursor(BITMAP *bmp, int x, int y);
static void clock_inc(void);
static void check_click(void);
static void update_mouse_position(void);

static void clock_inc(void)
{
  ji_clock++;
}

END_OF_STATIC_FUNCTION(clock_inc);

/* Based on "dclick_check" from allegro/src/gui.c.  */

static void check_click(void)
{
  /* Waiting mouse released...  */
  if (click_level == JI_CLICK_START) {
    /* The button was released.  */
    if (!m_b[0]) {
      click_clock = 0;
      click_level = JI_CLICK_RELEASE;
    }
    /* The button continue pressed.  */
    else {
      /* Does mouse button change?  */
      if (click_mouse_b != m_b[0]) {
	/* Start again with this new button.  */
	click_clock = 0;
	click_level = JI_CLICK_START;
	click_mouse_b = m_b[0];
      }
      else
	click_clock++;
    }
  }
  /* Waiting second mouse click...  */
  else if (click_level == JI_CLICK_RELEASE) {
    /* The button is pressed again.  */
    if (m_b[0]) {
      /* Is the same button?  */
      if (m_b[0] == click_mouse_b) {
	click_level = JI_CLICK_AGAIN;
      }
      /* If it's other button, start again with this one.  */
      else {
	click_clock = 0;
	click_level = JI_CLICK_START;
	click_mouse_b = m_b[0];
      }
    }
    else
      click_clock++;
  }

  if (click_clock > 10)
    click_level = JI_CLICK_NOT;
}

END_OF_STATIC_FUNCTION(check_click);

static void set_cursor(BITMAP *bmp, int x, int y)
{
  sprite_cursor = bmp;
  focus_x = x;
  focus_y = y;

  set_mouse_sprite(bmp);
  set_mouse_sprite_focus(x, y);
}

int _ji_system_init(void)
{
  /* Update screen pointer.  */
  ji_set_screen(screen);

  /* Install timer related stuff.  */
  LOCK_VARIABLE(ji_clock);
  LOCK_VARIABLE(m_b);
  LOCK_VARIABLE(click_clock);
  LOCK_VARIABLE(click_level);
  LOCK_VARIABLE(click_mouse_b);
  LOCK_FUNCTION(clock_inc);
  LOCK_FUNCTION(check_click);

  if ((install_int_ex(clock_inc, BPS_TO_TIMER(JI_TICKS_PER_SEC)) < 0) ||
      (install_int(check_click, 20) < 0))
    return -1;

  jmouse_poll();

  moved = TRUE;
  m_cursor = JI_CURSOR_NULL;

  return 0;
}

void _ji_system_exit(void)
{
  ji_set_screen(NULL);

  remove_int(check_click);
  remove_int(clock_inc);
}

void ji_set_screen(BITMAP *bmp)
{
  int cursor = jmouse_get_cursor(); /* get mouse cursor */

  jmouse_set_cursor(JI_CURSOR_NULL);
  ji_screen = bmp;

  if (ji_screen) {
    JWidget manager = ji_get_default_manager();

    /* update default-manager size */
    if (manager && (jrect_w(manager->rc) != JI_SCREEN_W ||
		    jrect_h(manager->rc) != JI_SCREEN_H)) {
      JRect rect = jrect_new(0, 0, JI_SCREEN_W, JI_SCREEN_H);
      jwidget_set_rect(manager, rect);
      jrect_free(rect);

      if (ji_dirty_region)
	jregion_reset(ji_dirty_region, manager->rc);
    }

    jmouse_set_cursor(cursor);	/* restore mouse cursor */
  }
}

void ji_add_dirty_rect(JRect rect)
{
  JRegion reg1;

  assert(ji_dirty_region != NULL);

  reg1 = jregion_new(rect, 1);
  jregion_union(ji_dirty_region, ji_dirty_region, reg1);
  jregion_free(reg1);
}

void ji_add_dirty_region(JRegion region)
{
  assert(ji_dirty_region != NULL);

  jregion_union(ji_dirty_region, ji_dirty_region, region);
}

void ji_flip_dirty_region(void)
{
  int c, nrects;
  JRect rc;

  assert(ji_dirty_region != NULL);

  nrects = JI_REGION_NUM_RECTS(ji_dirty_region);

  if (nrects == 1) {
    rc = JI_REGION_RECTS(ji_dirty_region);
    ji_flip_rect(rc);
  }
  else if (nrects > 1) {
    for (c=0, rc=JI_REGION_RECTS(ji_dirty_region);
	 c<nrects;
	 c++, rc++)
      ji_flip_rect(rc);
  }

  jregion_empty(ji_dirty_region);
}

void ji_flip_rect(JRect rect)
{
  assert(ji_screen != screen);

  if (JI_SCREEN_W == SCREEN_W && JI_SCREEN_H == SCREEN_H) {
    blit(ji_screen, screen,
	 rect->x1, rect->y1,
	 rect->x1, rect->y1,
	 jrect_w(rect),
	 jrect_h(rect));
  }
  else {
    stretch_blit(ji_screen, screen,
		 rect->x1,
		 rect->y1,
		 jrect_w(rect),
		 jrect_h(rect),
		 rect->x1 * SCREEN_W / JI_SCREEN_W,
		 rect->y1 * SCREEN_H / JI_SCREEN_H,
		 jrect_w(rect) * SCREEN_W / JI_SCREEN_W,
		 jrect_h(rect) * SCREEN_H / JI_SCREEN_H);
  }
}

void ji_set_translation_hook(const char *(*gettext) (const char *msgid))
{
  strings_hook = gettext;
}

const char *ji_translate_string(const char *msgid)
{
  if (strings_hook)
    return (*strings_hook) (msgid);
  else
    return msgid;
}

int jmouse_get_cursor(void)
{
  return m_cursor;
}

int jmouse_set_cursor(int type)
{
  JTheme theme = ji_get_theme();
  int old = m_cursor;
  m_cursor = type;

  if (m_cursor == JI_CURSOR_NULL) {
    show_mouse(NULL);
    set_cursor(NULL, 0, 0);
  }
  else {
    show_mouse(NULL);

    if (theme->set_cursor) {
      BITMAP *sprite;
      int x = 0;
      int y = 0;

      sprite = (*theme->set_cursor)(type, &x, &y);
      set_cursor(sprite, x, y);
    }

    if (ji_screen == screen)
      show_mouse(ji_screen);
  }

  return old;
}

/**
 * Use this routine if your "ji_screen" isn't Allegro's "screen" so
 * you must to draw the cursor by your self using this routine.
 */
void jmouse_draw_cursor()
{
  if (sprite_cursor != NULL && mouse_scares == 0) {
    int x = m_x[0]-focus_x;
    int y = m_y[0]-focus_y;
    JRect rect = jrect_new(x, y,
			   x+sprite_cursor->w,
			   y+sprite_cursor->h);

    jwidget_invalidate_rect(ji_get_default_manager(), rect);
    /* rectfill(ji_screen, rect->x1, rect->y1, rect->x2-1, rect->y2-1, makecol(0, 0, 255)); */
    draw_sprite(ji_screen, sprite_cursor, x, y);

    if (ji_dirty_region)
      ji_add_dirty_rect(rect);

    jrect_free(rect);
  }
}

void jmouse_hide()
{
  ASSERT(mouse_scares >= 0);
  if (ji_screen == screen)
    scare_mouse();
  mouse_scares++;
}

void jmouse_show()
{
  ASSERT(mouse_scares > 0);
  mouse_scares--;
  if (ji_screen == screen)
    unscare_mouse();
}

bool jmouse_is_hidden()
{
  ASSERT(mouse_scares >= 0);
  return mouse_scares > 0;
}

bool jmouse_is_shown()
{
  ASSERT(mouse_scares >= 0);
  return mouse_scares == 0;
}

/**
 * Updates the mouse information (position, wheel and buttons).
 * 
 * @return Returns TRUE if the mouse moved.
 */
bool jmouse_poll(void)
{
  m_b[1] = m_b[0];
  m_x[1] = m_x[0];
  m_y[1] = m_y[0];
  m_z[1] = m_z[0];

  poll_mouse();

  m_b[0] = mouse_b;
  m_z[0] = mouse_z;

  update_mouse_position();

  if ((m_x[0] != m_x[1]) || (m_y[0] != m_y[1])) {
    poll_mouse();
    update_mouse_position();
    moved = TRUE;
  }

  if (moved) {
    moved = FALSE;
    return TRUE;
  }
  else
    return FALSE;
}

void jmouse_set_position(int x, int y)
{
  moved = TRUE;

  m_x[0] = m_x[1] = x;
  m_y[0] = m_y[1] = y;

  if (ji_screen == screen) {
    position_mouse(x, y);
  }
  else {
    position_mouse(SCREEN_W * x / JI_SCREEN_W,
		   SCREEN_H * y / JI_SCREEN_H);
  }
}

int jmouse_x(int antique) { return m_x[antique & 1]; }
int jmouse_y(int antique) { return m_y[antique & 1]; }
int jmouse_z(int antique) { return m_z[antique & 1]; }
int jmouse_b(int antique) { return m_b[antique & 1]; }

bool jmouse_control_infinite_scroll(JRect rect)
{
  int x, y, u, v;

  u = jmouse_x(0);
  v = jmouse_y(0);

  if (u <= rect->x1)
    x = rect->x2-2;
  else if (u >= rect->x2-1)
    x = rect->x1+1;
  else
    x = u;

  if (v <= rect->y1)
    y = rect->y2-2;
  else if (v >= rect->y2-1)
    y = rect->y1+1;
  else
    y = v;

  if ((x != u) || (y != v)) {
    jmouse_set_position(x, y);
    return TRUE;
  }
  else
    return FALSE;
}

int jmouse_get_click_button(void)
{
  return click_mouse_b;
}

int jmouse_get_click_level(void)
{
  return click_level;
}

void jmouse_set_click_level(int level)
{
  click_level = level;
  if (level == JI_CLICK_START) {
    click_clock = 0;
    click_mouse_b = m_b[0];
  }
}

static void update_mouse_position(void)
{
  if (ji_screen == screen) {
    m_x[0] = mouse_x;
    m_y[0] = mouse_y;
  }
  else {
    m_x[0] = JI_SCREEN_W * mouse_x / SCREEN_W;
    m_y[0] = JI_SCREEN_H * mouse_y / SCREEN_H;
  }

#ifdef ALLEGRO_WINDOWS
  /* this help us (in windows) to get mouse feedback when we capture
     the mouse but we are outside the Allegro window */
  if (is_windowed_mode()) {
    POINT pt;
    RECT rc;

    if (GetCursorPos(&pt) && GetClientRect(win_get_window(), &rc)) {
      MapWindowPoints(win_get_window(), NULL, (POINT *)&rc, 2);

      if (!PtInRect(&rc, pt)) {
	/* if the mouse is free we can hide the cursor putting the
	   mouse outside the screen (right-bottom corder) */
	if (!jmanager_get_capture()) {
	  m_x[0] = JI_SCREEN_W+focus_x;
	  m_y[0] = JI_SCREEN_H+focus_y;
	}
	/* if the mouse is captured we can put it in the edges of the screen */
	else {
	  pt.x -= rc.left;
	  pt.y -= rc.top;

	  if (ji_screen == screen) {
	    m_x[0] = pt.x;
	    m_y[0] = pt.y;
	  }
	  else {
	    m_x[0] = JI_SCREEN_W * pt.x / SCREEN_W;
	    m_y[0] = JI_SCREEN_H * pt.y / SCREEN_H;
	  }

	  m_x[0] = MID(0, m_x[0], JI_SCREEN_W-1);
	  m_y[0] = MID(0, m_y[0], JI_SCREEN_H-1);
	}
      }
    }
  }
#endif
}

