/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "widgets/file_list.h"

#include "app.h"
#include "base/thread.h"
#include "commands/commands.h"
#include "console.h"
#include "document.h"
#include "file/file.h"
#include "gui/gui.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/rotate.h"
#include "raster/sprite.h"
#include "widgets/statebar.h"

#include <algorithm>
#include <allegro.h>

#define MAX_THUMBNAIL_SIZE              128
#define ISEARCH_KEYPRESS_INTERVAL_MSECS 500

using namespace gfx;

namespace widgets {

struct ThumbnailData
{
  Monitor* monitor;
  FileOp* fop;
  IFileItem* fileitem;
  FileList* fileview;
  Image* thumbnail;
  base::thread* thread;
  Palette* palette;
};

static void openfile_bg(ThumbnailData* data);
static void monitor_thumbnail_generation(void *data);
static void monitor_free_thumbnail_generation(void *data);

FileList::FileList()
  : Widget(JI_WIDGET)
  , m_timer(this, 200)
{
  setFocusStop(true);

  m_currentFolder = FileSystemModule::instance()->getRootFileItem();
  m_req_valid = false;
  m_selected = NULL;
  m_isearchClock = 0;

  m_itemToGenerateThumbnail = NULL;

  regenerateList();
}

FileList::~FileList()
{
  stopThreads();

  // at this point, can't be threads running in background
  ASSERT(m_monitors.empty());
}

void FileList::setExtensions(const char* extensions)
{
  m_exts = extensions;
}

void FileList::setCurrentFolder(IFileItem* folder)
{
  ASSERT(folder != NULL);
  ASSERT(folder->isBrowsable());

  m_currentFolder = folder;
  m_req_valid = false;
  m_selected = NULL;

  regenerateList();

  // select first folder
  if (!m_list.empty() && m_list.front()->isBrowsable())
    selectIndex(0);

  // Emit "CurrentFolderChanged" event.
  onCurrentFolderChanged();

  invalidate();
  View::getView(this)->updateView();
}

void FileList::goUp()
{
  IFileItem* folder = m_currentFolder;
  IFileItem* parent = folder->getParent();
  if (parent) {
    setCurrentFolder(parent);
    m_selected = folder;

    // Make the selected item visible.
    makeSelectedFileitemVisible();
  }
}

bool FileList::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      if (!m_req_valid) {
        gfx::Size reqSize(0, 0);

        // rows
        for (FileItemList::iterator
               it=m_list.begin();
             it!=m_list.end(); ++it) {
          IFileItem* fi = *it;
          gfx::Size itemSize = getFileItemSize(fi);
          reqSize.w = MAX(reqSize.w, itemSize.w);
          reqSize.h += itemSize.h;
        }

        m_req_valid = true;
        m_req_w = reqSize.w;
        m_req_h = reqSize.h;
      }
      msg->reqsize.w = m_req_w;
      msg->reqsize.h = m_req_h;
      return true;

