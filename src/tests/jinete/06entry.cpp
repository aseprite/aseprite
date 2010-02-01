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

#include "jinete/jinete.h"

int main (int argc, char *argv[])
{
  JWidget manager, window1, box1, box2, box3, box4, box5;
  JWidget label1, label2, entry1, entry2;
  JWidget button1, button2;

  allegro_init ();
  set_color_depth (8);
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

  window1 = jwindow_new ("Example 06");
  box1 = jbox_new (JI_VERTICAL);
  box2 = jbox_new (JI_HORIZONTAL);
  box3 = jbox_new (JI_VERTICAL | JI_HOMOGENEOUS);
  box4 = jbox_new (JI_VERTICAL | JI_HOMOGENEOUS);
  box5 = jbox_new (JI_HORIZONTAL | JI_HOMOGENEOUS);
  label1 = jlabel_new ("Entry1:");
  label2 = jlabel_new ("Entry2:");
  entry1 = jentry_new (8, "");
  entry2 = jentry_new (256, "%d+%d=%d", 4, 5, 4+5);
  button1 = jbutton_new ("&OK");
  button2 = jbutton_new ("&Cancel");

  jwidget_expansive (box4, true);
  jwidget_expansive (box5, true);
  jwidget_magnetic (button1, true);

  /* hierarchy */
  jwidget_add_child (window1, box1);
    jwidget_add_child (box1, box2);
      jwidget_add_child (box2, box3);
        jwidget_add_child (box3, label1);
        jwidget_add_child (box3, label2);
      jwidget_add_child (box2, box4);
        jwidget_add_child (box4, entry1);
        jwidget_add_child (box4, entry2);
    jwidget_add_child (box1, box5);
      jwidget_add_child (box5, button1);
      jwidget_add_child (box5, button2);

      window1->open_window_bg();
  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN();
