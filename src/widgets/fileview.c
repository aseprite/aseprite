/* ASE - Allegro Sprite Editor
 * Copyright (C) 2008  David A. Capello
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

#include <assert.h>
#include <allegro.h>

#include "jinete/jinete.h"

#include "modules/gfx.h"
#include "widgets/fileview.h"

#endif

typedef struct FileView
{
  FileItem *current_folder;
  JList list;
  bool req_valid;
  int req_w, req_h;
  FileItem *selected;
  const char *exts;
} FileView;

static FileView *fileview_data(JWidget widget);
static bool fileview_msg_proc(JWidget widget, JMessage msg);
static void fileview_get_fileitem_size(JWidget widget, FileItem *fi, int *w, int *h);
static void fileview_make_selected_fileitem_visible(JWidget widget);
static void fileview_regenerate_list(JWidget widget);
static int fileview_get_selected_index(JWidget widget);
static void fileview_select_index(JWidget widget, int index);

JWidget fileview_new(FileItem *start_folder, const char *exts)
{
  JWidget widget = jwidget_new(fileview_type());
  FileView *fileview = jnew(FileView, 1);

  if (!start_folder)
    start_folder = get_root_fileitem();
  else {
    while (!fileitem_is_folder(start_folder) &&
	   fileitem_get_parent(start_folder) != NULL) {
      start_folder = fileitem_get_parent(start_folder);
    }
  }

  jwidget_add_hook(widget, fileview_type(),
		   fileview_msg_proc, fileview);
  jwidget_focusrest(widget, TRUE);

  fileview->current_folder = start_folder;
  fileview->list = NULL;
  fileview->req_valid = FALSE;
  fileview->selected = NULL;
  fileview->exts = exts;

  fileview_regenerate_list(widget);

  return widget;
}

int fileview_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

FileItem *fileview_get_current_folder(JWidget widget)
{
  return fileview_data(widget)->current_folder;
}

FileItem *fileview_get_selected(JWidget widget)
{
  return fileview_data(widget)->selected;
}

void fileview_set_current_folder(JWidget widget, FileItem *folder)
{
  FileView *fileview = fileview_data(widget);

  assert(folder != NULL);
  assert(fileitem_is_browsable(folder));

  fileview->current_folder = folder;
  fileview->req_valid = FALSE;
  fileview->selected = NULL;

  fileview_regenerate_list(widget);

  /* select first folder */
  if (!jlist_empty(fileview->list) &&
      fileitem_is_browsable(jlist_first_data(fileview->list))) {
    fileview_select_index(widget, 0);
  }

  jwidget_emit_signal(widget, SIGNAL_FILEVIEW_CURRENT_FOLDER_CHANGED);

  jwidget_dirty(widget);
  jview_update(jwidget_get_view(widget));
}

void fileview_goup(JWidget widget)
{
  FileView *fileview = fileview_data(widget);
  FileItem *folder = fileview->current_folder;
  FileItem *parent = fileitem_get_parent(folder);
  if (parent) {
    fileview_set_current_folder(widget, parent);
    fileview->selected = folder;
  }
}

static FileView *fileview_data(JWidget widget)
{
  return jwidget_get_data(widget, fileview_type());
}

