/* jinete - a GUI library
 * Copyright (C) 2003-2005, 2007 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>

#include "jinete/intern.h"
#include "jinete/manager.h"
#include "jinete/rect.h"
#include "jinete/system.h"
#include "jinete/theme.h"
#include "jinete/widget.h"

/* Global output bitmap.  */

BITMAP *ji_screen = NULL;

/* Global timer.  */

volatile int ji_clock = 0;

/* Hook to translate strings.  */

static const char *(*strings_hook)(const char *msgid) = NULL;

/* Current mouse cursor type.  */

static int m_cursor;

/* Mouse information (button and position).  */

static volatile int m_b[2];
static int m_x[2];
static int m_y[2];
static int m_z[2];

static bool moved;

/* For double click management.  */

static volatile int click_clock = 0;
static volatile int click_level = JI_CLICK_NOT;
static volatile int click_mouse_b = 0;

/* Local routines.  */

static void clock_inc(void);
static void check_click(void);

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

END_OF_STATIC_FUNCTION (check_click);

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

  ji_mouse_poll();

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
  int cursor = ji_mouse_get_cursor(); /* get mouse cursor */

  ji_mouse_set_cursor(JI_CURSOR_NULL);
  ji_screen = bmp;

  if(ji_screen) {
    JWidget manager = ji_get_default_manager();

    /* update default-manager size */
    if (manager && (jrect_w(manager->rc) != JI_SCREEN_W ||
		    jrect_h(manager->rc) != JI_SCREEN_H)) {
      JRect rect = jrect_new(0, 0, JI_SCREEN_W, JI_SCREEN_H);
      jwidget_set_rect(manager, rect);
      jrect_free(rect);
    }

    ji_mouse_set_cursor(cursor);	/* restore mouse cursor */
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

int ji_mouse_get_cursor(void)
{
  return m_cursor;
}

int ji_mouse_set_cursor(int type)
{
  JTheme theme = ji_get_theme ();
  int old = m_cursor;

  m_cursor = type;

  if (m_cursor == JI_CURSOR_NULL) {
    show_mouse (NULL);
    set_mouse_sprite (NULL);
  }
  else {
    show_mouse (NULL);
    if (theme->set_cursor)
      (*theme->set_cursor) (m_cursor);
    show_mouse (ji_screen);
  }

  return old;
}

/* Returns TRUE if the mouse moved.  */

bool ji_mouse_poll(void)
{
  m_b[1] = m_b[0];
  m_x[1] = m_x[0];
  m_y[1] = m_y[0];
  m_z[1] = m_z[0];

  poll_mouse ();

  m_b[0] = mouse_b;
  m_x[0] = mouse_x;
  m_y[0] = mouse_y;
  m_z[0] = mouse_z;

  if ((m_x[0] != m_x[1]) || (m_y[0] != m_y[1])) {
    poll_mouse ();
    m_x[0] = mouse_x;
    m_y[0] = mouse_y;
    moved = TRUE;
  }

  if (moved) {
    moved = FALSE;
    return TRUE;
  }
  else
    return FALSE;
}

void ji_mouse_set_position(int x, int y)
{
  moved = TRUE;
  position_mouse(m_x[0] = m_x[1] = x,
		 m_y[0] = m_y[1] = y);
}

int ji_mouse_x(int antique) { return m_x[antique & 1]; }
int ji_mouse_y(int antique) { return m_y[antique & 1]; }
int ji_mouse_z(int antique) { return m_z[antique & 1]; }
int ji_mouse_b(int antique) { return m_b[antique & 1]; }

bool ji_mouse_control_infinite_scroll(JRect rect)
{
  int x, y, u, v;

  u = ji_mouse_x(0);
  v = ji_mouse_y(0);

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
    ji_mouse_set_position(x, y);
    return TRUE;
  }
  else
    return FALSE;
}

int ji_mouse_get_click_button(void)
{
  return click_mouse_b;
}

int ji_mouse_get_click_level(void)
{
  return click_level;
}

void ji_mouse_set_click_level(int level)
{
  click_level = level;
  if (level == JI_CLICK_START) {
    click_clock = 0;
    click_mouse_b = m_b[0];
  }
}
