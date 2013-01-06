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

#include "modules/gfx.h"
#include "skin/skin_theme.h"
#include "thumbnail_generator.h"
#include "ui/gui.h"

#include <algorithm>
#include <allegro.h>

#define ISEARCH_KEYPRESS_INTERVAL_MSECS 500

using namespace gfx;
using namespace ui;

namespace widgets {

FileList::FileList()
  : Widget(JI_WIDGET)
  , m_generateThumbnailTimer(200, this)
  , m_monitoringTimer(50, this)
{
  setFocusStop(true);

  m_currentFolder = FileSystemModule::instance()->getRootFileItem();
  m_req_valid = false;
  m_selected = NULL;
  m_isearchClock = 0;

  m_itemToGenerateThumbnail = NULL;

  m_generateThumbnailTimer.Tick.connect(&FileList::onGenerateThumbnailTick, this);
  m_monitoringTimer.Tick.connect(&FileList::onMonitoringTick, this);
  m_monitoringTimer.start();

  regenerateList();
}

FileList::~FileList()
{
  // Stop timers.
  m_generateThumbnailTimer.stop();
  m_monitoringTimer.stop();

  // Stop workers creating thumbnails.
  ThumbnailGenerator::instance()->stopAllWorkers();
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

    case JM_DRAW: {
      SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
      View* view = View::getView(this);
      gfx::Rect vp = view->getViewportBounds();
      int th = jwidget_get_text_height(this);
      int x, y = this->rc->y1;
      int evenRow = 0;
      ui::Color bgcolor;
      ui::Color fgcolor;
      BITMAP *thumbnail = NULL;
      int thumbnail_y = 0;

      // rows
      for (FileItemList::iterator
             it=m_list.begin(), end=m_list.end(); it!=end; ++it) {
        IFileItem* fi = *it;
        gfx::Size itemSize = getFileItemSize(fi);

        if (fi == m_selected) {
          fgcolor = theme->getColor(ThemeColor::FileListSelectedRowText);
          bgcolor = theme->getColor(ThemeColor::FileListSelectedRowFace);
        }
        else {
          bgcolor = evenRow ? theme->getColor(ThemeColor::FileListEvenRowFace):
                              theme->getColor(ThemeColor::FileListOddRowFace);

          if (fi->isFolder() && !fi->isBrowsable())
            fgcolor = theme->getColor(ThemeColor::FileListDisabledRowText);
          else
            fgcolor = evenRow ? theme->getColor(ThemeColor::FileListEvenRowText):
                                theme->getColor(ThemeColor::FileListOddRowText);
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
                   to_system(bgcolor));
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

        // draw progress bars
        double progress;
        ThumbnailGenerator::WorkerStatus workerStatus =
          ThumbnailGenerator::instance()->getWorkerStatus(fi, progress);
        if (workerStatus == ThumbnailGenerator::WorkingOnThumbnail) {
          theme->drawProgressBar(ji_screen,
                                 this->rc->x2-2-64, y+itemSize.h/2-3,
                                 this->rc->x2-2, y+itemSize.h/2+3,
                                 progress);
        }

        // Thumbnail position
        if (fi == m_selected) {
          thumbnail = fi->getThumbnail();
          if (thumbnail)
            thumbnail_y = y + itemSize.h/2;
        }

        y += itemSize.h;
        evenRow ^= 1;
      }

      if (y < this->rc->y2-1)
        rectfill(ji_screen,
                 this->rc->x1, y,
                 this->rc->x2-1, this->rc->y2-1,
                 to_system(theme->getColor(ThemeColor::Background)));

      // Draw the thumbnail
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
        draw_emptyset_symbol(ji_screen, vp, ui::rgba(194, 194, 194));
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

  }

  return Widget::onProcessMessage(msg);
}

void FileList::onPreferredSize(PreferredSizeEvent& ev)
{
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
  ev.setPreferredSize(Size(m_req_w, m_req_h));
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

void FileList::onMonitoringTick()
{
  if (ThumbnailGenerator::instance()->checkWorkers())
    invalidate();
}

void FileList::onGenerateThumbnailTick()
{
  m_generateThumbnailTimer.stop();

  IFileItem* fileitem = m_itemToGenerateThumbnail;
  if (fileitem)
    ThumbnailGenerator::instance()->addWorkerToGenerateThumbnail(fileitem);
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
      m_generateThumbnailTimer.start();
    }
}

} // namespace widgets
