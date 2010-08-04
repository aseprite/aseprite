/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
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
 *   * Neither the name of the author nor the names of its contributors may
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

#include "config.h"

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
int ji_screen_w = 0;
int ji_screen_h = 0;

/* Global timer.  */

volatile int ji_clock = 0;

/* Hook to translate strings.  */

static const char *(*strings_hook)(const char *msgid) = NULL;

/* Current mouse cursor type.  */

static int m_cursor;
static BITMAP *sprite_cursor = NULL;
static int focus_x;
static int focus_y;

static BITMAP *covered_area = NULL;
static int covered_area_x;
static int covered_area_y;

/* Mouse information (button and position).  */

static volatile int m_b[2];
static int m_x[2];
static int m_y[2];
static int m_z[2];

static bool moved;
static int mouse_scares = 0;

/* Local routines.  */

static void set_cursor(BITMAP *bmp, int x, int y);
static void clock_inc();
static void update_mouse_position();

static void capture_covered_area();
static void restore_covered_area();

static void clock_inc()
{
  ji_clock++;
}

END_OF_STATIC_FUNCTION(clock_inc);

static void set_cursor(BITMAP *bmp, int x, int y)
{
  sprite_cursor = bmp;
  focus_x = x;
  focus_y = y;

  set_mouse_sprite(bmp);
  set_mouse_sprite_focus(x, y);
}

int _ji_system_init()
{
  /* Update screen pointer.  */
  ji_set_screen(screen, SCREEN_W, SCREEN_H);

  /* Install timer related stuff.  */
  LOCK_VARIABLE(ji_clock);
  LOCK_VARIABLE(m_b);
  LOCK_FUNCTION(clock_inc);

  if (install_int_ex(clock_inc, BPS_TO_TIMER(1000)) < 0)
    return -1;

  if (screen)
    jmouse_poll();

  moved = true;
  m_cursor = JI_CURSOR_NULL;

  return 0;
}

void _ji_system_exit()
{
  ji_set_screen(NULL, 0, 0);

  remove_int(clock_inc);
}

void ji_set_screen(BITMAP *bmp, int width, int height)
{
  int cursor = jmouse_get_cursor(); /* get mouse cursor */

  jmouse_set_cursor(JI_CURSOR_NULL);
  ji_screen = bmp;
  ji_screen_w = width;
  ji_screen_h = height;

  if (ji_screen != NULL) {
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

  ASSERT(ji_dirty_region != NULL);

  reg1 = jregion_new(rect, 1);
  jregion_union(ji_dirty_region, ji_dirty_region, reg1);
  jregion_free(reg1);
}

void ji_add_dirty_region(JRegion region)
{
  ASSERT(ji_dirty_region != NULL);

  jregion_union(ji_dirty_region, ji_dirty_region, region);
}

void ji_flip_dirty_region()
{
  int c, nrects;
  JRect rc;

  ASSERT(ji_dirty_region != NULL);

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
  ASSERT(ji_screen != screen);

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

void ji_set_translation_hook(const char *(*gettext)(const char *msgid))
{
  strings_hook = gettext;
}

const char *ji_translate_string(const char *msgid)
{
  if (strings_hook)
    return (*strings_hook)(msgid);
  else
    return msgid;
}

int jmouse_get_cursor()
{
  return m_cursor;
}

int jmouse_set_cursor(int type)
{
  if (m_cursor == type)
    return type;
  else {
    JTheme theme = ji_get_theme();
    int old = m_cursor;
    m_cursor = type;

    if (m_cursor == JI_CURSOR_NULL) {
      show_mouse(NULL);
      set_cursor(NULL, 0, 0);
    }
    else {
      show_mouse(NULL);

      {
	BITMAP *sprite;
	int x = 0;
	int y = 0;

	sprite = theme->set_cursor(type, &x, &y);
	set_cursor(sprite, x, y);
      }

      if (ji_screen == screen)
	show_mouse(ji_screen);
    }

    return old;
  }
}

/**
 * Use this routine if your "ji_screen" isn't Allegro's "screen" so
 * you must to draw the cursor by your self using this routine.
 */
void jmouse_draw_cursor()
{
#if 0
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
#endif

  if (sprite_cursor != NULL && mouse_scares == 0) {
    int x = m_x[0]-focus_x;
    int y = m_y[0]-focus_y;

    restore_covered_area();
    capture_covered_area();

    draw_sprite(ji_screen, sprite_cursor, x, y);

    if (ji_dirty_region) {
      JRect rect = jrect_new(x, y,
			     x+sprite_cursor->w,
			     y+sprite_cursor->h);
      ji_add_dirty_rect(rect);
      jrect_free(rect);
    }
  }
}

void jmouse_hide()
{
  ASSERT(mouse_scares >= 0);

  if (ji_screen == screen)
    scare_mouse();
  else if (mouse_scares == 0)
    restore_covered_area();

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
 * @return Returns true if the mouse moved.
 */
bool jmouse_poll()
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
    moved = true;
  }

  if (moved) {
    moved = false;
    return true;
  }
  else
    return false;
}

void jmouse_set_position(int x, int y)
{
  moved = true;

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
    return true;
  }
  else
    return false;
}

static void update_mouse_position()
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
      MapWindowPoints(win_get_window(), NULL, (LPPOINT)&rc, 2);

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

static void capture_covered_area()
{
  if (sprite_cursor != NULL && mouse_scares == 0) {
    ASSERT(covered_area == NULL);

    covered_area = create_bitmap(sprite_cursor->w, sprite_cursor->h);
    covered_area_x = m_x[0]-focus_x;
    covered_area_y = m_y[0]-focus_y;

    blit(ji_screen, covered_area,
	 covered_area_x, covered_area_y, 0, 0,
	 covered_area->w, covered_area->h);
  }
}

static void restore_covered_area()
{
  if (covered_area != NULL) {
    blit(covered_area, ji_screen,
	 0, 0, covered_area_x, covered_area_y,
	 covered_area->w, covered_area->h);

    destroy_bitmap(covered_area);
    covered_area = NULL;
  }
}
