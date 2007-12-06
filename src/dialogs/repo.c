/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro/gfx.h>
#include <allegro/unicode.h>

#include "jinete/jinete.h"

#include "core/cfg.h"
#include "dialogs/repo.h"
#include "modules/gui.h"

#endif

static void fill_listbox (RepoDlg *repo_dlg);
static void kill_listbox (RepoDlg *repo_dlg);

static int repo_listbox_type (void);
static bool repo_listbox_msg_proc (JWidget widget, JMessage msg);

static void use_command (JWidget widget, void *data);
static void add_command (JWidget widget, void *data);
static void delete_command (JWidget widget, void *data);

void ji_show_repo_dlg (RepoDlg *repo_dlg)
{
  JWidget window, box1, box2, view, button_close;

  window = jwindow_new (repo_dlg->title);
  box1 = jbox_new (JI_HORIZONTAL);
  box2 = jbox_new (JI_VERTICAL);
  view = jview_new ();
  repo_dlg->listbox = jlistbox_new ();
  repo_dlg->button_use = jbutton_new (repo_dlg->use_text);
  repo_dlg->button_add = jbutton_new (_("&Add"));
  repo_dlg->button_delete = jbutton_new (_("&Delete"));
  button_close = jbutton_new (_("&Close"));

  jwidget_add_hook (repo_dlg->listbox, repo_listbox_type (),
		      repo_listbox_msg_proc, repo_dlg);

  jbutton_add_command_data (repo_dlg->button_use, use_command, repo_dlg);
  jbutton_add_command_data (repo_dlg->button_add, add_command, repo_dlg);
  jbutton_add_command_data (repo_dlg->button_delete, delete_command, repo_dlg);

  jwidget_magnetic(repo_dlg->button_use, TRUE);

  jwidget_expansive(view, TRUE);
  jview_attach(view, repo_dlg->listbox);
  jwidget_set_min_size(view, JI_SCREEN_W*25/100, JI_SCREEN_H*25/100);

  /* fill the list */
  fill_listbox (repo_dlg);
  jlistbox_select_index (repo_dlg->listbox, 0);

  /* no items? */
  if (jlistbox_get_selected_index (repo_dlg->listbox) != 0) {
    jwidget_disable (repo_dlg->button_use);
    jwidget_disable (repo_dlg->button_delete);
  }

  /* hierarchy */
  jwidget_add_child (box2, repo_dlg->button_use);
  jwidget_add_child (box2, repo_dlg->button_add);
  jwidget_add_child (box2, repo_dlg->button_delete);
  jwidget_add_child (box2, button_close);
  jwidget_add_child (box1, box2);
  jwidget_add_child (box1, view);
  jwidget_add_child (window, box1);

  /* default position */
  jwindow_remap (window);
  jwindow_center (window);

  /* load window configuration */
  load_window_pos (window, repo_dlg->config_section);

  /* open window */
  jwindow_open_fg (window);

  /* kill the list */
  kill_listbox (repo_dlg);

  /* save window configuration */
  save_window_pos (window, repo_dlg->config_section);

  jwidget_free (window);
}

static void fill_listbox (RepoDlg *repo_dlg)
{
  /* no item selected (ID=0) */
  repo_dlg->listitem = 0;

  /* load the entries */
  if (repo_dlg->load_listbox)
    (*repo_dlg->load_listbox) (repo_dlg);

  jview_update (jwidget_get_view (repo_dlg->listbox));
}

static void kill_listbox (RepoDlg *repo_dlg)
{
  JWidget listbox = repo_dlg->listbox;
  JWidget listitem;
  JLink link, next;

  /* save the entries */
  if (repo_dlg->save_listbox)
    (*repo_dlg->save_listbox) (repo_dlg);

  /* remove all items */
  JI_LIST_FOR_EACH_SAFE(listbox->children, link, next) {
    listitem = link->data;

    jwidget_remove_child (repo_dlg->listbox, listitem);

    if (repo_dlg->free_listitem) {
      repo_dlg->listitem = listitem;
      (*repo_dlg->free_listitem) (repo_dlg);
    }

    jwidget_free (listitem);
  }
}

static int repo_listbox_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static bool repo_listbox_msg_proc(JWidget widget, JMessage msg)
{
  RepoDlg *repo_dlg = jwidget_get_data(widget, repo_listbox_type ());

  switch (msg->type) {

    case JM_DESTROY:
      /* do nothing (don't free repo_dlg) */
      break;

    case JM_SIGNAL:
      switch (msg->signal.num) {

	case JI_SIGNAL_LISTBOX_CHANGE:
	  repo_dlg->listitem = jlistbox_get_selected_child(widget);

	  jwidget_enable(repo_dlg->button_use);
	  jwidget_enable(repo_dlg->button_delete);
	  break;

	case JI_SIGNAL_LISTBOX_SELECT:
	  if (repo_dlg->use_listitem)
	    if (!(*repo_dlg->use_listitem)(repo_dlg))
	      jwidget_close_window(widget);
	  break;
      }
      break;
  }

  return FALSE;
}

static void use_command(JWidget widget, void *data)
{
  RepoDlg *repo_dlg = (RepoDlg *)data;

  if (repo_dlg->use_listitem) {
    if (!(*repo_dlg->use_listitem)(repo_dlg))
      jwidget_close_window(widget);
  }
}

static void add_command(JWidget widget, void *data)
{
  RepoDlg *repo_dlg = (RepoDlg *)data;
  int ret, added;

  if (repo_dlg->add_listitem) {
    added = FALSE;
    ret = (*repo_dlg->add_listitem)(repo_dlg, &added);

    if (added) {
      /* update the list-box */
      jview_update(jwidget_get_view(repo_dlg->listbox));

      /* select the last item */
      jlistbox_select_index (repo_dlg->listbox,
			       jlist_length(repo_dlg->listbox->children)-1);
    }

    if (!ret)
      jwidget_close_window(widget);
  }
}

static void delete_command(JWidget widget, void *data)
{
  RepoDlg *repo_dlg = (RepoDlg *)data;
  int ret, kill, index, last;

  if (repo_dlg->delete_listitem) {
    kill = FALSE;
    ret = (*repo_dlg->delete_listitem) (repo_dlg, &kill);

    if (kill) {
      index = jlistbox_get_selected_index (repo_dlg->listbox);

      /* delete "repo_dlg->listitem" */
      jwidget_remove_child (repo_dlg->listbox, repo_dlg->listitem);

      if (repo_dlg->free_listitem)
	(*repo_dlg->free_listitem) (repo_dlg);

      jwidget_free (repo_dlg->listitem);

      /* disable buttons */
      if (!repo_dlg->listbox->children) {
	jwidget_disable (repo_dlg->button_use);
	jwidget_disable (repo_dlg->button_delete);
      }
      /* select other item */
      else {
	last = jlist_length (repo_dlg->listbox->children)-1;

	jlistbox_select_index (repo_dlg->listbox,
				 index > last ? last: index);
      }

      /* update the list-box */
      jview_update (jwidget_get_view (repo_dlg->listbox));
    }

    if (!ret)
      jwidget_close_window (widget);
  }
}