static bool fileview_msg_proc(JWidget widget, JMessage msg)
{
  FileView *fileview = fileview_data(widget);

  switch (msg->type) {

    case JM_DESTROY:
      jfree(fileview);
      break;

    case JM_REQSIZE:
      if (!fileview->req_valid) {
	FileItem *fileitem;
	int w, h, iw, ih;
	JLink link;

	w = 0;
	h = 0;

	/* rows */
	JI_LIST_FOR_EACH(fileview->list, link) {
	  fileitem = link->data;
	  fileview_get_fileitem_size(widget, fileitem, &iw, &ih);
	  w = MAX(w, iw);
	  h += ih;
	}

	fileview->req_valid = TRUE;
	fileview->req_w = w;
	fileview->req_h = h;
      }

      msg->reqsize.w = fileview->req_w;
      msg->reqsize.h = fileview->req_h;
      return TRUE;

    case JM_DRAW: {
      FileItem *fi;
      JLink link;
      int iw, ih;
      int th = jwidget_get_text_height(widget);
      int x, y = widget->rc->y1;
      int row = 0;
      int bgcolor;
      int fgcolor;

      jdraw_rectfill(widget->rc, makecol(255, 255, 255));

      /* rows */
      JI_LIST_FOR_EACH(fileview->list, link) {
	fi = link->data;
	fileview_get_fileitem_size(widget, fi, &iw, &ih);
	
	if (fi == fileview->selected) {
	  bgcolor = ji_color_selected();
	  fgcolor = ji_color_background();
	}
	else {
	  bgcolor = row ? makecol(240, 240, 240):
			  makecol(255, 255, 255);

	  fgcolor =
	    fileitem_is_folder(fi) &&
	    !fileitem_is_browsable(fi) ? makecol(255, 200, 200):
					 ji_color_foreground();
	}

	rectfill(ji_screen,
		 widget->rc->x1, y,
		 widget->rc->x2-1, y+2+th+2-1,
		 bgcolor);

	x = widget->rc->x1+2;

	if (fileitem_is_folder(fi)) {
	  jdraw_text(widget->text_font,
		     "[+]", x, y+2,
		     fgcolor, 0, FALSE);

	  x += ji_font_text_len(widget->text_font, "[+]")+2;
	}
	
	jdraw_text(widget->text_font,
		   fileitem_get_displayname(fi), x, y+2,
		   fgcolor, 0, FALSE);

	y += ih;
	row ^= 1;
      }
      break;
    }

    case JM_BUTTONPRESSED:
      jwidget_capture_mouse(widget);

    case JM_MOTION:
      if (jwidget_has_capture(widget)) {
	FileItem *fi;
	JLink link;
	int iw, ih;
	int th = jwidget_get_text_height(widget);
	int y = widget->rc->y1;
	FileItem *old_selected = fileview->selected;
	fileview->selected = NULL;

	/* rows */
	JI_LIST_FOR_EACH(fileview->list, link) {
	  fi = link->data;
	  fileview_get_fileitem_size(widget, fi, &iw, &ih);

	  if (((msg->mouse.y >= y) && (msg->mouse.y < y+2+th+2)) ||
	      (link == jlist_first(fileview->list) && msg->mouse.y < y) ||
	      (link == jlist_last(fileview->list) && msg->mouse.y >= y+2+th+2)) {
	    fileview->selected = fi;
	    fileview_make_selected_fileitem_visible(widget);
	    break;
	  }

	  y += ih;
	}

	if (old_selected != fileview->selected) {
	  jwidget_dirty(widget);
	  jwidget_emit_signal(widget, SIGNAL_FILEVIEW_FILE_SELECTED);
	}
      }
      break;

    case JM_BUTTONRELEASED:
      if (jwidget_has_capture(widget)) {
	jwidget_release_mouse(widget);
      }
      break;

    case JM_CHAR:
      if (jwidget_has_focus(widget)) {
	int select = fileview_get_selected_index(widget);
	JWidget view = jwidget_get_view(widget);
	int bottom = MAX(0, jlist_length(fileview->list)-1);

	switch (msg->key.scancode) {
	  case KEY_UP:
	    if (select >= 0)
	      select--;
	    else
	      select = 0;
	    break;
	  case KEY_DOWN:
	    if (select >= 0)
	      select++;
	    else
	      select = 0;
	    break;
	  case KEY_HOME:
	    select = 0;
	    break;
	  case KEY_END:
	    select = bottom;
	    break;
	  case KEY_PGUP:
	  case KEY_PGDN: {
	    int sgn = (msg->key.scancode == KEY_PGUP) ? -1: 1;
	    JRect vp = jview_get_viewport_position(view);
	    if (select < 0)
	      select = 0;
	    select += sgn * jrect_h(vp) / (2+jwidget_get_text_height(widget)+2);
	    jrect_free(vp);
	    break;
	  }
	  case KEY_LEFT:
	  case KEY_RIGHT:
	    if (select >= 0) {
	      JRect vp = jview_get_viewport_position(view);
	      int sgn = (msg->key.scancode == KEY_LEFT) ? -1: 1;
	      int scroll_x, scroll_y;

	      jview_get_scroll(view, &scroll_x, &scroll_y);
	      jview_set_scroll(view, scroll_x + jrect_w(vp)/2*sgn, scroll_y);
	      jrect_free(vp);
	    }
	    break;
	  case KEY_ENTER:
	    if (fileview->selected) {
	      if (fileitem_is_browsable(fileview->selected)) {
		fileview_set_current_folder(widget, fileview->selected);
		return TRUE;
	      }
	      else {
		jwidget_emit_signal(widget, SIGNAL_FILEVIEW_FILE_ACCEPT);
		return TRUE;
	      }
	    }
	    else
	      return FALSE;
	  case KEY_BACKSPACE:
	    fileview_goup(widget);
	    return TRUE;
	  default:
	    return FALSE;
	}

	fileview_select_index(widget, MID(0, select, bottom));
	return TRUE;
      }
      break;

    case JM_WHEEL: {
      JWidget view = jwidget_get_view(widget);
      if (view) {
	int scroll_x, scroll_y;

	jview_get_scroll(view, &scroll_x, &scroll_y);
	jview_set_scroll(view,
			 scroll_x,
			 scroll_y +
			 (jmouse_z(1) - jmouse_z(0))
			 *(2+jwidget_get_text_height(widget)+2)*3);
      }
      break;
    }

    case JM_DOUBLECLICK:
      if (fileview->selected) {
	if (fileitem_is_browsable(fileview->selected)) {
	  fileview_set_current_folder(widget, fileview->selected);
	  return TRUE;
	}
	else {
	  jwidget_emit_signal(widget, SIGNAL_FILEVIEW_FILE_ACCEPT);
	  return TRUE;
	}
      }
      break;

  }

  return FALSE;
}

