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

#include <assert.h>
#include <allegro.h>

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "console/console.h"
#include "core/app.h"
#include "dialogs/filesel.h"
#include "file/file.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/recent.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/rotate.h"
#include "raster/sprite.h"
#include "widgets/fileview.h"
#include "widgets/statebar.h"

#define MAX_THUMBNAIL_SIZE		128
#define ISEARCH_KEYPRESS_INTERVAL_MSECS	500

typedef struct FileView
{
  FileItem *current_folder;
  JList list;
  bool req_valid;
  int req_w, req_h;
  FileItem *selected;
  const char *exts;

  /* incremental-search */
  char isearch[256];
  int isearch_clock;

  /* thumbnail generation process */
  FileItem *item_to_generate_thumbnail;
  int timer_id;
  JList monitors;	   /* list of monitors watching threads */
} FileView;

typedef struct ThumbnailData
{
  Monitor *monitor;
  FileOp *fop;
  FileItem *fileitem;
  JWidget fileview;
  Image *thumbnail;
  JThread thread;
  PALETTE rgbpal;
} ThumbnailData;

static FileView *fileview_data(JWidget widget);
static bool fileview_msg_proc(JWidget widget, JMessage msg);
static void fileview_get_fileitem_size(JWidget widget, FileItem *fi, int *w, int *h);
static void fileview_make_selected_fileitem_visible(JWidget widget);
static void fileview_regenerate_list(JWidget widget);
static int fileview_get_selected_index(JWidget widget);
static void fileview_select_index(JWidget widget, int index);
static void fileview_generate_preview_of_selected_item(JWidget widget);
static bool fileview_generate_thumbnail(JWidget widget, FileItem *fileitem);

static void openfile_bg(void *data);
static void monitor_thumbnail_generation(void *data);
static void monitor_free_thumbnail_generation(void *data);

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

  ustrcpy(fileview->isearch, empty_string);
  fileview->isearch_clock = 0;

  fileview->item_to_generate_thumbnail = NULL;
  fileview->timer_id = jmanager_add_timer(widget, 200);
  fileview->monitors = jlist_new();

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

    /* make the selected item visible */
    fileview_make_selected_fileitem_visible(widget);
  }
}