    case JM_DRAW: {
      View* view = View::getView(this);
      gfx::Rect vp = view->getViewportBounds();
      int th = jwidget_get_text_height(this);
      int x, y = this->rc->y1;
      int row = 0;
      int bgcolor;
      int fgcolor;
      BITMAP *thumbnail = NULL;
      int thumbnail_y = 0;

      // rows
      for (FileItemList::iterator
             it=m_list.begin();
           it!=m_list.end(); ++it) {
        IFileItem* fi = *it;
        gfx::Size itemSize = getFileItemSize(fi);

        if (fi == m_selected) {
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

        x = this->rc->x1+2;

        if (fi->isFolder()) {
          int icon_w = ji_font_text_len(getFont(), "[+]");
          int icon_h = ji_font_get_size(getFont());

          jdraw_text(ji_screen, getFont(),
                     "[+]", x, y+2,
                     fgcolor, bgcolor, true, jguiscale());

          // background for the icon
          jrectexclude(ji_screen,
                       /* rectangle to fill */
                       this->rc->x1, y,
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
                   this->rc->x1, y,
                   x-1, y+2+th+2-1,
                   bgcolor);
        }

        // item name
        jdraw_text(ji_screen, getFont(),
                   fi->getDisplayName().c_str(), x, y+2,
                   fgcolor, bgcolor, true, jguiscale());

        // background for the item name
        jrectexclude(ji_screen,
                     /* rectangle to fill */
                     x, y,
                     this->rc->x2-1, y+2+th+2-1,
                     /* exclude where is the text located */
                     x, y+2,
                     x+ji_font_text_len(this->getFont(),
                                        fi->getDisplayName().c_str())-1,
                     y+2+ji_font_get_size(getFont())-1,
                     /* fill with the background color */
                     bgcolor);

        // draw progress bar
        if (!m_monitors.empty()) {
          for (MonitorList::iterator
                 it2 = m_monitors.begin();
               it2 != m_monitors.end(); ++it2) {
            Monitor* monitor = *it2;
            ThumbnailData* data = (ThumbnailData*)get_monitor_data(monitor);

            // Check if this monitor is for this file-item
            if (data->fileitem == fi) {
              // If the file operation is not done, means that we are
              // still loading the file, so we can show a progress bar
              if (!fop_is_done(data->fop)) {
                float progress = fop_get_progress(data->fop);

                draw_progress_bar(ji_screen,
                                  this->rc->x2-2-64, y+itemSize.h/2-3,
                                  this->rc->x2-2, y+itemSize.h/2+3,
                                  progress);
              }
              break;
            }
          }
        }

        // thumbnail position
        if (fi == m_selected) {
          thumbnail = fi->getThumbnail();
          if (thumbnail)
            thumbnail_y = y + itemSize.h/2;
        }

        y += itemSize.h;
        row ^= 1;
      }

      if (y < this->rc->y2-1)
        rectfill(ji_screen,
                 this->rc->x1, y,
                 this->rc->x2-1, this->rc->y2-1,
                 ji_color_background());

      /* draw the thumbnail */
      if (thumbnail) {
        x = vp.x+vp.w-2-thumbnail->w;
        y = thumbnail_y-thumbnail->h/2;
        y = MID(vp.y+2, y, vp.y+vp.h-3-thumbnail->h);

        blit(thumbnail, ji_screen, 0, 0, x, y, thumbnail->w, thumbnail->h);
        rect(ji_screen,
             x-1, y-1, x+thumbnail->w, y+thumbnail->h,
             makecol(0, 0, 0));
      }

      // is the current folder empty?
      if (m_list.empty())
        draw_emptyset_symbol(ji_screen, vp, makecol(194, 194, 194));
      return true;
    }

    case JM_BUTTONPRESSED:
      captureMouse();

    case JM_MOTION:
      if (hasCapture()) {
        int th = jwidget_get_text_height(this);
        int y = this->rc->y1;
        IFileItem* old_selected = m_selected;
        m_selected = NULL;

        // rows
        for (FileItemList::iterator
               it=m_list.begin();
             it!=m_list.end(); ++it) {
          IFileItem* fi = *it;
          gfx::Size itemSize = getFileItemSize(fi);

          if (((msg->mouse.y >= y) && (msg->mouse.y < y+2+th+2)) ||
              (it == m_list.begin() && msg->mouse.y < y) ||
              (it == m_list.end()-1 && msg->mouse.y >= y+2+th+2)) {
            m_selected = fi;
            makeSelectedFileitemVisible();
            break;
          }

          y += itemSize.h;
        }

        if (old_selected != m_selected) {
          generatePreviewOfSelectedItem();

          invalidate();

          // Emit "FileSelected" event.
          onFileSelected();
        }
      }
      break;

    case JM_BUTTONRELEASED:
      if (hasCapture()) {
        releaseMouse();
      }
      break;

    case JM_KEYPRESSED:
      if (hasFocus()) {
        int select = getSelectedIndex();
        View* view = View::getView(this);
        int bottom = m_list.size();

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
            gfx::Rect vp = view->getViewportBounds();
            if (select < 0)
              select = 0;
            select += sgn * vp.h / (2+jwidget_get_text_height(this)+2);
            break;
          }
          case KEY_LEFT:
          case KEY_RIGHT:
            if (select >= 0) {
              gfx::Rect vp = view->getViewportBounds();
              int sgn = (msg->key.scancode == KEY_LEFT) ? -1: 1;
              gfx::Point scroll = view->getViewScroll();
              scroll.x += vp.w/2*sgn;
              view->setViewScroll(scroll);
            }
            break;
          case KEY_ENTER:
            if (m_selected) {
              if (m_selected->isBrowsable()) {
                setCurrentFolder(m_selected);
                return true;
              }
              if (m_selected->isFolder()) {
                // Do nothing (is a folder but not browseable.
                return true;
              }
              else {
                // Emit "FileAccepted" event.
                onFileAccepted();
                return true;
              }
            }
            else
              return Widget::onProcessMessage(msg);
          case KEY_BACKSPACE:
            goUp();
            return true;
          default:
            if (msg->key.ascii == ' ' ||
                (utolower(msg->key.ascii) >= 'a' &&
                 utolower(msg->key.ascii) <= 'z') ||
                (utolower(msg->key.ascii) >= '0' &&
                 utolower(msg->key.ascii) <= '9')) {
              if (ji_clock - m_isearchClock > ISEARCH_KEYPRESS_INTERVAL_MSECS)
                m_isearch.clear();

              m_isearch.push_back(msg->key.ascii);

              int i, chrs = m_isearch.size();
              FileItemList::iterator
                link = m_list.begin() + ((select >= 0) ? select: 0);

              for (i=MAX(select, 0); i<bottom; ++i, ++link) {
                IFileItem* fi = *link;
                if (ustrnicmp(fi->getDisplayName().c_str(),
                              m_isearch.c_str(),
                              chrs) == 0) {
                  select = i;
                  break;
                }
              }
              m_isearchClock = ji_clock;
              // Go to selectIndex...
            }
            else
              return Widget::onProcessMessage(msg);
        }

