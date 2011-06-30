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

#include "config.h"

#include <allegro/gfx.h>
#include <allegro/unicode.h>

#include "base/bind.h"
#include "gui/gui.h"

#include "dialogs/repo.h"
#include "ini_file.h"
#include "modules/gui.h"

static void fill_listbox(RepoDlg *repo_dlg);
static void kill_listbox(RepoDlg *repo_dlg);

static int repo_listbox_type();
static bool repo_listbox_msg_proc(JWidget widget, Message* msg);

static void use_command(Button* widget, RepoDlg* repo_dlg);
static void add_command(Button* widget, RepoDlg* repo_dlg);
static void delete_command(Button* widget, RepoDlg* repo_dlg);

void ji_show_repo_dlg(RepoDlg *repo_dlg)
{
  Frame* window = new Frame(false, repo_dlg->title);
  Box* box1 = new Box(JI_HORIZONTAL);
  Box* box2 = new Box(JI_VERTICAL);
  View* view = new View();
  repo_dlg->listbox = jlistbox_new();
  repo_dlg->button_use = new Button(repo_dlg->use_text);
  repo_dlg->button_add = new Button("&Add");
  repo_dlg->button_delete = new Button("&Delete");
  Button* button_close = new Button("&Close");

  jwidget_add_hook(repo_dlg->listbox, repo_listbox_type(),
		   repo_listbox_msg_proc, repo_dlg);

  repo_dlg->button_use->Click.connect(Bind<void>(&use_command, repo_dlg->button_use, repo_dlg));
  repo_dlg->button_add->Click.connect(Bind<void>(&add_command, repo_dlg->button_add, repo_dlg));
  repo_dlg->button_delete->Click.connect(Bind<void>(&delete_command, repo_dlg->button_delete, repo_dlg));
  button_close->Click.connect(Bind<void>(&Frame::closeWindow, window, button_close));

  jwidget_magnetic(repo_dlg->button_use, true);

  jwidget_expansive(view, true);
  view->attachToView(repo_dlg->listbox);
  jwidget_set_min_size(view, JI_SCREEN_W*25/100, JI_SCREEN_H*25/100);

  /* fill the list */
  fill_listbox(repo_dlg);
  jlistbox_select_index(repo_dlg->listbox, 0);

  /* no items? */
  if (jlistbox_get_selected_index(repo_dlg->listbox) != 0) {
    repo_dlg->button_use->setEnabled(false);
    repo_dlg->button_delete->setEnabled(false);
  }

  /* hierarchy */
  box2->addChild(repo_dlg->button_use);
  box2->addChild(repo_dlg->button_add);
  box2->addChild(repo_dlg->button_delete);
  box2->addChild(button_close);
  box1->addChild(box2);
  box1->addChild(view);
  window->addChild(box1);

  /* default position */
  window->remap_window();
  window->center_window();

  /* load window configuration */
  load_window_pos(window, repo_dlg->config_section);

  /* open window */
  window->open_window_fg();

  /* kill the list */
  kill_listbox(repo_dlg);

  /* save window configuration */
  save_window_pos(window, repo_dlg->config_section);

  jwidget_free(window);
}

static void fill_listbox(RepoDlg *repo_dlg)
{
  /* no item selected (ID=0) */
  repo_dlg->listitem = 0;

  /* load the entries */
  if (repo_dlg->load_listbox)
    (*repo_dlg->load_listbox)(repo_dlg);

  View::getView(repo_dlg->listbox)->updateView();
}

static void kill_listbox(RepoDlg *repo_dlg)
{
  JWidget listbox = repo_dlg->listbox;
  JWidget listitem;
  JLink link, next;

  /* save the entries */
  if (repo_dlg->save_listbox)
    (*repo_dlg->save_listbox) (repo_dlg);

  /* remove all items */
  JI_LIST_FOR_EACH_SAFE(listbox->children, link, next) {
    listitem = reinterpret_cast<JWidget>(link->data);

    repo_dlg->listbox->removeChild(listitem);

    if (repo_dlg->free_listitem) {
      repo_dlg->listitem = listitem;
      (*repo_dlg->free_listitem) (repo_dlg);
    }

    jwidget_free(listitem);
  }
}

static int repo_listbox_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static bool repo_listbox_msg_proc(JWidget widget, Message* msg)
{
  RepoDlg* repo_dlg = reinterpret_cast<RepoDlg*>(jwidget_get_data(widget, repo_listbox_type()));

  switch (msg->type) {

    case JM_DESTROY:
      /* do nothing (don't free repo_dlg) */
      break;

    case JM_SIGNAL:
      switch (msg->signal.num) {

	case JI_SIGNAL_LISTBOX_CHANGE:
	  repo_dlg->listitem = jlistbox_get_selected_child(widget);

	  repo_dlg->button_use->setEnabled(true);
	  repo_dlg->button_delete->setEnabled(true);
	  break;

	case JI_SIGNAL_LISTBOX_SELECT:
	  if (repo_dlg->use_listitem)
	    if (!(*repo_dlg->use_listitem)(repo_dlg))
	      widget->closeWindow();
	  break;
      }
      break;
  }

  return false;
}

static void use_command(Button* widget, RepoDlg* repo_dlg)
{
  if (repo_dlg->use_listitem) {
    if (!(*repo_dlg->use_listitem)(repo_dlg))
      widget->closeWindow();
  }
}

static void add_command(Button* widget, RepoDlg* repo_dlg)
{
  int ret, added;

  if (repo_dlg->add_listitem) {
    added = false;
    ret = (*repo_dlg->add_listitem)(repo_dlg, &added);

    if (added) {
      /* update the list-box */
      View::getView(repo_dlg->listbox)->updateView();

      /* select the last item */
      jlistbox_select_index(repo_dlg->listbox,
			    jlist_length(repo_dlg->listbox->children)-1);
    }

    if (!ret)
      widget->closeWindow();
  }
}

static void delete_command(Button* widget, RepoDlg* repo_dlg)
{
  int ret, kill, index, last;

  if (repo_dlg->delete_listitem) {
    kill = false;
    ret = (*repo_dlg->delete_listitem) (repo_dlg, &kill);

    if (kill) {
      index = jlistbox_get_selected_index(repo_dlg->listbox);

      /* delete "repo_dlg->listitem" */
      repo_dlg->listbox->removeChild(repo_dlg->listitem);

      if (repo_dlg->free_listitem)
	(*repo_dlg->free_listitem) (repo_dlg);

      jwidget_free(repo_dlg->listitem);

      /* disable buttons */
      if (!repo_dlg->listbox->children) {
	repo_dlg->button_use->setEnabled(false);
	repo_dlg->button_delete->setEnabled(false);
      }
      /* select other item */
      else {
	last = jlist_length(repo_dlg->listbox->children)-1;

	jlistbox_select_index(repo_dlg->listbox,
			      index > last ? last: index);
      }

      /* update the list-box */
      View::getView(repo_dlg->listbox)->updateView();
    }

    if (!ret)
      widget->closeWindow();
  }
}
