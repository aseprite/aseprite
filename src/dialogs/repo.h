/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#ifndef DIALOGS_REPO_H_INCLUDED
#define DIALOGS_REPO_H_INCLUDED

#include "gui/base.h"

class Button;

typedef struct RepoDlg {	/* a window to shows repositories
				   (used for mask/paths repositories,
				   and the bookmarks) */

  /**********************************************************************/
  /* from user */

  const char *config_section;
  const char *title;
  const char *use_text;

  /* fill the list box (you should add items to "repo_dlg->listbox") */
  void (*load_listbox)(struct RepoDlg *repo_dlg);

  /* when the dialog is closed (you should save the modified entries) */
  void (*save_listbox)(struct RepoDlg *repo_dlg);

  /* free the "user_data" of "repo_dlg->listitem", don't call
     jwidget_free(), it's called automatically */
  void (*free_listitem)(struct RepoDlg *repo_dlg);

  /* return false if to close the window, true to continue (use the
     "repo_dlg->listitem") */
  bool (*use_listitem)(struct RepoDlg *repo_dlg);
  bool (*add_listitem)(struct RepoDlg *repo_dlg, int *added);
  bool (*delete_listitem)(struct RepoDlg *repo_dlg, int *kill);

  void *user_data[4];

  /**********************************************************************/
  /* for the user */

  JWidget listbox;		/* list box */
  JWidget listitem;		/* selected list item */
  Button* button_use;		/* button for usage the selected item */
  Button* button_add;		/* "Add" button */
  Button* button_delete;	/* "Delete" button */
} RepoDlg;

void ji_show_repo_dlg(RepoDlg *repo_dlg);

#endif

