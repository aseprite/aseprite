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
  JWidget manager, window, box1, box2, button_ok, button_cancel;
  JWidget label1, label2, combobox1, combobox2;
  char buf[256];
  int c;

  /* Allegro stuff */
  allegro_init();
  if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0) < 0) {
    if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
      allegro_message("%s\n", allegro_error);
      return 1;
    }
  }
  install_timer();
  install_keyboard();
  install_mouse();

  /* Jinete initialization */
  manager = jmanager_new();
  ji_set_standard_theme();

  /* create main window */
  window = jwindow_new("Combo-boxes");
  box1 = jbox_new(JI_VERTICAL);
  box2 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
  label1 = jlabel_new("Non-editable");
  label2 = jlabel_new("Editable");
  combobox1 = jcombobox_new();
  combobox2 = jcombobox_new();
  button_ok = jbutton_new("&OK");
  button_cancel = jbutton_new("&Cancel");

  jwidget_expansive(label1, true);
  jwidget_expansive(label2, true);

  for (c=0; c<16; c++) {
    usprintf(buf, "Option %d", c);
    jcombobox_add_string(combobox1, buf, NULL);
  }

  jcombobox_add_string(combobox2, "First", NULL);
  jcombobox_add_string(combobox2, "Second", NULL);
  jcombobox_add_string(combobox2, "Third", NULL);
  jcombobox_add_string(combobox2, "Fourth", NULL);
  jcombobox_add_string(combobox2, "Fifth", NULL);
  jcombobox_add_string(combobox2, "Sixth", NULL);
  jcombobox_editable(combobox2, true);

  jwidget_add_child(window, box1);
  jwidget_add_children(box1, label1, combobox1, label2, combobox2, box2, 0);
  jwidget_add_children(box2, button_ok, button_cancel, 0);

  window->open_window_bg();

  jmanager_run(manager);
  jmanager_free(manager);
  return 0;
}

END_OF_MAIN();