        if (bottom > 0)
          selectIndex(MID(0, select, bottom-1));

        return true;
      }
      break;

    case JM_WHEEL: {
      View* view = View::getView(this);
      if (view) {
        gfx::Point scroll = view->getViewScroll();
        scroll.y += (jmouse_z(1)-jmouse_z(0)) * 3*(2+jwidget_get_text_height(this)+2);
        view->setViewScroll(scroll);
      }
      break;
    }

    case JM_DOUBLECLICK:
      if (m_selected) {
        if (m_selected->isBrowsable()) {
          setCurrentFolder(m_selected);
          return true;
        }
        else {
          onFileAccepted();         // Emit "FileAccepted" event.
          return true;
        }
      }
      break;

    case JM_TIMER:
      /* is time to generate the thumbnail? */
      if (msg->timer.timer == &m_timer) {
        IFileItem* fileitem;

        m_timer.stop();

        fileitem = m_itemToGenerateThumbnail;
        generateThumbnail(fileitem);
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void FileList::onFileSelected()
{
  FileSelected();
}

void FileList::onFileAccepted()
{
  FileAccepted();
}

void FileList::onCurrentFolderChanged()
{
  CurrentFolderChanged();
}

gfx::Size FileList::getFileItemSize(IFileItem* fi) const
{
  int len = 0;

  if (fi->isFolder()) {
    len += ji_font_text_len(getFont(), "[+]")+2;
  }

  len += ji_font_text_len(getFont(), fi->getDisplayName().c_str());

  return gfx::Size(2+len+2,
                   2+jwidget_get_text_height(this)+2);
}

void FileList::makeSelectedFileitemVisible()
{
  View* view = View::getView(this);
  gfx::Rect vp = view->getViewportBounds();
  gfx::Point scroll = view->getViewScroll();
  int th = jwidget_get_text_height(this);
  int y = this->rc->y1;

  // rows
  for (FileItemList::iterator
         it=m_list.begin();
       it!=m_list.end(); ++it) {
    IFileItem* fi = *it;
    gfx::Size itemSize = getFileItemSize(fi);

    if (fi == m_selected) {
      if (y < vp.y)
        scroll.y = y - this->rc->y1;
      else if (y > vp.y + vp.h - (2+th+2))
        scroll.y = y - this->rc->y1 - vp.h + (2+th+2);

      view->setViewScroll(scroll);
      break;
    }

    y += itemSize.h;
  }
}

void FileList::regenerateList()
{
  // get the children of the current folder
  m_list = m_currentFolder->getChildren();

  // filter the list by the available extensions
  if (!m_exts.empty()) {
    for (FileItemList::iterator
           it=m_list.begin();
         it!=m_list.end(); ) {
      IFileItem* fileitem = *it;
      if (!fileitem->isFolder() &&
          !fileitem->hasExtension(m_exts.c_str())) {
        it = m_list.erase(it);
      }
      else
        ++it;
    }
  }
}

int FileList::getSelectedIndex()
{
  for (FileItemList::iterator
         it = m_list.begin();
       it != m_list.end(); ++it) {
    if (*it == m_selected)
      return it - m_list.begin();
  }

  return -1;
}

void FileList::selectIndex(int index)
{
  IFileItem* old_selected = m_selected;

  m_selected = m_list.at(index);
  if (old_selected != m_selected) {
    makeSelectedFileitemVisible();

    invalidate();

    // Emit "FileSelected" event.
    onFileSelected();
  }

  generatePreviewOfSelectedItem();
}

// Puts the selected file-item as the next item to be processed by the
// round-robin that generate thumbnails
void FileList::generatePreviewOfSelectedItem()
{
  if (m_selected &&
      !m_selected->isFolder() &&
      !m_selected->getThumbnail())
    {
      m_itemToGenerateThumbnail = m_selected;
      m_timer.start();
    }
}

// Returns true if it does some hard work like access to the disk.
bool FileList::generateThumbnail(IFileItem* fileitem)
{
  if (fileitem->isBrowsable() ||
      fileitem->getThumbnail() != NULL)
    return false;

  FileOp* fop =
    fop_to_load_document(fileitem->getFileName().c_str(),
                         FILE_LOAD_SEQUENCE_NONE |
                         FILE_LOAD_ONE_FRAME);
  if (!fop)
    return true;

  if (fop->has_error()) {
    fop_free(fop);
  }
  else {
    ThumbnailData* data = new ThumbnailData;

    data->fop = fop;
    data->fileitem = fileitem;
    data->fileview = this;
    data->thumbnail = NULL;

    data->thread = new base::thread(&openfile_bg, data);
    if (data->thread) {
      // add a monitor to check the loading (FileOp) progress
      data->monitor = add_gui_monitor(monitor_thumbnail_generation,
                                      monitor_free_thumbnail_generation, data);

      m_monitors.push_back(data->monitor);
      invalidate();
    }
    else {
      fop_free(fop);
      delete data;
    }
  }

  return true;
}

void FileList::stopThreads()
{
  // stop the generation of threads
  m_timer.stop();

  // join all threads (removing all monitors)
  for (MonitorList::iterator
         it = m_monitors.begin();
       it != m_monitors.end(); ) {
    Monitor* monitor = *it;
    ++it;
    remove_gui_monitor(monitor);
  }

  // clear the list of monitors
  m_monitors.clear();
}

// Thread to do the hard work: load the file from the disk (in
// background).
//
// [loading thread]
static void openfile_bg(ThumbnailData* data)
{
  FileOp* fop = (FileOp*)data->fop;

  try {
    fop_operate(fop);
  }
  catch (const std::exception& e) {
    fop_error(fop, "Error loading file:\n%s", e.what());
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
  ThumbnailData* data = (ThumbnailData*)_data;
  FileOp* fop = data->fop;

  /* is done? ...ok, now the thumbnail is in the main thread only... */
  if (fop_is_done(fop)) {
    // Post load
    fop_post_load(fop);

    // Convert the loaded document into the Allegro bitmap "data->thumbnail".
    {
      int thumb_w, thumb_h;
      Image* image;

      Sprite* sprite = (fop->document && fop->document->getSprite()) ? fop->document->getSprite():
                                                                       NULL;
      if (!fop_is_stop(fop) && sprite) {
        // The palette to convert the Image to a BITMAP
        data->palette = new Palette(*sprite->getPalette(0));

        // Render the 'sprite' in one plain 'image'
        image = Image::create(sprite->getPixelFormat(), sprite->getWidth(), sprite->getHeight());
        sprite->render(image, 0, 0);

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
        data->thumbnail = Image::create(image->getPixelFormat(), thumb_w, thumb_h);
        image_clear(data->thumbnail, 0);
        image_scale(data->thumbnail, image, 0, 0, thumb_w, thumb_h);
        image_free(image);
      }

      delete fop->document;
    }

    /* set the thumbnail of the file-item */
    if (data->thumbnail) {
      BITMAP *bmp = create_bitmap_ex(16,
                                     data->thumbnail->w,
                                     data->thumbnail->h);

      image_to_allegro(data->thumbnail, bmp, 0, 0, data->palette);

      delete data->thumbnail;   // image
      delete data->palette;
      data->thumbnail = NULL;
      data->palette = NULL;

      data->fileitem->setThumbnail(bmp);

      /* is the selected file-item the one that now has a thumbnail? */
      if (data->fileview->getSelectedFileItem() == data->fileitem) {
        /* we have to dirty the file-view to show the thumbnail */
        data->fileview->invalidate();
      }
    }

    remove_gui_monitor(data->monitor);
  }
  else {
    data->fileview->invalidate();
  }
}

/**
 * [main thread]
 */
static void monitor_free_thumbnail_generation(void *_data)
{
  ThumbnailData *data = (ThumbnailData*)_data;
  FileOp *fop = data->fop;

  fop_stop(fop);
  data->thread->join();
  delete data->thread;

  // Remove the monitor from the FileList.
  data->fileview->removeMonitor(data->monitor);

  // Destroy the thumbnail
  if (data->thumbnail) {
    image_free(data->thumbnail);
    data->thumbnail = NULL;
  }

  fop_free(fop);
  delete data;
}

void FileList::removeMonitor(Monitor* monitor)
{
  MonitorList::iterator it = std::find(m_monitors.begin(), m_monitors.end(), monitor);
  ASSERT(it != m_monitors.end());
  m_monitors.erase(it);
}

} // namespace widgets
