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

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "jinete.h"

#include "modules/gui.h"

#endif

#define DEPTH_TO_INDEX(bpp)			\
  ((bpp == 8)? 0:				\
   (bpp == 15)? 1:				\
   (bpp == 16)? 2:				\
   (bpp == 24)? 3:				\
   (bpp == 32)? 4: -1)

#define INDEX_TO_DEPTH(index)			\
  ((index == 0)? 8:				\
   (index == 1)? 15:				\
   (index == 2)? 16:				\
   (index == 3)? 24:				\
   (index == 4)? 32: -1)

typedef struct GfxCard
{
  int id;
  char *name;
} GfxCard;

static JList gfx_cards;

static JWidget list_card;
static JWidget list_depth;
static JWidget entry_width;
static JWidget entry_height;

static int request_gfx_cards(void);
static GfxCard *gfx_card_new(int id, const char *name);

static int listbox_size_change_signal (JWidget listbox, int user_data);

int gfxmode_select(int *card, int *w, int *h, int *depth)
{
  JWidget window, box1, box2, box3, box4, box5, box6;
  JWidget label_card, label_size;
  JWidget view_card, view_size, view_depth;
  JWidget list_size, button_select, button_cancel;
  GfxCard *gfx_card;
  JLink link;
  int n;
  int ret = FALSE;

  if (!gfx_cards) {
    if (request_gfx_cards() < 0) {
      jalert(_("Error<<Not enough memory||&Close"));
      return ret;
    }
  }

  window = jwindow_new(_("Select Graphics Mode"));
  box1 = jbox_new(JI_HORIZONTAL);
  box2 = jbox_new(JI_VERTICAL);
  box3 = jbox_new(JI_VERTICAL);
  box4 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
  box5 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
  box6 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
  label_card = jlabel_new(_("Card:"));
  label_size = jlabel_new(_("Size:"));
  view_card = jview_new();
  view_size = jview_new();
  view_depth = jview_new();
  list_card = jlistbox_new();
  list_size = jlistbox_new();
  list_depth = jlistbox_new();
  entry_width = jentry_new(4, "%d", *w);
  entry_height = jentry_new(4, "%d", *h);
  button_select = jbutton_new(_("&OK"));
  button_cancel = jbutton_new(_("&Cancel"));

  jwidget_set_align(label_card, JI_LEFT | JI_MIDDLE);
  jwidget_set_align(label_size, JI_RIGHT | JI_MIDDLE);

  jview_attach(view_card, list_card);
  jview_attach(view_size, list_size);
  jview_attach(view_depth, list_depth);

  JI_LIST_FOR_EACH(gfx_cards, link) {
    gfx_card = link->data;
    jwidget_add_child(list_card, jlistitem_new(gfx_card->name));
  }
  n = 0;
  JI_LIST_FOR_EACH(gfx_cards, link) {
    gfx_card = link->data;
    if (*card == gfx_card->id)
      jlistbox_select_index(list_card, n);
    n++;
  }

  jwidget_add_childs(list_size,
		       jlistitem_new ("320x200"),
		       jlistitem_new ("320x240"),
		       jlistitem_new ("640x400"),
		       jlistitem_new ("640x480"),
		       jlistitem_new ("800x600"),
		       jlistitem_new ("1024x768"), NULL);
  jwidget_add_childs(list_depth,
		       jlistitem_new (_("8 bpp (256 colors)")),
		       jlistitem_new (_("15 bpp (32K colors)")),
		       jlistitem_new (_("16 bpp (64K colors)")),
		       jlistitem_new (_("24 bpp (16M colors)")),
		       jlistitem_new (_("32 bpp (16M colors)")), NULL);
  jlistbox_select_index(list_depth, DEPTH_TO_INDEX(*depth));

  HOOK(list_size, JI_SIGNAL_LISTBOX_CHANGE, listbox_size_change_signal, 0);

  jwidget_set_static_size(view_card, JI_SCREEN_W/2, JI_SCREEN_H*7/10);
  jview_maxsize(view_size);
  jview_maxsize(view_depth);

  jwidget_expansive(box1, TRUE);
  jwidget_expansive(box2, TRUE);
  jwidget_expansive(box3, TRUE);
  jwidget_expansive(view_card, TRUE);
  jwidget_expansive(view_size, TRUE);
  jwidget_expansive(view_depth, TRUE);

  jwidget_add_child(window, box1);
  jwidget_add_childs(box1, box2, box3, NULL);
  jwidget_add_childs(box2, box6, view_card, NULL);
  jwidget_add_childs(box3, box4, view_size, view_depth, box5, NULL);
  jwidget_add_childs(box4, entry_width, entry_height, NULL);
  jwidget_add_childs(box5, button_select, button_cancel, NULL);
  jwidget_add_childs(box6, label_card, label_size, NULL);

  jwidget_magnetic(button_select, TRUE);

  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == button_select) {
    int card_index = jlistbox_get_selected_index(list_card);
    GfxCard *gfx_card;
    int n;

    /* card ID */
    n = 0;
    JI_LIST_FOR_EACH(gfx_cards, link) {
      gfx_card = link->data;
      if (n == card_index) {
	*card = gfx_card->id;
	break;
      }
      n++;
    }

    /* width and height */
    *w = ustrtol(jwidget_get_text(entry_width), NULL, 10);
    *h = ustrtol(jwidget_get_text(entry_height), NULL, 10);

    /* color depth */
    *depth = INDEX_TO_DEPTH(jlistbox_get_selected_index(list_depth));

    ret = TRUE;
  }

  jwidget_free(window);
  return ret;
}

