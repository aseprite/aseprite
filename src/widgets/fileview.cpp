/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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
#include <algorithm>

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "console.h"
#include "app.h"
#include "dialogs/filesel.h"
#include "file/file.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
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
  IFileItem* current_folder;
  FileItemList list;
  bool req_valid;
  int req_w, req_h;
  IFileItem* selected;
  jstring exts;

  /* incremental-search */
  char isearch[256];
  int isearch_clock;

  /* thumbnail generation process */
  IFileItem* item_to_generate_thumbnail;
  int timer_id;
  MonitorList monitors; // list of monitors watching threads
} FileView;

typedef struct ThumbnailData
{
  Monitor* monitor;
  FileOp* fop;
  IFileItem* fileitem;
  JWidget fileview;
  Image* thumbnail;
  JThread thread;
  Palette* palette;
} ThumbnailData;

static FileView* fileview_data(JWidget widget);
static bool fileview_msg_proc(JWidget widget, JMessage msg);
static void fileview_get_fileitem_size(JWidget widget, IFileItem* fi, int *w, int *h);
static void fileview_make_selected_fileitem_visible(JWidget widget);
static void fileview_regenerate_list(JWidget widget);
static int fileview_get_selected_index(JWidget widget);
static void fileview_select_index(JWidget widget, int index);
static void fileview_generate_preview_of_selected_item(JWidget widget);
static bool fileview_generate_thumbnail(JWidget widget, IFileItem* fileitem);
static void fileview_stop_threads(FileView* fileview);

static void openfile_bg(void *data);
static void monitor_thumbnail_generation(void *data);
static void monitor_free_thumbnail_generation(void *data);

JWidget fileview_new(IFileItem* start_folder, const jstring& exts)
{
  Widget* widget = new Widget(fileview_type());
  FileView* fileview = new FileView;

  if (!start_folder)
    start_folder = FileSystemModule::instance()->getRootFileItem();
  else {
    while (!start_folder->isFolder() &&
	   start_folder->getParent() != NULL) {
      start_folder = start_folder->getParent();
    }
  }

  jwidget_add_hook(widget, fileview_type(),
		   fileview_msg_proc, fileview);
  jwidget_focusrest(widget, true);

  fileview->current_folder = start_folder;
  fileview->req_valid = false;
  fileview->selected = NULL;
  fileview->exts = exts;

  ustrcpy(fileview->isearch, empty_string);
  fileview->isearch_clock = 0;

  fileview->item_to_generate_thumbnail = NULL;
  fileview->timer_id = jmanager_add_timer(widget, 200);

  fileview_regenerate_list(widget);

  return widget;
}

int fileview_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

IFileItem* fileview_get_current_folder(JWidget widget)
{
  return fileview_data(widget)->current_folder;
}

IFileItem* fileview_get_selected(JWidget widget)
{
  return fileview_data(widget)->selected;
}

void fileview_set_current_folder(JWidget widget, IFileItem* folder)
{
  FileView* fileview = fileview_data(widget);

  ASSERT(folder != NULL);
  ASSERT(folder->isBrowsable());

  fileview->current_folder = folder;
  fileview->req_valid = false;
  fileview->selected = NULL;

  fileview_regenerate_list(widget);

  // select first folder
  if (!fileview->list.empty() && fileview->list.front()->isBrowsable())
    fileview_select_index(widget, 0);

  jwidget_emit_signal(widget, SIGNAL_FILEVIEW_CURRENT_FOLDER_CHANGED);

  jwidget_dirty(widget);
  jview_update(jwidget_get_view(widget));
}

const FileItemList& fileview_get_filelist(JWidget widget)
{
  FileView* fileview = fileview_data(widget);

  return fileview->list;
}

void fileview_goup(JWidget widget)
{
  FileView* fileview = fileview_data(widget);
  IFileItem* folder = fileview->current_folder;
  IFileItem* parent = folder->getParent();
  if (parent) {
    fileview_set_current_folder(widget, parent);
    fileview->selected = folder;

    /* make the selected item visible */
    fileview_make_selected_fileitem_visible(widget);
  }
}

static FileView* fileview_data(JWidget widget)
{
  return reinterpret_cast<FileView*>(jwidget_get_data(widget, fileview_type()));
}

