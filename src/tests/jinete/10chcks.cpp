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

#include <allegro.h>
#include <stdio.h>

#include "jinete/jinete.h"

static JWidget check, radio[12];

static int radio_flags[] = {
  JI_LEFT, JI_CENTER, JI_RIGHT,
  JI_TOP, JI_MIDDLE, JI_BOTTOM
};

static void update_radios ();
static bool hooked_check_msg_proc (JWidget widget, JMessage msg);

int main (int argc, char *argv[])
{
  JWidget manager, window, box1, box2, box3, label1, label2;
  int c;

  allegro_init ();
  if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0) < 0) {
    if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
      allegro_message("%s\n", allegro_error);
      return 1;
    }
  }
  install_timer ();
  install_keyboard ();
  install_mouse ();

  manager = jmanager_new ();
  ji_set_standard_theme ();

  window = jwindow_new ("Example 10");
  box1 = jbox_new (JI_HORIZONTAL);
  box2 = jbox_new (JI_VERTICAL);
  box3 = jbox_new (JI_VERTICAL);
  label1 = jlabel_new ("Text:");
  label2 = jlabel_new ("Icon:");
  check = jcheck_new ("Check Button");
  radio[0] = jradio_new ("Left", 1);
  radio[1] = jradio_new ("Center", 1);
  radio[2] = jradio_new ("Right", 1);
  radio[3] = jradio_new ("Top", 2);
  radio[4] = jradio_new ("Middle", 2);
  radio[5] = jradio_new ("Bottom", 2);
  radio[6] = jradio_new ("Left", 3);
  radio[7] = jradio_new ("Center", 3);
  radio[8] = jradio_new ("Right", 3);
  radio[9] = jradio_new ("Top", 4);
  radio[10] = jradio_new ("Middle", 4);
  radio[11] = jradio_new ("Bottom", 4);

  for (c=0; c<12; c++)
    jwidget_add_hook(radio[c], JI_WIDGET, hooked_check_msg_proc, NULL);

  jwidget_add_child (window, box1);
  jwidget_add_child (box1, check);
  jwidget_add_child (box1, box2);
  jwidget_add_child (box1, box3);
  jwidget_add_child (box2, label1);
  jwidget_add_child (box2, radio[0]);
  jwidget_add_child (box2, radio[1]);
  jwidget_add_child (box2, radio[2]);
  jwidget_add_child (box2, radio[3]);
  jwidget_add_child (box2, radio[4]);
  jwidget_add_child (box2, radio[5]);
  jwidget_add_child (box3, label2);
  jwidget_add_child (box3, radio[6]);
  jwidget_add_child (box3, radio[7]);
  jwidget_add_child (box3, radio[8]);
  jwidget_add_child (box3, radio[9]);
  jwidget_add_child (box3, radio[10]);
  jwidget_add_child (box3, radio[11]);

  jwidget_expansive (check, true);
  update_radios ();

  window->open_window_bg();
  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN ();

static void update_radios ()
{
#define SET_STATUS(index, status)		\
  if (status)					\
    jwidget_select (radio[index]);		\
  else						\
    jwidget_deselect (radio[index]);

  int text_align = jwidget_get_align (check);
  int icon_align = ji_generic_button_get_icon_align(check);

  SET_STATUS (0, text_align & JI_LEFT);
  SET_STATUS (1, text_align & JI_CENTER);
  SET_STATUS (2, text_align & JI_RIGHT);
  SET_STATUS (3, text_align & JI_TOP);
  SET_STATUS (4, text_align & JI_MIDDLE);
  SET_STATUS (5, text_align & JI_BOTTOM);

  SET_STATUS (6, icon_align & JI_LEFT);
  SET_STATUS (7, icon_align & JI_CENTER);
  SET_STATUS (8, icon_align & JI_RIGHT);
  SET_STATUS (9, icon_align & JI_TOP);
  SET_STATUS (10, icon_align & JI_MIDDLE);
  SET_STATUS (11, icon_align & JI_BOTTOM);
}

static bool hooked_check_msg_proc (JWidget widget, JMessage msg)
{
  if (msg->type == JM_SIGNAL &&
      msg->signal.num == JI_SIGNAL_RADIO_CHANGE) {
    int text_align = 0;
    int icon_align = 0;
    int c;

    for (c=0; c<12; c++) {
      if (c < 6) {
	if (jwidget_is_selected(radio[c]))
	  text_align |= radio_flags[c];
      }
      else {
	if (jwidget_is_selected(radio[c]))
	  icon_align |= radio_flags[c-6];
      }
    }

    if (text_align != jwidget_get_align(check))
      check->set_align(text_align);

    if (icon_align != ji_generic_button_get_icon_align(check))
      ji_generic_button_set_icon_align(check, icon_align);

    return true;
  }
  else
    return false;
}