void fileview_stop_threads(JWidget widget)
{
  FileView *fileview = fileview_data(widget);
  JLink link, next;

  /* stop the generation of threads */
  jmanager_stop_timer(fileview->timer_id);

  /* join all threads (removing all monitors) */
  JI_LIST_FOR_EACH_SAFE(fileview->monitors, link, next) {
    remove_gui_monitor(link->data);
  }

  /* clear the list of monitors */
  jlist_clear(fileview->monitors);
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
      /* at this point, can't be threads running in background */
      assert(jlist_empty(fileview->monitors));

      jlist_free(fileview->monitors);

      if (fileview->list != NULL)
	jlist_free(fileview->list);

      jmanager_remove_timer(fileview->timer_id);
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
      JWidget view = jwidget_get_view(widget);
      JRect vp = jview_get_viewport_position(view);
      FileItem *fi;
      JLink link;
      int iw, ih;
      int th = jwidget_get_text_height(widget);
      int x, y = widget->rc->y1;
      int row = 0;
      int bgcolor;
      int fgcolor;
      BITMAP *thumbnail = NULL;
      int thumbnail_y = 0;

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
			  ji_color_background();

	  fgcolor =
	    fileitem_is_folder(fi) &&
	    !fileitem_is_browsable(fi) ? makecol(255, 200, 200):
					 ji_color_foreground();
	}

	x = widget->rc->x1+2;

	if (fileitem_is_folder(fi)) {
	  int icon_w = ji_font_text_len(widget->text_font, "[+]");
	  int icon_h = ji_font_get_size(widget->text_font);

	  jdraw_text(widget->text_font,
		     "[+]", x, y+2,
		     fgcolor, bgcolor, TRUE);

	  /* background for the icon */
	  jrectexclude(ji_screen,
		       /* rectangle to fill */
		       widget->rc->x1, y,
		       x+icon_w+2-1, y+2+th+2-1,
		       /* exclude where is the icon located */
		       x, y+2,
		       x+icon_w-1,
		       y+2+icon_h-1,
		       /* fill with the background color */
		       bgcolor);

	  x += icon_w+2;
	}
	else {
	  /* background for the left side of the item */
	  rectfill(ji_screen,
		   widget->rc->x1, y,
		   x-1, y+2+th+2-1,
		   bgcolor);
	}

	/* item name */
	jdraw_text(widget->text_font,
		   fileitem_get_displayname(fi), x, y+2,
		   fgcolor, bgcolor, TRUE);

	/* background for the item name */
	jrectexclude(ji_screen,
		     /* rectangle to fill */
		     x, y,
		     widget->rc->x2-1, y+2+th+2-1,
		     /* exclude where is the text located */
		     x, y+2,
		     x+ji_font_text_len(widget->text_font,
					fileitem_get_displayname(fi))-1,
		     y+2+ji_font_get_size(widget->text_font)-1,
		     /* fill with the background color */
		     bgcolor);

	/* thumbnail position */
	if (fi == fileview->selected) {
	  thumbnail = fileitem_get_thumbnail(fi);
	  if (thumbnail)
	    thumbnail_y = y + ih/2;
	}

	y += ih;
	row ^= 1;
      }

      if (y < widget->rc->y2-1)
	rectfill(ji_screen,
		 widget->rc->x1, y,
		 widget->rc->x2-1, widget->rc->y2-1,
		 ji_color_background());

      /* draw the thumbnail */
      if (thumbnail) {
	x = vp->x2-2-thumbnail->w;
	y = thumbnail_y-thumbnail->h/2;
	y = MID(vp->y1+2, y, vp->y2-3-thumbnail->h);

	blit(thumbnail, ji_screen, 0, 0, x, y, thumbnail->w, thumbnail->h);
	rect(ji_screen,
	     x-1, y-1, x+thumbnail->w, y+thumbnail->h,
	     makecol(0, 0, 0));
      }

      /* is the current folder empty? */
      if (jlist_empty(fileview->list))
	draw_emptyset_symbol(vp, makecol(194, 194, 194));

      jrect_free(vp);
      break;
    }

    case JM_BUTTONPRESSED:
      jwidget_hard_capture_mouse(widget);

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
	  fileview_generate_preview_of_selected_item(widget);

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

    case JM_KEYPRESSED:
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
	    if (msg->key.ascii == ' ' ||
		(utolower(msg->key.ascii) >= 'a' &&
		 utolower(msg->key.ascii) <= 'z') ||
		(utolower(msg->key.ascii) >= '0' &&
		 utolower(msg->key.ascii) <= '9')) {
	      if (ji_clock - fileview->isearch_clock > ISEARCH_KEYPRESS_INTERVAL_MSECS)
		ustrcpy(fileview->isearch, empty_string);

	      usprintf(fileview->isearch+ustrsize(fileview->isearch),
		       "%c", msg->key.ascii);

	      {
		int i, chrs = ustrlen(fileview->isearch);
		JLink link = jlist_nth_link(fileview->list,
					    (select >= 0) ? select: 0);

		for (i=select; i<=bottom; ++i, link=link->next) {
		  FileItem *fi = link->data;
		  if (ustrnicmp(fileitem_get_displayname(fi),
				fileview->isearch,
				chrs) == 0) {
		    select = i;
		    break;
		  }
		}
	      }
	      fileview->isearch_clock = ji_clock;
	      /* go to fileview_select_index... */
	    }
	    else
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

    case JM_TIMER:
      /* is time to generate the thumbnail? */
      if (msg->timer.timer_id == fileview->timer_id) {
	FileItem *fileitem;

	jmanager_stop_timer(fileview->timer_id);

	fileitem = fileview->item_to_generate_thumbnail;
	fileview_generate_thumbnail(widget, fileitem);
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

  if (fileview->list != NULL)
    jlist_free(fileview->list);

  /* get the list of children */
  children = fileitem_get_children(fileview->current_folder);
  if (children != NULL) {
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

  fileview_generate_preview_of_selected_item(widget);
}

/* puts the selected file-item as the next item to be processed by the
   round-robin that generate thumbnails */
static void fileview_generate_preview_of_selected_item(JWidget widget)
{
  FileView *fileview = fileview_data(widget);

  if (fileview->selected &&
      !fileitem_is_folder(fileview->selected) &&
      !fileitem_get_thumbnail(fileview->selected))
    {
      fileview->item_to_generate_thumbnail = fileview->selected;
      jmanager_start_timer(fileview->timer_id);
    }
}

/* returns true if it does some hard work like access to the disk */
static bool fileview_generate_thumbnail(JWidget widget, FileItem *fileitem)
{
  FileOp *fop;

  if (fileitem_is_browsable(fileitem) ||
      fileitem_get_thumbnail(fileitem) != NULL)
    return FALSE;

  fop = fop_to_load_sprite(fileitem_get_filename(fileitem),
			   FILE_LOAD_SEQUENCE_NONE |
			   FILE_LOAD_ONE_FRAME);
  if (!fop)
    return TRUE;

  if (fop->error) {
    fop_free(fop);
  }
  else {
    ThumbnailData *data = jnew(ThumbnailData, 1);

    data->fop = fop;
    data->fileitem = fileitem;
    data->fileview = widget;
    data->thumbnail = NULL;

    data->thread = jthread_new(openfile_bg, data);
    if (data->thread) {
      /* add a monitor to check the loading (FileOp) progress */
      data->monitor = add_gui_monitor(monitor_thumbnail_generation,
				      monitor_free_thumbnail_generation, data);

      jlist_append(fileview_data(widget)->monitors, data->monitor);
      jwidget_dirty(widget);
    }
    else {
      fop_free(fop);
      jfree(data);
    }
  }

  return TRUE;
}

/**
 * Thread to do the hard work: load the file from the disk (in
 * background).
 *
 * [loading thread]
 */
static void openfile_bg(void *_data)
{
  ThumbnailData *data = (ThumbnailData *)_data;
  FileOp *fop = (FileOp *)data->fop;
  Sprite *sprite;
  int thumb_w, thumb_h;
  Image *image;

  /* load the file */
  fop_operate(fop);

  sprite = fop->sprite;
  if (sprite) {
    if (fop_is_stop(fop))
      sprite_free(fop->sprite);
    else {
      /* the palette to convert the Image to a BITMAP */
      palette_to_allegro(sprite_get_palette(sprite, 0), data->rgbpal);

      /* render the 'sprite' in one plain 'image' */
      image = image_new(sprite->imgtype, sprite->w, sprite->h);
      image_clear(image, 0);
      sprite_render(sprite, image, 0, 0);
      sprite_free(sprite);

      /* calculate the thumbnail size */
      thumb_w = MAX_THUMBNAIL_SIZE * image->w / MAX(image->w, image->h);
      thumb_h = MAX_THUMBNAIL_SIZE * image->h / MAX(image->w, image->h);
      if (MAX(thumb_w, thumb_h) > MAX(image->w, image->h)) {
	thumb_w = image->w;
	thumb_h = image->h;
      }
      thumb_w = MID(1, thumb_w, MAX_THUMBNAIL_SIZE);
      thumb_h = MID(1, thumb_h, MAX_THUMBNAIL_SIZE);

      /* stretch the 'image' */
      data->thumbnail = image_new(image->imgtype, thumb_w, thumb_h);
      image_clear(data->thumbnail, 0);
      image_scale(data->thumbnail, image, 0, 0, thumb_w, thumb_h);
      image_free(image);
    }
  }

  fop_done(fop);
}

/**
 * Called by the gui-monitor (a timer in the gui module that is called
 * every 100 milliseconds).
 * 
 * [main thread]
 */
static void monitor_thumbnail_generation(void *_data)
{
  ThumbnailData *data = (ThumbnailData *)_data;
  FileOp *fop = (FileOp *)data->fop;

  /* is done? ...ok, now the thumbnail is in the main thread only... */
  if (fop_is_done(fop)) {
    /* set the thumbnail of the file-item */
    if (data->thumbnail) {
      BITMAP *bmp = create_bitmap_ex(16,
				     data->thumbnail->w,
				     data->thumbnail->h);

      select_palette(data->rgbpal);
      image_to_allegro(data->thumbnail, bmp, 0, 0);
      unselect_palette();

      image_free(data->thumbnail);

      fileitem_set_thumbnail(data->fileitem, bmp);

      /* is the selected file-item the one that now has a thumbnail? */
      if (fileview_get_selected(data->fileview) == data->fileitem) {
	/* we have to dirty the file-view to show the thumbnail */
	jwidget_dirty(data->fileview);
      }
    }

    remove_gui_monitor(data->monitor);
  }
}

/**
 * [main thread]
 */
static void monitor_free_thumbnail_generation(void *_data)
{
  ThumbnailData *data = (ThumbnailData *)_data;
  FileOp *fop = (FileOp *)data->fop;

  fop_stop(fop);
  jthread_join(data->thread);

  jlist_remove(fileview_data(data->fileview)->monitors,
	       data->monitor);

  fop_free(fop);
  jfree(data);
}
