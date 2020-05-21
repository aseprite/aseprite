// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/file_list.h"

#include "app/modules/gfx.h"
#include "app/thumbnail_generator.h"
#include "app/ui/skin/skin_theme.h"
#include "base/clamp.h"
#include "base/string.h"
#include "base/time.h"
#include "os/font.h"
#include "os/surface.h"
#include "ui/ui.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#define ISEARCH_KEYPRESS_INTERVAL_MSECS 500

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;

FileList::FileList()
  : Widget(kGenericWidget)
  , m_currentFolder(FileSystemModule::instance()->getRootFileItem())
  , m_req_valid(false)
  , m_selected(nullptr)
  , m_isearchClock(0)
  , m_generateThumbnailTimer(200, this)
  , m_monitoringTimer(50, this)
  , m_itemToGenerateThumbnail(nullptr)
  , m_multiselect(false)
  , m_zoom(1.0)
  , m_itemsPerRow(0)
{
  setFocusStop(true);
  setDoubleBuffered(true);

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

void FileList::setExtensions(const base::paths& extensions)
{
  m_exts = extensions;

  // Refresh
  if (isVisible())
    setCurrentFolder(m_currentFolder);
}

void FileList::setCurrentFolder(IFileItem* folder)
{
  ASSERT(folder != NULL);
  ASSERT(folder->isBrowsable());

  m_currentFolder = folder;
  m_req_valid = false;
  m_selected = nullptr;

  regenerateList();

  // As now we are in other folder, we can stop the generation of all
  // thumbnails.
  ThumbnailGenerator::instance()->stopAllWorkers();

  // select first folder
  if (!m_list.empty() && m_list.front()->isBrowsable())
    selectIndex(0);

  // Emit "CurrentFolderChanged" event.
  onCurrentFolderChanged();

  invalidate();
  View::getView(this)->updateView();
}

FileItemList FileList::selectedFileItems() const
{
  ASSERT(m_selectedItems.size() == m_list.size());

  FileItemList result;
  for (int i=0; i<int(m_list.size()); ++i) {
    if (m_selectedItems[i])
      result.push_back(m_list[i]);
  }
  return result;
}

void FileList::deselectedFileItems()
{
  if (!m_multiselect)
    return;

  bool redraw = false;

  for (int i=0; i<int(m_list.size()); ++i) {
    if (m_selected == m_list[i]) {
      if (!m_selectedItems[i]) {
        m_selectedItems[i] = true;
        redraw = true;
      }
    }
    else if (m_selectedItems[i]) {
      m_selectedItems[i] = false;
      redraw = true;
    }
  }

  if (redraw)
    invalidate();
}

void FileList::setMultipleSelection(bool multiple)
{
  m_multiselect = multiple;
}

void FileList::goUp()
{
  IFileItem* folder = m_currentFolder;
  IFileItem* parent = folder->parent();
  if (parent) {
    setCurrentFolder(parent);

    m_selected = folder;
    deselectedFileItems();

    // Make the selected item visible.
    makeSelectedFileitemVisible();
  }
}

void FileList::setZoom(const double zoom)
{
  m_zoom = base::clamp(zoom, 1.0, 8.0);
  m_req_valid = false;

  // if (auto view = View::getView(this))
  recalcAllFileItemInfo();
}

void FileList::animateToZoom(const double zoom)
{
  m_fromZoom = m_zoom;
  m_toZoom = zoom;
  startAnimation(ANI_ZOOM, 10);
}

bool FileList::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseDownMessage:
      captureMouse();

    case kMouseMoveMessage:
      if (hasCapture()) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        IFileItem* oldSelected = m_selected;
        m_selected = nullptr;

        // Mouse position in client coordinates
        gfx::Point mousePos = mouseMsg->position() - bounds().origin();

        // rows
        int i = 0;
        for (auto begin=m_list.begin(), end=m_list.end(), it=begin;
             it!=end; ++it, ++i) {
          IFileItem* fi = *it;
          ItemInfo info = getFileItemInfo(i);

          if ((info.bounds.contains(mousePos)) ||
              (isListView() &&
               ((it == begin && mousePos.y < info.bounds.y) ||
                (it == end-1 && mousePos.y >= info.bounds.y2())))) {
            m_selected = fi;

            if (m_multiselect &&
                oldSelected != fi &&
                m_selectedItems.size() == m_list.size()) {
              if (msg->shiftPressed() ||
                  msg->ctrlPressed() ||
                  msg->cmdPressed()) {
                m_selectedItems[i] = !m_selectedItems[i];
              }
              else {
                std::fill(m_selectedItems.begin(),
                          m_selectedItems.end(), false);
                m_selectedItems[i] = true;
              }
              invalidate();
            }

            makeSelectedFileitemVisible();
            break;
          }
        }

        if (oldSelected != m_selected) {
          if (hasThumbnailsPerItem())
            generateThumbnailForFileItem(m_selected);
          else
            delayThumbnailGenerationForSelectedItem();

          invalidate();

          // Emit "FileSelected" event.
          onFileSelected();
        }
      }
      break;

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();
      }
      break;

    case kKeyDownMessage:
      if (hasFocus()) {
        KeyMessage* keyMsg = static_cast<KeyMessage*>(msg);
        KeyScancode scancode = keyMsg->scancode();
        int unicodeChar = keyMsg->unicodeChar();
        int select = selectedIndex();
        View* view = View::getView(this);
        int bottom = m_list.size();

        switch (scancode) {

          case kKeyUp:
            if (select >= 0)
              select -= m_itemsPerRow;
            else
              select = 0;
            break;

          case kKeyDown:
            if (select >= 0)
              select += m_itemsPerRow;
            else
              select = 0;
            break;

          case kKeyHome:
            select = 0;
            break;

          case kKeyEnd:
            select = bottom-1;
            break;

          case kKeyPageUp:
          case kKeyPageDown: {
            int sgn = (scancode == kKeyPageUp) ? -1: 1;
            gfx::Rect vp = view->viewportBounds();
            if (select < 0)
              select = 0;
            select += sgn * vp.h / (textHeight()+4*guiscale());
            break;
          }

          case kKeyLeft:
          case kKeyRight: {
            const int delta = (scancode == kKeyLeft ? -1: 1);
            if (isIconView()) {
              if (select >= 0)
                select += delta;
              else
                select = 0;
            }
            else if (select >= 0) {
              gfx::Rect vp = view->viewportBounds();
              gfx::Point scroll = view->viewScroll();
              scroll.x += vp.w/2*delta;
              view->setViewScroll(scroll);
            }
            break;
          }

          case kKeyEnter:
          case kKeyEnterPad:
            if (m_selected) {
              if (m_selected->isBrowsable()) {
                setCurrentFolder(m_selected);
                return true;
              }
              if (m_selected->isFolder()) {
                // Do nothing (is a folder but not browseable).
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

          case kKeyBackspace:
            goUp();
            return true;

          default:
            if (unicodeChar == ' ' ||
                (std::tolower(unicodeChar) >= 'a' &&
                 std::tolower(unicodeChar) <= 'z') ||
                (unicodeChar >= '0' &&
                 unicodeChar <= '9')) {
              if ((base::current_tick() - m_isearchClock) > ISEARCH_KEYPRESS_INTERVAL_MSECS)
                m_isearch.clear();

              m_isearch.push_back(unicodeChar);

              int i, chrs = m_isearch.size();
              FileItemList::iterator
                link = m_list.begin() + ((select >= 0) ? select: 0);

              for (i=std::max(select, 0); i<bottom; ++i, ++link) {
                IFileItem* fi = *link;
                if (base::utf8_icmp(fi->displayName(), m_isearch, chrs) == 0) {
                  select = i;
                  break;
                }
              }
              m_isearchClock = base::current_tick();
              // Go to selectIndex...
            }
            else
              return Widget::onProcessMessage(msg);
        }

        if (bottom > 0)
          selectIndex(base::clamp(select, 0, bottom-1));

        return true;
      }
      break;

    case kMouseWheelMessage: {
      View* view = View::getView(this);
      if (view) {
        // Zoom
        if (msg->ctrlPressed() || // TODO configurable
            msg->cmdPressed()) {
          gfx::Point delta = static_cast<MouseMessage*>(msg)->wheelDelta();
          const bool precise = static_cast<MouseMessage*>(msg)->preciseWheel();
          double dz = delta.x + delta.y;

          if (precise) {
            dz /= 1.5;
            if (dz < -1.0) dz = -1.0;
            else if (dz > 1.0) dz = 1.0;
          }

          setZoom(zoom() - dz);
          break;
        }
        else {
          gfx::Point scroll = view->viewScroll();

          if (static_cast<MouseMessage*>(msg)->preciseWheel())
            scroll += static_cast<MouseMessage*>(msg)->wheelDelta();
          else
            scroll += static_cast<MouseMessage*>(msg)->wheelDelta() * 3*(textHeight()+4*guiscale());

          view->setViewScroll(scroll);
        }
      }
      break;
    }

    case kDoubleClickMessage:
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

    case kTouchMagnifyMessage: {
      setZoom(zoom() + 4.0*static_cast<ui::TouchMessage*>(msg)->magnification());
      break;
    }

  }

  return Widget::onProcessMessage(msg);
}

