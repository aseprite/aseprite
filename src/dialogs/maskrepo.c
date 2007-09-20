/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

#include <stdio.h>

#include "jinete/alert.h"
#include "jinete/box.h"
#include "jinete/button.h"
#include "jinete/entry.h"
#include "jinete/label.h"
#include "jinete/list.h"
#include "jinete/listbox.h"
#include "jinete/widget.h"
#include "jinete/window.h"

#include "core/core.h"
#include "dialogs/repo.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/mask.h"
#include "raster/sprite.h"

#endif

static JWidget new_listitem (Mask *mask);

static void load_listbox (RepoDlg *repo_dlg);
static bool use_listitem (RepoDlg *repo_dlg);
static bool add_listitem (RepoDlg *repo_dlg, int *added);
static bool delete_listitem (RepoDlg *repo_dlg, int *kill);

void dialogs_mask_repository(void)
{
  RepoDlg repo_dlg = {
    "MaskRepository",
    _("Mask Repository"),
    _("&Replace"),
    load_listbox,
    NULL,
    NULL,
    use_listitem,
    add_listitem,
    delete_listitem,
    { NULL, NULL, NULL, NULL }
  };
  Sprite *sprite = current_sprite;

  if (!is_interactive () || !sprite)
    return;

  ji_show_repo_dlg (&repo_dlg);
}

static JWidget new_listitem (Mask *mask)
{
  char buf[256];
  JWidget listitem;

  sprintf (buf, "%s (%d %d)", mask->name, mask->w, mask->h);

  listitem = jlistitem_new (buf);
  listitem->user_data[0] = mask;

  return listitem;
}

static void load_listbox (RepoDlg *repo_dlg)
{
  Sprite *sprite = current_sprite;
  JLink link;

  /* disable "Add" button if there isn't a current mask */
  if (!sprite->mask->bitmap)
    jwidget_disable (repo_dlg->button_add);

  /* add new items to the list-box */
  JI_LIST_FOR_EACH(sprite->repository.masks, link)
    jwidget_add_child (repo_dlg->listbox, new_listitem (link->data));
}

static bool use_listitem(RepoDlg *repo_dlg)
{
  JWidget listitem = repo_dlg->listitem;
  Mask *mask = listitem->user_data[0];
  Sprite *sprite = current_sprite;

  /* XXXX undo stuff */
  sprite_set_mask(sprite, mask);
  sprite_generate_mask_boundaries(sprite);
  GUI_Refresh(sprite);

  return TRUE;
}

static bool add_listitem (RepoDlg *repo_dlg, int *added)
{
  JWidget window, box1, box2, box3;
  JWidget label_name, entry_name, button_ok, button_cancel;

  window = jwindow_new (_("New Mask"));
  box1 = jbox_new (JI_VERTICAL);
  box2 = jbox_new (JI_HORIZONTAL);
  box3 = jbox_new (JI_HORIZONTAL | JI_HOMOGENEOUS);
  label_name = jlabel_new (_("Name:"));
  entry_name = jentry_new (256, _("Current Mask"));
  button_ok = jbutton_new (_("&OK"));
  button_cancel = jbutton_new (_("&Cancel"));

  jwidget_magnetic (button_ok, TRUE);

  jwidget_expansive (entry_name, TRUE);
  jwidget_expansive (box3, TRUE);

  jwidget_add_child (box2, label_name);
  jwidget_add_child (box2, entry_name);
  jwidget_add_child (box1, box2);
  jwidget_add_child (box3, button_ok);
  jwidget_add_child (box3, button_cancel);
  jwidget_add_child (box1, box3);
  jwidget_add_child (window, box1);

  jwindow_open_fg (window);

  if (jwindow_get_killer (window) == button_ok) {
    const char *name = jwidget_get_text (entry_name);
    Sprite *sprite = current_sprite;
    Mask *mask;

    /* if the mask already exists */
    if (sprite_request_mask (sprite, name)) {
      jalert (_("Error"
		"<<You can't have two masks with"
		"<<the same name in the repository||&OK"));
    }
    else {
      mask = mask_new_copy (sprite->mask);

      /* set the mask's name */
      mask_set_name (mask, name);

      /* append the mask */
      sprite_add_mask (sprite, mask);

      /* we added the new mask */
      jwidget_add_child (repo_dlg->listbox, new_listitem (mask));
      *added = TRUE;
    }
  }

  jwidget_free (window);
  return TRUE;
}

static bool delete_listitem (RepoDlg *repo_dlg, int *kill)
{
  JWidget listitem = repo_dlg->listitem;
  Sprite *sprite = current_sprite;
  Mask *mask = listitem->user_data[0];

  sprite_remove_mask (sprite, mask);
  mask_free (mask);

  *kill = TRUE;
  return TRUE;
}
