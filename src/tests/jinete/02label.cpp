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

int main (int argc, char *argv[])
{
  JWidget manager, window, box, label1, label2, label3, label4;

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

  window = jwindow_new ("Example 02");
  box = jbox_new (JI_VERTICAL | JI_HOMOGENEOUS);
  label1 = jlabel_new ("&Left Top");
  label2 = jlabel_new ("&Center Middle");
  label3 = jlabel_new ("&Right Bottom");
  label4 = jlabel_new ("&Disabled");

  label1->set_align(JI_LEFT | JI_TOP);
  label2->set_align(JI_CENTER | JI_MIDDLE);
  label3->set_align(JI_RIGHT | JI_BOTTOM);
  label4->set_align(JI_CENTER | JI_BOTTOM);

  jwidget_disable (label4);

  jwidget_add_child (window, box);
  jwidget_add_child (box, label1);
  jwidget_add_child (box, label2);
  jwidget_add_child (box, label3);
  jwidget_add_child (box, label4);

  window->open_window_bg();

  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN();
