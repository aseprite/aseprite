// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_RECENT_FILE_H_INCLUDED
#define APP_UI_RECENT_FILE_H_INCLUDED
#pragma once

#include "app/app.h"
#include "app/i18n/strings.h"
#include "app/modules/gfx.h"
#include "app/pref/preferences.h"
#include "base/fs.h"
#include "base/launcher.h"
#include "skin/skin_theme.h"
#include "ui/box.h"
#include "ui/manager.h"
#include "ui/menu.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/utf8_range_builder.h"

namespace app {

using namespace ui;

inline std::string pretty_path(const std::string& path)
{
  std::string result = base::normalize_path(path);
#if LAF_WINDOWS
  return result;
#else
  const char* env = std::getenv("HOME");
  if (env && (*env) && result.rfind(env, 0) == 0) {
    result = result.substr(strlen(env));
    result.insert(result.begin(), '~');
  }
  return result;
#endif
}

class RecentFile final : public VBox {
public:
  explicit RecentFile(const std::string& path, const int thumbnailSize, bool pinned, bool horizontal)
    : m_path(path)
    , m_thumbnailSize(thumbnailSize)
    , m_isPinned(pinned)
    , m_isHorizontal(horizontal)
    , m_showPinButton(false)
  {
    setFocusStop(true);
    disableFlags(IGNORE_MOUSE);

    m_filename = base::get_file_name(path);
    m_visiblePath = pretty_path(m_path);
  }

  void reconfigure(const bool horizontal, const bool pinned)
  {
    m_isHorizontal = horizontal;
    m_isPinned = pinned;
    invalidate();
  }

  void setThumbnailSize(const int thumbnailSize)
  {
    if (m_thumbnailSize == thumbnailSize)
      return;

    m_thumbnailSize = thumbnailSize;
    invalidate();
  }

  int thumbnailSize() const { return m_thumbnailSize; }

  const std::string& path() const { return m_path; }
  const std::string& filename() const { return m_filename; }
  void setThumbnail(const os::SurfaceRef& thumbnail) { m_thumbnail = thumbnail; }

protected:
  void onInitTheme(InitThemeEvent& ev) override
  {
    m_filenameBlob.reset();
    m_pathBlob.reset();
  }

  bool onProcessMessage(Message* msg) override
  {
    switch (msg->type()) {
      case kFocusEnterMessage:
      case kFocusLeaveMessage: invalidate(); break;
      case kKeyDownMessage:    {
        const auto* keyMsg = static_cast<KeyMessage*>(msg);
        if (keyMsg->scancode() == kKeyEnter) {
          click();
          return true;
        }
        if (keyMsg->altPressed()) {
          switch (keyMsg->scancode()) {
            case kKeyEnter: {
              click();
              return true;
            }
            case kKeyF: {
              openFolder();
              return true;
            }
            case kKeyP: {
              togglePin();
              return true;
            }
            case kKeyDel:
            case kKeyR:   {
              remove();
              return true;
            }
            default: break;
          }
        }
        break;
      }
      case kSetCursorMessage:  set_mouse_cursor(kHandCursor); return true;
      case kMouseLeaveMessage: {
        releaseMouse();
        m_showPinButton = false;
        invalidate();
        break;
      }
      case kMouseMoveMessage: {
        const auto* mouseMsg = dynamic_cast<MouseMessage*>(msg);
        const gfx::Rect thumbRect(bounds().origin(), gfx::Size(m_thumbnailSize, m_thumbnailSize));
        m_showPinButton = thumbRect.contains(mouseMsg->position());
        invalidate();
        break;
      }
      case kMouseDownMessage: {
        captureMouse();
        break;
      }
      case kMouseUpMessage: {
        if (!hasCapture())
          break;

        releaseMouse();

        const auto* mouseMsg = dynamic_cast<MouseMessage*>(msg);
        if (!bounds().contains(mouseMsg->position()))
          break;

        if (mouseMsg->left() && mouseMsg->shiftPressed()) {
          manager()->setFocus(this);
        }
        else if (mouseMsg->right()) {
          Menu menu;
          MenuItem open(Strings::home_view_recent_open_file());
          MenuItem openFolder(Strings::home_view_recent_open_folder());
          MenuItem togglePin(m_isPinned ? Strings::home_view_recent_unpin() :
                                          Strings::home_view_recent_pin());
          MenuItem remove(Strings::home_view_recent_remove());

          menu.addChild(&open);
          menu.addChild(&openFolder);
          menu.addChild(new MenuSeparator);
          menu.addChild(&togglePin);
          menu.addChild(&remove);

          open.Click.connect(&RecentFile::click, this);
          openFolder.Click.connect(&RecentFile::openFolder, this);
          remove.Click.connect(&RecentFile::remove, this);
          togglePin.Click.connect(&RecentFile::togglePin, this);

          for (auto* item : menu.children())
            item->processMnemonicFromText();

          menu.showPopup(mousePosInDisplay(), display());
          return true;
        }
        else {
          if (m_showPinButton) {
            if (const auto* pinBitmap = skin::SkinTheme::get(this)->parts.pinned()->bitmap(0)) {
              const auto buttonSize = pinBitmap->width() * 2;
              const gfx::Rect pinButtonArea(
                bounds().origin() + gfx::Point(m_thumbnailSize - buttonSize, 0),
                gfx::Size(buttonSize, buttonSize));
              if (pinButtonArea.contains(mouseMsg->position())) {
                togglePin();
                return true;
              }
            }
          }
          click();
        }
        break;
      }
      default: break;
    }
    return VBox::onProcessMessage(msg);
  }

