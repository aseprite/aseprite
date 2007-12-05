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
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "jinete/jinete.h"

#include "console/console.h"
#include "core/cfg.h"
#include "core/dirs.h"
#include "intl/intl.h"
#include "modules/gui.h"
#include "modules/palette.h"

#endif

static JWidget tips_new (void);
static int tips_type (void);
static bool tips_msg_proc (JWidget widget, JMessage msg);
static void tips_request_size (JWidget widget, int *w, int *h);

static JWidget tips_image_new (BITMAP *bmp);
static int tips_image_type (void);
static bool tips_image_msg_proc (JWidget widget, JMessage msg);

static FILE *tips_open_file (void);
static int tips_count_pages (void);
static void tips_load_page (JWidget widget);
static JWidget tips_load_box (FILE *f, char *buf, int sizeof_buf, int *take);
static BITMAP *tips_load_image (const char *filename, PALETTE pal);

static void prev_command (JWidget widget, void *data);
static void next_command (JWidget widget, void *data);
static int check_signal (JWidget widget, int user_data);

void dialogs_tips(bool forced)
{
  JWidget window, vbox, hbox, box;
  JWidget button_close, button_prev, button_next;
  JWidget view, tips;
  JWidget check;
  PALETTE old_pal;

  /* don't show it? */
  if (!forced && !get_config_bool("Tips", "Show", TRUE))
    return;

  /* next page */
  {
    int pages = tips_count_pages ();
    int page = get_config_int ("Tips", "Page", -1);

    /* error? */
    if (!pages)
      return;

    /* first time */
    if (page == -1)
      set_config_int ("Tips", "Page", 0);
    /* other times (go to next tip) */
    else {
      page = MID (0, page, pages-1);
      set_config_int ("Tips", "Page", (page+1) % pages);
    }
  }

  window = jwindow_new("Allegro Sprite Editor");
  vbox = jbox_new(JI_VERTICAL);
  hbox = jbox_new(JI_HORIZONTAL);
  box = jbox_new(0);
  button_close = jbutton_new(_("&Close"));
  button_prev = jbutton_new(_("&Previous"));
  button_next = jbutton_new(_("&Next"));
  view = jview_new();
  tips = tips_new();
  check = jcheck_new(_("Show me it in the start up"));

  jwidget_set_static_size(button_close, 50, 0);
  jwidget_set_static_size(button_prev, 50, 0);
  jwidget_set_static_size(button_next, 50, 0);

  jbutton_add_command_data(button_prev, prev_command, tips);
  jbutton_add_command_data(button_next, next_command, tips);

  HOOK(check, JI_SIGNAL_CHECK_CHANGE, check_signal, 0);

  if (get_config_bool("Tips", "Show", TRUE))
    jwidget_select(check);

  jview_attach(view, tips);

  jwidget_expansive(view, TRUE);
  jwidget_expansive(box, TRUE);

  jwidget_add_child(vbox, view);
  jwidget_add_child(hbox, button_close);
  jwidget_add_child(box, check);
  jwidget_add_child(hbox, box);
  jwidget_add_child(hbox, button_prev);
  jwidget_add_child(hbox, button_next);
  jwidget_add_child(vbox, hbox);
  jwidget_add_child(window, vbox);

/*   if (JI_SCREEN_W > 320) */
    jwidget_set_static_size(window,
			    MIN(400, JI_SCREEN_W-32),
			    MIN(300, JI_SCREEN_H-16));
/*   else */
/*     jwidget_set_static_size(window, 282, 200); */

  /* open the window */
  jwindow_open(window);
  jwidget_set_static_size(window, 0, 0);

  /* load first page */
  memcpy(old_pal, current_palette, sizeof (PALETTE));
  tips_load_page(tips);

  /* run the window */
  jwindow_open_fg(window);
  jwidget_free(window);

  /* restore the palette */
  set_current_palette(old_pal, TRUE);
  jmanager_refresh_screen();
}

/***********************************************************************
				Tips
 ***********************************************************************/

static JWidget tips_new(void)
{
  JWidget widget = jwidget_new(tips_type());

  jwidget_add_hook(widget, tips_type(), tips_msg_proc, NULL);
  jwidget_focusrest(widget, TRUE);
  jwidget_noborders(widget);

  return widget;
}

static int tips_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type ();
  return type;
}