void destroy_gfx_cards(void)
{
  if (gfx_cards) {
    GfxCard *gfx_card;
    JLink link;

    JI_LIST_FOR_EACH(gfx_cards, link) {
      gfx_card = link->data;
      jfree(gfx_card->name);
      jfree(gfx_card);
    }

    jlist_free(gfx_cards);
    gfx_cards = NULL;
  }
}

static int request_gfx_cards(void)
{
  _DRIVER_INFO *driver_info;
  GFX_DRIVER *gfx_driver;
  char buf[256];
  int n;

  gfx_cards = jlist_new();

  if (system_driver->gfx_drivers)
    driver_info = system_driver->gfx_drivers();
  else
    driver_info = _gfx_driver_list;

  gfx_card_new(GFX_AUTODETECT, _("Autodetect"));
  gfx_card_new(GFX_AUTODETECT_FULLSCREEN, _("Auto fullscreen"));
  gfx_card_new(GFX_AUTODETECT_WINDOWED, _("Auto windowed"));

  for (n=0; driver_info[n].driver; n++) {
    gfx_driver = driver_info[n].driver;
    do_uconvert(gfx_driver->ascii_name, U_ASCII,
		buf, U_CURRENT, sizeof(buf));

    if (!gfx_card_new(driver_info[n].id, buf))
      return -1;
  }

  return 0;
}

static GfxCard *gfx_card_new(int id, const char *name)
{
  GfxCard *gfx_card;

  gfx_card = jnew(GfxCard, 1);
  if (!gfx_card)
    return NULL;

  gfx_card->id = id;
  gfx_card->name = jstrdup(name);
  if (!gfx_card->name) {
    jfree(gfx_card);
    return NULL;
  }

  jlist_append(gfx_cards, gfx_card);

  return gfx_card;
}

static int listbox_size_change_signal (JWidget widget, int user_data)
{
  JWidget selected = jlistbox_get_selected_child (widget);

  if (selected) {
    char *buf = jstrdup (jwidget_get_text (selected));
    char *s = ustrchr (buf, 'x');

    *s = 0;
    jwidget_set_text (entry_width, buf);
    jwidget_set_text (entry_height, s+1);

    jentry_select_text (entry_width, 0, -1);
    jentry_select_text (entry_height, 0, -1);

    jfree (buf);
  }

  return 0;
}