void FileList::onPaint(ui::PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  gfx::Rect bounds = clientBounds();

  g->fillRect(theme->colors.background(), bounds);
  // g->fillRect(bgcolor, gfx::Rect(bounds.x, y, bounds.w, itemSize.h));

  int i = 0, selectedIndex = -1;
  for (IFileItem* fi : m_list) {
    if (m_selected != fi) {
      paintItem(g, fi, i);
    }
    else {
      selectedIndex = i;
    }
    ++i;
  }

  // Paint main selected index (so if the filename label is bigger it
  // will appear over other items).
  if (m_selected)
    paintItem(g, m_selected, selectedIndex);

  // Draw main thumbnail for the selected item when there are no
  // thumbnails per item.
  if (isListView() &&
      m_selected &&
      m_selected->getThumbnail()) {
    gfx::Rect tbounds = mainThumbnailBounds();
    tbounds.enlarge(1);
    g->drawRect(gfx::rgba(0, 0, 0, 64), tbounds);
    tbounds.shrink(1);

    os::Surface* thumbnail = m_selected->getThumbnail();
    g->drawRgbaSurface(thumbnail,
                       gfx::Rect(0, 0, thumbnail->width(), thumbnail->height()),
                       tbounds);
  }
}

void FileList::paintItem(ui::Graphics* g, IFileItem* fi, const int i)
{
  ItemInfo info = getFileItemInfo(i);
  if ((g->getClipBounds() & info.bounds).isEmpty())
    return;

  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  const bool evenRow = ((i & 1) == 0);
  gfx::Rect tbounds = info.thumbnail;

  gfx::Color bgcolor;
  gfx::Color fgcolor;
  if ((!m_multiselect && fi == m_selected) ||
      (m_multiselect &&
       m_selectedItems.size() == m_list.size() &&
       m_selectedItems[i])) {
    fgcolor = theme->colors.filelistSelectedRowText();
    bgcolor = theme->colors.filelistSelectedRowFace();
  }
  else {
    bgcolor = evenRow ? theme->colors.filelistEvenRowFace():
                        theme->colors.filelistOddRowFace();

    if (fi->isFolder() && !fi->isBrowsable())
      fgcolor = theme->colors.filelistDisabledRowText();
    else
      fgcolor = evenRow ? theme->colors.filelistEvenRowText():
                          theme->colors.filelistOddRowText();
  }

  // Item background
  g->fillRect(bgcolor, info.bounds);

  gfx::Rect textBounds = info.text;

  // Folder icon or thumbnail
  os::Surface* thumbnail = nullptr;
  if (fi->isFolder()) {
    if (isListView()) {
      thumbnail = theme->parts.folderIconSmall()->bitmap(0);
      tbounds = textBounds;
      tbounds.x += 2*guiscale();
      tbounds.w = tbounds.h;
      textBounds.x += tbounds.x2();
    }
    else {
      thumbnail =
        (m_zoom < 4.0 ?
         theme->parts.folderIconMedium()->bitmap(0):
         theme->parts.folderIconBig()->bitmap(0));
    }
  }
  else {
    thumbnail = fi->getThumbnail();
  }

  // item name
  if (isIconView() && textBounds.w > info.bounds.w) {
    g->drawAlignedUIText(
      fi->displayName().c_str(),
      fgcolor, bgcolor,
      (textBounds & gfx::Rect(info.bounds).shrink(2*guiscale())),
      ui::CENTER | ui::TOP | ui::CHARWRAP);
  }
  else {
    g->drawText(
      fi->displayName().c_str(),
      fgcolor, bgcolor,
      gfx::Point(textBounds.x+2*guiscale(),
                 textBounds.y+2*guiscale()));
  }

  // Draw thumbnail progress bar
  double progress = fi->getThumbnailProgress();
  if (isIconView() && !thumbnail) {
    if (progress == 0.0)
      generateThumbnailForFileItem(fi);
  }

  if (!tbounds.isEmpty()) {
    if (thumbnail) {
      tbounds =
        gfx::Rect(0, 0,  thumbnail->width(), thumbnail->height())
        .fitIn(tbounds);

      if (!fi->isFolder()) {
        g->drawRect(gfx::rgba(0, 0, 0, 64), tbounds);
        tbounds.shrink(1);
      }

      g->drawRgbaSurface(thumbnail,
                         gfx::Rect(0, 0, thumbnail->width(), thumbnail->height()),
                         tbounds);
    }
    else {
      tbounds = gfx::Rect(0, 0, 20*guiscale(), 2+4*(8.0-m_zoom)/8.0*guiscale())
        .fitIn(tbounds);

      // Start thumbnail generation for this item
      generateThumbnailForFileItem(fi);
      theme->paintProgressBar(g, tbounds, progress);
    }
  }
}