static bool tips_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      tips_request_size (widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_WHEEL:
      {
	JWidget view = jwidget_get_view(widget);
	JRect vp = jview_get_viewport_position(view);
	int dz = jmouse_z(1) - jmouse_z(0);
	int scroll_x, scroll_y;

	jview_get_scroll(view, &scroll_x, &scroll_y);
	jview_set_scroll(view, scroll_x, scroll_y + dz * jrect_h(vp)/3);

	jrect_free(vp);
      }
      break;
      
  }

  return FALSE;
}

static void tips_request_size(JWidget widget, int *w, int *h)
{
  int max_w, max_h;
  int req_w, req_h;
  JWidget child;
  JLink link;

  max_w = max_h = 0;

  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;

    jwidget_request_size(child, &req_w, &req_h);
    max_w = MAX(max_w, req_w);
    max_h = MAX(max_h, req_h);
  }

  *w = max_w;
  *h = max_h;
}

static JWidget tips_image_new(BITMAP *bmp)
{
  JWidget widget = jimage_new(bmp, JI_CENTER | JI_MIDDLE);
  jwidget_add_hook(widget, tips_image_type(),
		   tips_image_msg_proc, bmp);
  return widget;
}

static int tips_image_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static bool tips_image_msg_proc(JWidget widget, JMessage msg)
{
  if (msg->type == JM_DESTROY)
    destroy_bitmap(jwidget_get_data(widget, tips_image_type()));

  return FALSE;
}

static FILE *tips_open_file(void)
{
  char filename[1024];
  DIRS *dirs, *dir;

  sprintf (filename, "tips/tips.%s", intl_get_lang ());
  dirs = filename_in_datadir (filename);

  for (dir=dirs; dir; dir=dir->next) {
    if ((dir->path) && exists (dir->path)) {
      strcpy (filename, dir->path);
      break;
    }
  }

  if (!dir)
    strcpy (filename, dirs->path);

  dirs_free (dirs);

  return fopen (filename, "rt");
}

static int tips_count_pages (void)
{
  char buf[1024];
  int page = 0;
  FILE *f;

  f = tips_open_file ();
  if (!f) {
    console_printf ("Error loading tips files\n");
    return 0;
  }

  while (fgets (buf, sizeof (buf), f)) {
    if (*buf == 12)
      page++;
  }

  fclose (f);

  if (!page)
    console_printf ("No pages tips file\n");

  return page;
}

static void tips_load_page(JWidget widget)
{
  int use_page = get_config_int("Tips", "Page", 0);
  char buf[1024];
  int page = 0;
  int take = TRUE;
  FILE *f;

  /* destroy old page */
  if (!jlist_empty(widget->children)) {
    JWidget child = jlist_first(widget->children)->data;
    jwidget_remove_child(widget, child);
    jwidget_free(child);
  }

  /* set default palette */
  set_current_palette(NULL, FALSE);

  f = tips_open_file();
  if (!f) {
    jwidget_add_child(widget, jlabel_new(_("Error loading tips file.")));
    return;
  }

  while (fgets(buf, sizeof(buf), f)) {
    if (*buf == 12) {
      /* read this page */
      if (use_page == page) {
	JWidget vbox = tips_load_box(f, buf, sizeof (buf), &take);
	if (vbox)
	  jwidget_add_child(widget, vbox);
	break;
      }
      page++;
    }
  }

  fclose(f);

  jview_update(jwidget_get_view(widget));
  jview_set_scroll(jwidget_get_view(widget), 0, 0);

  jmanager_refresh_screen();
}

