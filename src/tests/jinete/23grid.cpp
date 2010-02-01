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

int main(int argc, char *argv[])
{
  JWidget manager, window;

  allegro_init();
  if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0) < 0) {
    if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) < 0) {
      allegro_message("%s\n", allegro_error);
      return 1;
    }
  }
  install_timer();
  install_keyboard();
  install_mouse();

  manager = jmanager_new();
  ji_set_standard_theme();

  window = jwindow_new("GRID");
  {
    JWidget grid, label1, label2, label3, entry1, entry2, entry3;
    JWidget grid2, button1, button2;
    grid = jgrid_new(2, false);
    grid2 = jgrid_new(2, true);
    label1 = jlabel_new("A:");
    label2 = jlabel_new("BB:");
    label3 = jlabel_new("CCC:");
    entry1 = jentry_new(8, "");
    entry2 = jentry_new(256, "%d+%d=%d", 4, 5, 4+5);
    entry3 = jentry_new(8, "");
    button1 = jbutton_new("&OK");
    button2 = jbutton_new("&Cancel");

    jgrid_add_child(grid, label1, 1, 1, JI_RIGHT);
    jgrid_add_child(grid, entry1, 1, 1, JI_HORIZONTAL);
    jgrid_add_child(grid, label2, 1, 1, JI_RIGHT);
    jgrid_add_child(grid, entry2, 1, 1, JI_HORIZONTAL);
    jgrid_add_child(grid, label3, 1, 1, JI_RIGHT);
    jgrid_add_child(grid, entry3, 1, 1, JI_HORIZONTAL);
    
    jgrid_add_child(grid2, button1, 1, 1, JI_RIGHT | JI_HORIZONTAL);
    jgrid_add_child(grid2, button2, 1, 1, JI_RIGHT);
    jgrid_add_child(grid, grid2, 2, 1, 0);
    jwidget_add_child(window, grid);
  }

  window->open_window_bg();

  jmanager_run(manager);
  jmanager_free(manager);
  return 0;
}

END_OF_MAIN();