gfx::Rect FileList::mainThumbnailBounds()
{
  gfx::Rect result;

  if (!m_selected)
    return result;

  os::Surface* thumbnail = m_selected->getThumbnail();
  if (!thumbnail)
    return result;

  ItemInfo info = getFileItemInfo(selectedIndex());
  if (!info.thumbnail.isEmpty()) // There is thumbnail per item
    return result;

  View* view = View::getView(this);
  gfx::Rect vp = view->viewportBounds();
  int x = vp.x+vp.w - 2*guiscale() - thumbnail->width();
  int y = info.bounds.center().y - thumbnail->height()/2 + bounds().y;
  y = base::clamp(y, vp.y+2*guiscale(), vp.y+vp.h-3*guiscale()-thumbnail->height());
  x -= bounds().x;
  y -= bounds().y;
  return gfx::Rect(x, y, thumbnail->width(), thumbnail->height());
}

void FileList::onSizeHint(SizeHintEvent& ev)
{
  if (!m_req_valid) {
    gfx::Rect req;

    // rows
    for (int i=0; i<int(m_list.size()); ++i) {
      ItemInfo info = getFileItemInfo(i);
      req |= info.bounds;
    }

    m_req_valid = true;
    m_req_w = req.w;
    m_req_h = req.h;
  }
  ev.setSizeHint(Size(m_req_w, m_req_h));
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
  auto start = base::current_tick();
  while (!m_generateThumbnailsForTheseItems.empty() &&
         // No more than 200ms launching thumbnail generators
         base::current_tick() - start < 200) {
    auto fi = m_generateThumbnailsForTheseItems.front();
    m_generateThumbnailsForTheseItems.pop_front();
    ThumbnailGenerator::instance()->generateThumbnail(fi);
  }

  if (ThumbnailGenerator::instance()->checkWorkers())
    invalidate();
}

