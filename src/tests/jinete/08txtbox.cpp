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

void do_text(const char *title, int align)
{
  const char *LGPL_text =
    "This library is free software; you can redistribute it and/or\n"
    "modify it under the terms of the GNU Lesser General Public\n"
    "License as published by the Free Software Foundation; either\n"
    "version 2.1 of the License, or (at your option) any later version.\n"
    "\n"
    "This library is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
    "Lesser General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU Lesser General Public\n"
    "License along with this library; if not, write to the Free Software\n"
    "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA";

  JWidget window, box1, box2, box3, box4, label1, label2;
  JWidget view1, view2, text_box1, text_box2, button;

  window = jwindow_new(title);
  box1 = jbox_new(JI_VERTICAL);
  box2 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
  box3 = jbox_new(JI_VERTICAL);
  box4 = jbox_new(JI_VERTICAL);
  label1 = jlabel_new("With word-wrap");
  label2 = jlabel_new("Without word-wrap");
  view1 = jview_new();
  view2 = jview_new();
  text_box1 = jtextbox_new(LGPL_text, align | JI_WORDWRAP);
  text_box2 = jtextbox_new(LGPL_text, align | 0);
  button = jbutton_new("&Close");

  jview_attach(view1, text_box1);
  jview_attach(view2, text_box2);

  jwidget_expansive(view1, true);
  jwidget_expansive(view2, true);
  jwidget_expansive(box2, true);

  jwidget_set_min_size(view1, 64, 64);
  jwidget_set_min_size(view2, 64, 64);

  jwidget_add_child(box3, label1);
  jwidget_add_child(box3, view1);
  jwidget_add_child(box4, label2);
  jwidget_add_child(box4, view2);
  jwidget_add_child(box2, box3);
  jwidget_add_child(box2, box4);
  jwidget_add_child(box1, box2);
  jwidget_add_child(box1, button);
  jwidget_add_child(window, box1);

  window->open_window_bg();
}

int main(int argc, char *argv[])
{
  JWidget manager;

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

  manager = jmanager_new();
  ji_set_standard_theme();

  do_text("Right", JI_RIGHT);
  do_text("Center", JI_CENTER);
  do_text("Left", JI_LEFT);

  jmanager_run(manager);
  jmanager_free(manager);
  return 0;
}

END_OF_MAIN();