static void fileview_get_fileitem_size(JWidget widget, FileItem *fi, int *w, int *h)
{
/*   char buf[512]; */
  int len = 0;

  if (fileitem_is_folder(fi)) {
    len += ji_font_text_len(widget->text_font, "[+]")+2;
  }

  len += ji_font_text_len(widget->text_font,
			  fileitem_get_displayname(fi));

/*   if (!fileitem_is_folder(fi)) { */
/*     len += 2+ji_font_text_len(widget->text_font, buf); */
/*   } */

  *w = 2+len+2;
  *h = 2+jwidget_get_text_height(widget)+2;
}

static void fileview_make_selected_fileitem_visible(JWidget widget)
{
  FileView *fileview = fileview_data(widget);
  JWidget view = jwidget_get_view(widget);
  JRect vp = jview_get_viewport_position(view);
  FileItem *fi;
  JLink link;
  int iw, ih;
  int th = jwidget_get_text_height(widget);
  int y = widget->rc->y1;
  int scroll_x, scroll_y;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  /* rows */
  JI_LIST_FOR_EACH(fileview->list, link) {
    fi = link->data;
    fileview_get_fileitem_size(widget, fi, &iw, &ih);

    if (fi == fileview->selected) {
      if (y < vp->y1)
	jview_set_scroll(view, scroll_x, y - widget->rc->y1);
      else if (y > vp->y2 - (2+th+2))
	jview_set_scroll(view, scroll_x,
			 y - widget->rc->y1 - jrect_h(vp) + (2+th+2));
      break;
    }

    y += ih;
  }

  jrect_free(vp);
}

static void fileview_regenerate_list(JWidget widget)
{
  FileView *fileview = fileview_data(widget);
  FileItem *fileitem;
  JLink link, next;
  JList children;

  if (fileview->list)
    jlist_free(fileview->list);

  /* get the list of children */
  children = fileitem_get_children(fileview->current_folder);
  if (children) {
    fileview->list = jlist_copy(children);

    /* filter the list */
    if (fileview->exts) {
      JI_LIST_FOR_EACH_SAFE(fileview->list, link, next) {
	fileitem = link->data;
	if (!fileitem_is_folder(fileitem) &&
	    !fileitem_has_extension(fileitem, fileview->exts)) {
	  jlist_delete_link(fileview->list, link);
	}
      }
    }
  }
  else
    fileview->list = jlist_new();
}

static int fileview_get_selected_index(JWidget widget)
{
  FileView *fileview = fileview_data(widget);
  JLink link;
  int i = 0;

  JI_LIST_FOR_EACH(fileview->list, link) {
    if (link->data == fileview->selected)
      return i;
    i++;
  }

  return -1;
}

static void fileview_select_index(JWidget widget, int index)
{
  FileView *fileview = fileview_data(widget);
  FileItem *old_selected = fileview->selected;

  fileview->selected = jlist_nth_data(fileview->list, index);
  if (old_selected != fileview->selected) {
    fileview_make_selected_fileitem_visible(widget);

    jwidget_dirty(widget);
    jwidget_emit_signal(widget, SIGNAL_FILEVIEW_FILE_SELECTED);
  }
}