void FileList::onGenerateThumbnailTick()
{
  m_generateThumbnailTimer.stop();

  auto fi = m_itemToGenerateThumbnail;
  if (fi)
    generateThumbnailForFileItem(fi);
}

void FileList::recalcAllFileItemInfo()
{
  View* view = View::getView(this);
  if (view)
    view->setScrollableSize(gfx::Size(1, view->bounds().h), false);

  m_info.resize(m_list.size());
  if (m_info.empty()) {
    m_itemsPerRow = 0;
    return;
  }

  for (int i=0; i<int(m_info.size()); ++i)
    m_info[i] = calcFileItemInfo(i);

  m_itemsPerRow = 1;

  // Add the vertical scrollbar space
  if (isListView()) {
    if (view) {
      view->updateView();
      if (!view->verticalBar()->isVisible()) {
        gfx::Rect vp = view->viewportBounds();
        for (auto& info : m_info) {
          info.bounds.w = vp.w;
        }
      }
    }
  }
  // Redistribute items in X axis
  else if (isIconView()) {
    int maxWidth = 0;
    int maxTextWidth = 0;
    for (const auto& info : m_info) {
      int w = std::min(info.bounds.w, info.thumbnail.w*2);
      maxWidth = std::max(maxWidth, w);
      maxTextWidth = std::max(maxTextWidth, info.text.w);
    }
    if (maxWidth == 0)
      return;

    gfx::Size vp = (view ? view->viewportBounds().size(): size());

    int itemsPerRow = vp.w / maxWidth;
    if (itemsPerRow < 3) itemsPerRow = 3;
    int itemWidth = vp.w / itemsPerRow;

    int i = 0;
    for (int y=0; i<int(m_info.size()); ) {
      int h = 0;
      int j = 0;
      int x = 0;
      for (; j<itemsPerRow && i<int(m_info.size()); ++j, ++i, x += itemWidth) {
        auto& info = m_info[i];
        int deltax = x - info.bounds.x;
        int deltay = y - info.bounds.y;
        info.bounds.x += deltax;
        info.bounds.y += deltay;
        info.bounds.w = itemWidth;

        if (maxTextWidth > itemWidth) {
          info.bounds.h += info.text.h;
          info.text.h += info.text.h;
        }

        info.text.x = info.bounds.x + info.bounds.w/2 - info.text.w/2;
        info.text.y += deltay;
        info.thumbnail.x = info.bounds.x + info.bounds.w/2 - info.thumbnail.w/2;
        info.thumbnail.y += deltay;
        h = info.bounds.h;
      }
      if (h)
        y += h;
      else
        break;
    }

    m_itemsPerRow = itemsPerRow;
  }

  invalidate();
  if (view) {
    view->updateView();
    makeSelectedFileitemVisible();
  }
}