  void onSizeHint(SizeHintEvent& ev) override
  {
    gfx::Size textSize;
    if (Preferences::instance().general.showFullPath()) {
      textSize = gfx::Size(std::max<int>(pathBlob()->bounds().w, filenameBlob()->bounds().w),
                           pathBlob()->bounds().h + (2 * guiscale()) + filenameBlob()->bounds().h);
    }
    else {
      textSize = gfx::Size(pathBlob()->bounds().w, pathBlob()->bounds().h + (2 * guiscale()));
    }

    if (m_isHorizontal) {
      ev.setSizeHint(gfx::Size(m_thumbnailSize + (4 * guiscale()) + textSize.w,
                               std::max(m_thumbnailSize, textSize.h)));
    }
    else {
      ev.setSizeHint(
        gfx::Size(m_thumbnailSize, 1 + m_thumbnailSize + textSize.h + (2 * guiscale())));
    }
  }

  void onPaint(PaintEvent& ev) override
  {
    Graphics* g = ev.graphics();

    const auto* theme = skin::SkinTheme::get(this);
    const bool underline = hasFlags(HAS_MOUSE) || hasFocus();
    const bool path = Preferences::instance().general.showFullPath();

    Paint paint;
    paint.color(bgColor());
    g->drawRect(gfx::Rect(gfx::Point(0, 0), size()), paint);

    paint.style(Paint::Stroke);
    paint.color(theme->colors.workspaceText());

    const gfx::Point point(clientChildrenBounds().origin());
    const gfx::Rect thumb(point, gfx::Size(m_thumbnailSize, m_thumbnailSize));
    {
      const IntersectClip thumbClip(g, thumb);
      if (m_thumbnail && thumbClip) {
        Paint thumbPaint;
        thumbPaint.blendMode(os::BlendMode::SrcOver);

        // Draw the grid only if it's going to show up
        if (bgColor() != theme->colors.workspace())
          draw_checkered_grid(g, thumb, gfx::Size(16, 16), bgColor(), theme->colors.workspace());

        os::Sampling sampling;
        gfx::Rect thumbRect;
        if (m_thumbnail->width() > m_thumbnailSize || m_thumbnail->height() > m_thumbnailSize) {
          sampling = os::Sampling(os::Sampling::Filter::Linear, os::Sampling::Mipmap::Nearest);

          const double widthRatio = m_thumbnailSize / static_cast<double>(m_thumbnail->width());
          const double heightRatio = m_thumbnailSize / static_cast<double>(m_thumbnail->height());
          int thumbWidth, thumbHeight;
          if (widthRatio < heightRatio) {
            thumbWidth = m_thumbnail->width() * widthRatio;
            thumbHeight = m_thumbnail->height() * widthRatio;
          }
          else {
            thumbWidth = m_thumbnail->width() * heightRatio;
            thumbHeight = m_thumbnail->height() * heightRatio;
          }

          thumbRect = gfx::Rect(guiscaled_center(point.x, m_thumbnailSize, thumbWidth),
                                guiscaled_center(point.y, m_thumbnailSize, thumbHeight),
                                thumbWidth,
                                thumbHeight);
        }
        else {
          thumbRect = gfx::Rect(guiscaled_center(point.x, m_thumbnailSize, m_thumbnail->width()),
                                guiscaled_center(point.y, m_thumbnailSize, m_thumbnail->height()),
                                m_thumbnail->width(),
                                m_thumbnail->height());
        }

        g->drawSurface(m_thumbnail.get(),
                       gfx::Rect(0, 0, m_thumbnail->width(), m_thumbnail->height()),
                       thumbRect,
                       sampling,
                       &thumbPaint);
      }
      else {
        g->drawRect(theme->colors.workspace(), thumb);
        g->drawLine(paint.color(), point, point + gfx::Point(m_thumbnailSize, m_thumbnailSize));
      }

      g->drawRect(thumb, paint);
    }

    paint.style(Paint::Fill);
    paint.color(theme->colors.workspaceText());

    const int lineSeparation = (m_isHorizontal ? 4 : 2) * guiscale();

    gfx::PointF textPoint;
    if (m_isHorizontal && path) {
      textPoint = gfx::PointF(
        thumb.x2() + lineSeparation,
        point.y + (thumb.h / 2.0) -
          ((filenameBlob()->bounds().h + lineSeparation + pathBlob()->bounds().h) / 2.0));
    }
    else if (m_isHorizontal) {
      textPoint = gfx::PointF(thumb.x2() + lineSeparation,
                              point.y + (thumb.h / 2.0) - (filenameBlob()->bounds().h / 2.0));
    }
    else {
      textPoint = gfx::PointF(thumb.x, thumb.y2() + lineSeparation);
    }

    g->drawTextBlob(filenameBlob(), textPoint, paint);

    if (m_showPinButton || m_isPinned) {
      const auto part = m_isPinned ? theme->parts.pinned() : theme->parts.unpinned();
      if (auto* bitmap = part->bitmap(0)) {
        const gfx::Point pinPos(thumb.x2() - bitmap->width() - lineSeparation,
                                thumb.y + lineSeparation);
        paint.style(Paint::StrokeAndFill);
        paint.color(theme->colors.workspaceText());
        g->drawColoredRgbaSurface(bitmap, paint.color(), pinPos.x, pinPos.y);
      }
    }

    paint.color(theme->colors.workspaceText());

    if (underline)
      g->drawLine(textPoint + gfx::PointF(0, m_filenameBlob->bounds().h),
                  textPoint + gfx::PointF(m_filenameBlob->bounds().w, m_filenameBlob->bounds().h),
                  paint);

    if (path) {
      textPoint += gfx::Point(0, m_filenameBlob->bounds().h + lineSeparation);
      g->drawTextBlob(pathBlob(), textPoint, paint);
      if (underline)
        g->drawLine(textPoint + gfx::PointF(0, m_pathBlob->bounds().h),
                    textPoint + gfx::PointF(m_pathBlob->bounds().w, m_pathBlob->bounds().h),
                    paint);
    }
  }

private:
  const text::TextBlobRef& filenameBlob()
  {
    if (!m_filenameBlob) {
      const auto* theme = skin::SkinTheme::get(this);
      m_filenameBlob = text::TextBlob::MakeWithShaper(theme->fontMgr(), font(), m_filename);
    }
    return m_filenameBlob;
  }