static JWidget tips_load_box(FILE *f, char *buf, int sizeof_buf, int *take)
{
  JWidget vbox = jbox_new (JI_VERTICAL);

  jwidget_set_border (vbox, 2, 2, 2, 2);

  for (;;) {
    if (*take) {
      if (!fgets (buf, sizeof_buf, f))
	break;
    }
    else
      *take = TRUE;

    if (*buf == 12)
      break;
    /* comment */
    else if (*buf == '#')
      continue;

    /* remove trailing space chars */
    while (*buf && isspace (ugetat (buf, -1)))
      usetat (buf, -1, 0);

    /************************************************************/
    /* empty? */
    if (!*buf) {
      /* add a box with an static size to separate paragraphs */
      JWidget box = jbox_new (0);

      jwidget_set_static_size (box, 0, text_height (box->text_font));

      jwidget_add_child (vbox, box);
    }
    /************************************************************/
    /* special object (line start with \) */
    else if (*buf == '\\') {
      /* \split */
      if (ustrncmp (buf+1, "split", 5) == 0) {
	JWidget box, hbox = jbox_new (JI_HORIZONTAL | JI_HOMOGENEOUS);

	do {
	  box = tips_load_box (f, buf, sizeof_buf, take);
	  if (box)
	    jwidget_add_child (hbox, box);
	} while (ustrncmp (buf, "\\next", 5) == 0);

	jwidget_add_child (vbox, hbox);
	*take = TRUE;
      }
      /* \next and \done */
      else if ((ustrncmp (buf+1, "next", 4) == 0) ||
	       (ustrncmp (buf+1, "done", 4) == 0)) {
	break;
      }
      /* \image filename */
      else if (ustrncmp (buf+1, "image", 5) == 0) {
	char filename[1024];
	PALETTE pal;
	BITMAP *bmp;

	sprintf (filename, "tips/%s", strchr (buf, ' ')+1);
	bmp = tips_load_image (filename, pal);
	if (bmp) {
	  JWidget image = tips_image_new (bmp);
	  jwidget_add_child (vbox, image);
	}
	else {
	  sprintf (buf, _("Error loading image %s"), filename);
	  jwidget_add_child (vbox, jlabel_new (buf));
	}
      }
      /* \palette filename */
      else if (ustrncmp (buf+1, "palette", 7) == 0) {
	char filename[1024];
	PALETTE pal;
	BITMAP *bmp;

	sprintf (filename, "tips/%s", strchr (buf, ' ')+1);
	bmp = tips_load_image (filename, pal);
	if (bmp) {
	  set_current_palette (pal, FALSE);
	  destroy_bitmap (bmp);
	}
	else {
	  sprintf (buf, _("Error loading palette %s"), filename);
	  jwidget_add_child (vbox, jlabel_new (buf));
	}
      }
    }
    /************************************************************/
    /* add text */
    else {
      char *text = jstrdup (buf);

      /* read more text (to generate a paragraph) */
      if (*text != '*') {
	while (fgets(buf, sizeof_buf, f)) {
	  if (*buf == 12 || *buf == '\\') {
	    *take = FALSE;
	    break;
	  }
	  /* comment */
	  else if (*buf == '#')
	    continue;

	  /* remove trailing space chars */
	  while (*buf && isspace(ugetat (buf, -1)))
	    usetat(buf, -1, 0);

	  /* empty? */
	  if (!*buf) {
	    *take = FALSE;
	    break;
	  }

	  /* add chars */
	  text = jrealloc(text, strlen (text) + 1 + strlen (buf) + 1);
	  strcat(text, " ");
	  strcpy(text+strlen (text), buf);
	}

	/* add the textbox */
	jwidget_add_child(vbox, jtextbox_new(text, JI_WORDWRAP | JI_CENTER));
      }
      else {
	/* add a label */
	jwidget_add_child(vbox, jtextbox_new(text+2, JI_WORDWRAP | JI_LEFT));
      }

      jfree(text);
    }
  }

  /* white background */
  jwidget_set_bg_color (vbox, makecol (255, 255, 255));

  return vbox;
}

static BITMAP *tips_load_image(const char *filename, PALETTE pal)
{
  BITMAP *bmp = NULL;
  DIRS *dir, *dirs;

  dirs = filename_in_datadir(filename);

  for (dir=dirs; dir; dir=dir->next) {
    bmp = load_bitmap(dir->path, pal);
    if (bmp)
      break;
  }

  dirs_free(dirs);

  return bmp;
}

static void prev_command (JWidget widget, void *data)
{
  int pages = tips_count_pages ();
  int page = get_config_int ("Tips", "Page", 0);

  set_config_int ("Tips", "Page", page > 0 ? page-1: pages-1);
  tips_load_page ((JWidget)data);
}

static void next_command (JWidget widget, void *data)
{
  int pages = tips_count_pages ();
  int page = get_config_int ("Tips", "Page", 0);

  set_config_int ("Tips", "Page", page < pages-1 ? page+1: 0);
  tips_load_page ((JWidget)data);
}

static int check_signal (JWidget widget, int user_data)
{
  set_config_bool ("Tips", "Show", jwidget_is_selected (widget));
  return TRUE;
}