FileList::ItemInfo FileList::calcFileItemInfo(int i) const
{
  ASSERT(i >= 0 && i < int(m_list.size()));

  const bool withThumbnails = hasThumbnailsPerItem();
  IFileItem* fi = m_list[i];
  int len = 0;

  if (fi->isFolder() && isListView()) {
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    len += theme->parts.folderIconSmall()->bitmap(0)->width() + 2*guiscale();
  }

  len += font()->textLength(fi->displayName().c_str());

  int textHeight = this->textHeight() + 4*guiscale();
  int rowHeight = textHeight + (withThumbnails ? 8*m_zoom+2*guiscale(): 0);

  ItemInfo info;
  info.text = gfx::Rect(0, 0+rowHeight*i, len+4*guiscale(), textHeight);
  if (withThumbnails) {
    info.thumbnail = gfx::Rect(0, info.text.y,
                               8*m_zoom*guiscale(),
                               8*m_zoom*guiscale());
    info.text.y += info.thumbnail.h + 2*guiscale();
  }

  info.bounds = info.text | info.thumbnail;
  if (withThumbnails) {
    info.text.x = info.bounds.x + info.bounds.w/2 - info.text.w/2;
    info.thumbnail.x = info.bounds.x + info.bounds.w/2 - info.thumbnail.w/2;
  }
  else {
    info.bounds.x = 0;
    if (View* view = View::getView(this))
      info.bounds.w = view->viewportBounds().w;
    else
      info.bounds.w = bounds().w;
  }
  return info;
}