static bool fileview_msg_proc(JWidget widget, JMessage msg)
{
  FileView* fileview = fileview_data(widget);

  switch (msg->type) {

    case JM_DESTROY:
      fileview_stop_threads(fileview);

      // at this point, can't be threads running in background
      ASSERT(fileview->monitors.empty());

      jmanager_remove_timer(fileview->timer_id);
      delete fileview;
      break;

    case JM_REQSIZE:
      if (!fileview->req_valid) {
	int w, h, iw, ih;

	w = 0;
	h = 0;

	// rows
	for (FileItemList::iterator
	       it=fileview->list.begin();
	     it!=fileview->list.end(); ++it) {
	  IFileItem* fi = *it;
	  fileview_get_fileitem_size(widget, fi, &iw, &ih);
	  w = MAX(w, iw);
	  h += ih;
	}

	fileview->req_valid = true;
	fileview->req_w = w;
	fileview->req_h = h;
      }

      msg->reqsize.w = fileview->req_w;
      msg->reqsize.h = fileview->req_h;
      return true;

    case JM_DRAW: {
      JWidget view = jwidget_get_view(widget);
      JRect vp = jview_get_viewport_position(view);
      int iw, ih;
      int th = jwidget_get_text_height(widget);
      int x, y = widget->rc->y1;
      int row = 0;
      int bgcolor;
      int fgcolor;
      BITMAP *thumbnail = NULL;
      int thumbnail_y = 0;

      // rows
      for (FileItemList::iterator
	     it=fileview->list.begin();
	   it!=fileview->list.end(); ++it) {
	IFileItem* fi = *it;
	fileview_get_fileitem_size(widget, fi, &iw, &ih);
	
	if (fi == fileview->selected) {
	  bgcolor = ji_color_selected();
	  fgcolor = ji_color_background();
	}
	else {
	  bgcolor = row ? makecol(240, 240, 240):
			  ji_color_background();

	  fgcolor =
	    fi->isFolder() &&
	    !fi->isBrowsable() ? makecol(255, 200, 200):
				 ji_color_foreground();
	}

	x = widget->rc->x1+2;

	if (fi->isFolder()) {
	  int icon_w = ji_font_text_len(widget->getFont(), "[+]");
	  int icon_h = ji_font_get_size(widget->getFont());

	  jdraw_text(ji_screen, widget->getFont(),
		     "[+]", x, y+2,
		     fgcolor, bgcolor, true, jguiscale());

	  // background for the icon
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
	  // background for the left side of the item
	  rectfill(ji_screen,
		   widget->rc->x1, y,
		   x-1, y+2+th+2-1,
		   bgcolor);
	}

	// item name
	jdraw_text(ji_screen, widget->getFont(),
		   fi->getDisplayName().c_str(), x, y+2,
		   fgcolor, bgcolor, true, jguiscale());

	// background for the item name
	jrectexclude(ji_screen,
		     /* rectangle to fill */
		     x, y,
		     widget->rc->x2-1, y+2+th+2-1,
		     /* exclude where is the text located */
		     x, y+2,
		     x+ji_font_text_len(widget->getFont(),
					fi->getDisplayName().c_str())-1,
		     y+2+ji_font_get_size(widget->getFont())-1,
		     /* fill with the background color */
		     bgcolor);

	// draw progress bar
	if (!fileview->monitors.empty()) {
	  for (MonitorList::iterator
		 it2 = fileview->monitors.begin();
	       it2 != fileview->monitors.end(); ++it2) {
	    Monitor* monitor = *it2;
	    ThumbnailData* data = (ThumbnailData*)get_monitor_data(monitor);

	    // Check if this monitor is for this file-item
	    if (data->fileitem == fi) {
	      // If the file operation is not done, means that we are
	      // still loading the file, so we can show a progress bar
	      if (!fop_is_done(data->fop)) {
		float progress = fop_get_progress(data->fop);

		draw_progress_bar(ji_screen,
				  widget->rc->x2-2-64, y+ih/2-3,
				  widget->rc->x2-2, y+ih/2+3,
				  progress);
	      }
	      break;
	    }
	  }
	}

	// thumbnail position
	if (fi == fileview->selected) {
	  thumbnail = fi->getThumbnail();
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

      // is the current folder empty?
      if (fileview->list.empty())
	draw_emptyset_symbol(ji_screen,
			     Rect(vp->x1, vp->y1, jrect_w(vp), jrect_h(vp)),
			     makecol(194, 194, 194));

      jrect_free(vp);
      break;
    }

    case JM_BUTTONPRESSED:
      widget->captureMouse();

    case JM_MOTION:
      if (widget->hasCapture()) {
	int iw, ih;
	int th = jwidget_get_text_height(widget);
	int y = widget->rc->y1;
	IFileItem* old_selected = fileview->selected;
	fileview->selected = NULL;

	// rows
	for (FileItemList::iterator
	       it=fileview->list.begin();
	     it!=fileview->list.end(); ++it) {
	  IFileItem* fi = *it;
	  fileview_get_fileitem_size(widget, fi, &iw, &ih);

	  if (((msg->mouse.y >= y) && (msg->mouse.y < y+2+th+2)) ||
	      (it == fileview->list.begin() && msg->mouse.y < y) ||
	      (it == fileview->list.end()-1 && msg->mouse.y >= y+2+th+2)) {
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
      if (widget->hasCapture()) {
	widget->releaseMouse();
      }
      break;

    case JM_KEYPRESSED:
      if (widget->hasFocus()) {
	int select = fileview_get_selected_index(widget);
	JWidget view = jwidget_get_view(widget);
	int bottom = fileview->list.size();

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
	    select = bottom-1;
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
	      if (fileview->selected->isBrowsable()) {
		fileview_set_current_folder(widget, fileview->selected);
		return true;
	      }
	      if (fileview->selected->isFolder()) {
		// do nothing (is a folder but not browseable
		return true;
	      }
	      else {
		// a file was selected
		jwidget_emit_signal(widget, SIGNAL_FILEVIEW_FILE_ACCEPT);
		return true;
	      }
	    }
	    else
	      return false;
	  case KEY_BACKSPACE:
	    fileview_goup(widget);
	    return true;
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
		FileItemList::iterator
		  link = fileview->list.begin() + ((select >= 0) ? select: 0);

		for (i=MAX(select, 0); i<bottom; ++i, ++link) {
		  IFileItem* fi = *link;
		  if (ustrnicmp(fi->getDisplayName().c_str(),
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
	      return false;
	}

	if (bottom > 0)
	  fileview_select_index(widget, MID(0, select, bottom-1));
	return true;
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
	if (fileview->selected->isBrowsable()) {
	  fileview_set_current_folder(widget, fileview->selected);
	  return true;
	}
	else {
	  jwidget_emit_signal(widget, SIGNAL_FILEVIEW_FILE_ACCEPT);
	  return true;
	}
      }
      break;

    case JM_TIMER:
      /* is time to generate the thumbnail? */
      if (msg->timer.timer_id == fileview->timer_id) {
	IFileItem* fileitem;

	jmanager_stop_timer(fileview->timer_id);

	fileitem = fileview->item_to_generate_thumbnail;
	fileview_generate_thumbnail(widget, fileitem);
      }
      break;

  }

  return false;
}

static void fileview_get_fileitem_size(JWidget widget, IFileItem* fi, int *w, int *h)
{
/*   char buf[512]; */
  int len = 0;

  if (fi->isFolder()) {
    len += ji_font_text_len(widget->getFont(), "[+]")+2;
  }

  len += ji_font_text_len(widget->getFont(),
			  fi->getDisplayName().c_str());

/*   if (!fileitem_is_folder(fi)) { */
/*     len += 2+ji_font_text_len(widget->text_font, buf); */
/*   } */

  *w = 2+len+2;
  *h = 2+jwidget_get_text_height(widget)+2;
}

static void fileview_make_selected_fileitem_visible(JWidget widget)
{
  FileView* fileview = fileview_data(widget);
  JWidget view = jwidget_get_view(widget);
  JRect vp = jview_get_viewport_position(view);
  int iw, ih;
  int th = jwidget_get_text_height(widget);
  int y = widget->rc->y1;
  int scroll_x, scroll_y;

  jview_get_scroll(view, &scroll_x, &scroll_y);

  // rows
  for (FileItemList::iterator
	 it=fileview->list.begin();
       it!=fileview->list.end(); ++it) {
    IFileItem* fi = *it;
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
  FileView* fileview = fileview_data(widget);

  // get the children of the current folder
  fileview->list = fileview->current_folder->getChildren();

  // filter the list by the available extensions
  if (!fileview->exts.empty()) {
    for (FileItemList::iterator
	   it=fileview->list.begin();
	 it!=fileview->list.end(); ) {
      IFileItem* fileitem = *it;
      if (!fileitem->isFolder() &&
	  !fileitem->hasExtension(fileview->exts.c_str())) {
	it = fileview->list.erase(it);
      }
      else
	++it;
    }
  }
}

static int fileview_get_selected_index(JWidget widget)
{
  FileView* fileview = fileview_data(widget);

  for (FileItemList::iterator
	 it = fileview->list.begin();
       it != fileview->list.end(); ++it) {
    if (*it == fileview->selected)
      return it - fileview->list.begin();
  }

  return -1;
}

static void fileview_select_index(JWidget widget, int index)
{
  FileView* fileview = fileview_data(widget);
  IFileItem* old_selected = fileview->selected;

  fileview->selected = fileview->list.at(index);
  if (old_selected != fileview->selected) {
    fileview_make_selected_fileitem_visible(widget);
    
    jwidget_dirty(widget);
    jwidget_emit_signal(widget, SIGNAL_FILEVIEW_FILE_SELECTED);
  }

  fileview_generate_preview_of_selected_item(widget);
}

/**
 * Puts the selected file-item as the next item to be processed by the
 * round-robin that generate thumbnails
 */
static void fileview_generate_preview_of_selected_item(JWidget widget)
{
  FileView* fileview = fileview_data(widget);

  if (fileview->selected &&
      !fileview->selected->isFolder() &&
      !fileview->selected->getThumbnail())
    {
      fileview->item_to_generate_thumbnail = fileview->selected;
      jmanager_start_timer(fileview->timer_id);
    }
}

/* returns true if it does some hard work like access to the disk */
static bool fileview_generate_thumbnail(JWidget widget, IFileItem* fileitem)
{
  if (fileitem->isBrowsable() ||
      fileitem->getThumbnail() != NULL)
    return false;

  FileOp* fop =
    fop_to_load_sprite(fileitem->getFileName().c_str(),
		       FILE_LOAD_SEQUENCE_NONE |
		       FILE_LOAD_ONE_FRAME);
  if (!fop)
    return true;

  if (fop->error) {
    fop_free(fop);
  }
  else {
    ThumbnailData* data = new ThumbnailData;

    data->fop = fop;
    data->fileitem = fileitem;
    data->fileview = widget;
    data->thumbnail = NULL;

    data->thread = jthread_new(openfile_bg, data);
    if (data->thread) {
      // add a monitor to check the loading (FileOp) progress
      data->monitor = add_gui_monitor(monitor_thumbnail_generation,
				      monitor_free_thumbnail_generation, data);

      fileview_data(widget)->monitors.push_back(data->monitor);
      jwidget_dirty(widget);
    }
    else {
      fop_free(fop);
      delete data;
    }
  }

  return true;
}

static void fileview_stop_threads(FileView* fileview)
{
  // stop the generation of threads
  jmanager_stop_timer(fileview->timer_id);

  // join all threads (removing all monitors)
  for (MonitorList::iterator
	 it = fileview->monitors.begin();
       it != fileview->monitors.end(); ) {
    Monitor* monitor = *it;
    ++it;
    remove_gui_monitor(monitor);
  }

  // clear the list of monitors
  fileview->monitors.clear();
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
      delete fop->sprite;
    else {
      // The palette to convert the Image to a BITMAP
      data->palette = new Palette(*sprite->getPalette(0));

      // Render the 'sprite' in one plain 'image'
      image = image_new(sprite->getImgType(), sprite->getWidth(), sprite->getHeight());
      image_clear(image, 0);
      sprite->render(image, 0, 0);
      delete sprite;

      // Calculate the thumbnail size
      thumb_w = MAX_THUMBNAIL_SIZE * image->w / MAX(image->w, image->h);
      thumb_h = MAX_THUMBNAIL_SIZE * image->h / MAX(image->w, image->h);
      if (MAX(thumb_w, thumb_h) > MAX(image->w, image->h)) {
	thumb_w = image->w;
	thumb_h = image->h;
      }
      thumb_w = MID(1, thumb_w, MAX_THUMBNAIL_SIZE);
      thumb_h = MID(1, thumb_h, MAX_THUMBNAIL_SIZE);

      // Stretch the 'image'
      data->thumbnail = image_new(image->imgtype, thumb_w, thumb_h);
      image_clear(data->thumbnail, 0);
      image_scale(data->thumbnail, image, 0, 0, thumb_w, thumb_h);
      image_free(image);
    }
  }

  fop_done(fop);
}

/**
 * Called by the GUI-monitor (a timer in the gui module that is called
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

      image_to_allegro(data->thumbnail, bmp, 0, 0, data->palette);

      delete data->thumbnail;	// image
      delete data->palette;
      data->thumbnail = NULL;
      data->palette = NULL;

      data->fileitem->setThumbnail(bmp);

      /* is the selected file-item the one that now has a thumbnail? */
      if (fileview_get_selected(data->fileview) == data->fileitem) {
	/* we have to dirty the file-view to show the thumbnail */
	jwidget_dirty(data->fileview);
      }
    }

    remove_gui_monitor(data->monitor);
  }
  else {
    jwidget_dirty(data->fileview);
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

  // remove the monitor from the list
  MonitorList& monitors(fileview_data(data->fileview)->monitors);
  MonitorList::iterator it =
    std::find(monitors.begin(), monitors.end(), data->monitor);
  ASSERT(it != monitors.end());
  monitors.erase(it);

  // destroy the thumbnail
  if (data->thumbnail) {
    image_free(data->thumbnail);
    data->thumbnail = NULL;
  }

  fop_free(fop);
  jfree(data);
}
