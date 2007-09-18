/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>
#include <stdio.h>

#include "jinete.h"

void jwidget_add_tip (JWidget widget, const char *text);

int main (int argc, char *argv[])
{
  JWidget manager, window, box, check1, check2, button1, button2;

  allegro_init ();
  if (set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
    allegro_message ("%s\n", allegro_error);
    return 1;
  }
  install_timer ();
  install_keyboard ();
  install_mouse ();

  manager = jmanager_new ();
  ji_set_standard_theme ();

  window = jwindow_new ("Example 18");
  box = jbox_new (JI_VERTICAL | JI_HOMOGENEOUS);
  check1 = jcheck_new ("Check &1");
  check2 = jcheck_new ("Check &2");
  button1 = jbutton_new ("&OK");
  button2 = jbutton_new ("&Cancel");

  jwidget_add_tip (button1, "This is a tip for \"OK\" button");
  jwidget_add_tip (button2, "This is a tip for the \"Cancel\" button");

  jwidget_add_child (window, box);
  jwidget_add_child (box, check1);
  jwidget_add_child (box, check2);
  jwidget_add_child (box, button1);
  jwidget_add_child (box, button2);

  jwindow_open_bg (window);

  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN();

/**********************************************************************/
/* tip */

static int tip_type (void);
static bool tip_hook (JWidget widget, JMessage msg);

static JWidget tip_window_new (const char *text);
static bool tip_window_hook (JWidget widget, JMessage msg);

typedef struct TipData {
  int time;
  char *text;
} TipData;

void jwidget_add_tip(JWidget widget, const char *text)
{
  TipData *tip = jnew (TipData, 1);

  tip->time = -1;
  tip->text = jstrdup (text);

  jwidget_add_hook (widget, tip_type (), tip_hook, tip);
}

static int tip_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type ();
  return type;
}

static bool tip_hook(JWidget widget, JMessage msg)
{
  TipData *tip = jwidget_get_data (widget, tip_type ());

  switch (msg->type) {

    case JM_DESTROY:
      jfree (tip->text);
      jfree (tip);
      break;

    case JM_MOUSEENTER:
      tip->time = ji_clock;
      break;

    case JM_MOUSELEAVE:
      tip->time = -1;
      break;

    case JM_IDLE:
      if (tip->time >= 0 && ji_clock-tip->time > JI_TICKS_PER_SEC) {
	JWidget window = tip_window_new (tip->text);
	int w = jrect_w(window->rc);
	int h = jrect_h(window->rc);
	jwindow_remap (window);
	jwindow_position (window,
			    MID (0, ji_mouse_x (0)-w/2, JI_SCREEN_W-w),
			    MID (0, ji_mouse_y (0)-h/2, JI_SCREEN_H-h));
	jwindow_open (window);

	tip->time = -1;
      }
      break;
  }
  return FALSE;
}

static JWidget tip_window_new(const char *text)
{
  JWidget window = jwindow_new(text);
  JLink link, next;

  jwindow_sizeable(window, FALSE);
  jwindow_moveable(window, FALSE);

  jwidget_set_align(window, JI_CENTER | JI_MIDDLE);

  /* remove decorative widgets */
  JI_LIST_FOR_EACH_SAFE(window->children, link, next)
    jwidget_free(link->data);

  jwidget_add_hook(window, JI_WIDGET, tip_window_hook, NULL);
  jwidget_init_theme(window);

  return window;
}

static bool tip_window_hook(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_INIT_THEME) {
	widget->border_width.l = 3;
	widget->border_width.t = 3+jwidget_get_text_height (widget);
	widget->border_width.r = 3;
	widget->border_width.b = 3;
	return TRUE;
      }
      break;

    case JM_MOUSELEAVE:
    case JM_BUTTONPRESSED:
      jwindow_close(widget, NULL);
      break;

    case JM_DRAW: {
      JRect pos = jwidget_get_rect (widget);

      jdraw_rect(pos, makecol (0, 0, 0));

      jrect_shrink (pos, 1);
      jdraw_rectfill (pos, makecol (255, 255, 140));

      jdraw_widget_text (widget, makecol (0, 0, 0),
			   makecol (255, 255, 140), FALSE);
      return TRUE;
    }
  }
  return FALSE;
}