void FileList::makeSelectedFileitemVisible()
{
  int i = selectedIndex();
  if (i < 0)
    return;

  View* view = View::getView(this);
  gfx::Rect vp = view->viewportBounds();
  gfx::Point scroll = view->viewScroll();
  ItemInfo info = getFileItemInfo(i);

  if (info.bounds.x+bounds().x <= vp.x)
    scroll.x = info.bounds.x;
  else if (info.bounds.x+bounds().x > vp.x2() - info.bounds.w)
    scroll.x = info.bounds.x - vp.w + info.bounds.w;

  if (info.bounds.y+bounds().y < vp.y)
    scroll.y = info.bounds.y;
  else if (info.bounds.y+bounds().y > vp.y2() - info.bounds.h)
    scroll.y = info.bounds.y - vp.h + info.bounds.h;

  view->setViewScroll(scroll);
}

void FileList::regenerateList()
{
  // get the children of the current folder
  m_list = m_currentFolder->children();

  // filter the list by the available extensions
  if (!m_exts.empty()) {
    for (FileItemList::iterator
           it=m_list.begin();
         it!=m_list.end(); ) {
      IFileItem* fileitem = *it;

      if (fileitem->isHidden())
        it = m_list.erase(it);
      else if (!fileitem->isFolder() &&
               !fileitem->hasExtension(m_exts)) {
        it = m_list.erase(it);
      }
      else
        ++it;
    }
  }

  recalcAllFileItemInfo();

  if (m_multiselect && !m_list.empty()) {
    m_selectedItems.resize(m_list.size());
    deselectedFileItems();
  }
  else
    m_selectedItems.clear();
}

int FileList::selectedIndex() const
{
  for (auto it = m_list.begin(), end = m_list.end();
       it != end; ++it) {
    if (*it == m_selected)
      return it - m_list.begin();
  }
  return -1;
}

void FileList::selectIndex(int index)
{
  IFileItem* oldSelected = m_selected;

  m_selected = m_list.at(index);
  deselectedFileItems();

  if (oldSelected != m_selected) {
    makeSelectedFileitemVisible();

    invalidate();

    // Emit "FileSelected" event.
    onFileSelected();
  }

  delayThumbnailGenerationForSelectedItem();
}

void FileList::generateThumbnailForFileItem(IFileItem* fi)
{
  if (fi && animation() == ANI_NONE) {
    auto it = std::find(m_generateThumbnailsForTheseItems.begin(),
                        m_generateThumbnailsForTheseItems.end(), fi);
    if (it != m_generateThumbnailsForTheseItems.end())
      m_generateThumbnailsForTheseItems.erase(it);
    m_generateThumbnailsForTheseItems.push_front(fi);
  }
}

void FileList::delayThumbnailGenerationForSelectedItem()
{
  if (m_selected &&
      !m_selected->isFolder() &&
      !m_selected->getThumbnail()) {
    m_itemToGenerateThumbnail = m_selected;
    m_generateThumbnailTimer.start();
  }
}

void FileList::onAnimationStop(int animation)
{
  if (animation == ANI_ZOOM)
    setZoom(m_toZoom);
}

void FileList::onAnimationFrame()
{
  if (animation() == ANI_ZOOM)
    setZoom(inbetween(m_fromZoom, m_toZoom, animationTime()));
}

} // namespace app