  const text::TextBlobRef& pathBlob()
  {
    if (!m_pathBlob) {
      const auto* theme = skin::SkinTheme::get(this);
      m_pathBlob =
        text::TextBlob::MakeWithShaper(theme->fontMgr(), theme->getMiniFont(), m_visiblePath);
    }
    return m_pathBlob;
  }

  void click() { RecentGrid::onClick(m_path); }

  void openFolder() { base::launcher::open_folder(base::get_file_path(m_path)); }

  void remove()
  {
    auto* recents = App::instance()->recentFiles();
    if (m_isPinned) {
      recents->removePinnedFile(m_path);
    }
    else {
      recents->removeRecentFile(m_path);
    }
  }

  void togglePin()
  {
    auto* recents = App::instance()->recentFiles();
    if (m_isPinned) {
      recents->removePinnedFile(m_path);
    }
    else {
      recents->addPinnedFile(m_path);
    }
  }

  std::string m_filename;
  std::string m_path;
  std::string m_visiblePath;

  text::TextBlobRef m_filenameBlob;
  text::TextBlobRef m_pathBlob;

  int m_thumbnailSize;
  bool m_isPinned;
  bool m_isHorizontal;
  bool m_showPinButton;
  os::SurfaceRef m_thumbnail;
};
} // namespace app

#endif
