/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <allegro.h>

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/core.h"
#include "core/dirs.h"
#include "modules/gui.h"

static char *read_authors_txt(const char *filename);

static void cmd_about_execute(const char *argument)
{
  JWidget box1, label1, label2, separator1;
  JWidget textbox, view, separator2;
  JWidget label3, label4, box2, box3, box4, button1;
  char *authors_txt = read_authors_txt("AUTHORS.txt");

  JWidgetPtr window;
  if (authors_txt)
    window = jwindow_new_desktop();
  else
    window = jwindow_new(_("About ASE"));

  box1 = jbox_new(JI_VERTICAL);
  label1 = jlabel_new("Allegro Sprite Editor - " VERSION);
  label2 = jlabel_new(_("Just Another Tool to Create Sprites"));
  separator1 = ji_separator_new(NULL, JI_HORIZONTAL);

  if (authors_txt) {
    view = jview_new();
    textbox = jtextbox_new(authors_txt, JI_LEFT);
    separator2 = ji_separator_new(NULL, JI_HORIZONTAL);
    jfree(authors_txt);
  }
  else {
    view = textbox = separator2 = NULL;
  }

  label3 = jlabel_new(COPYRIGHT);
  label4 = jlabel_new(WEBSITE);
  box2 = jbox_new(JI_HORIZONTAL);
  box3 = jbox_new(JI_HORIZONTAL);
  box4 = jbox_new(JI_HORIZONTAL);
  button1 = jbutton_new(_("&Close"));
  
  jwidget_magnetic(button1, TRUE);

  jwidget_set_border(box1, 4, 4, 4, 4);
  jwidget_add_children(box1, label1, label2, separator1, NULL);
  if (textbox) {
    jview_attach(view, textbox);
    jwidget_expansive(view, TRUE);
    jwidget_set_min_size(view, JI_SCREEN_W/3, JI_SCREEN_H/4);
    jwidget_add_children(box1, view, separator2, NULL);
  }
  jwidget_expansive(box3, TRUE);
  jwidget_expansive(box4, TRUE);
  jwidget_add_children(box2, box3, button1, box4, NULL);
  jwidget_add_children(box1, label3, label4, NULL);
  jwidget_add_child(box1, box2);
  jwidget_add_child(window, box1);

  jwidget_set_border(button1,
		     button1->border_width.l+16,
		     button1->border_width.t,
		     button1->border_width.r+16,
		     button1->border_width.b);

  jwindow_open_fg(window);
}

static char *read_authors_txt(const char *filename)
{
  DIRS *dirs, *dir;
  char *txt = NULL;

  dirs = filename_in_bindir(filename);
  dirs_cat_dirs(dirs, filename_in_datadir(filename));

  /* search the configuration file from first to last path */
  for (dir=dirs; dir && !txt; dir=dir->next) {
    PRINTF("Triying opening %s\n", dir->path);
    if ((dir->path) && exists(dir->path)) {
      int size;
      FILE *f;

#if (MAKE_VERSION(4, 2, 1) >= MAKE_VERSION(ALLEGRO_VERSION,		\
					   ALLEGRO_SUB_VERSION,		\
					   ALLEGRO_WIP_VERSION))
      size = file_size(dir->path);
#else
      size = file_size_ex(dir->path);
#endif

      if (size > 0) {
	f = fopen(dir->path, "r");
	if (f) {
	  txt = (char *)jmalloc0(size+2);
	  fread(txt, 1, size, f);
	  fclose(f);
	}
      }
    }
  }

  dirs_free(dirs);
  return txt;
}

Command cmd_about = {
  CMD_ABOUT,
  NULL,
  NULL,
  cmd_about_execute,
};
