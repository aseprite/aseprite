// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_RECENT_FILE_H_INCLUDED
#define APP_UI_RECENT_FILE_H_INCLUDED
#pragma once

#include "ui/box.h"

namespace app {

class RecentFile final : public ui::VBox {
public:
  explicit RecentFile(const std::string& path, int thumbnailSize, bool pinned, bool horizontal);

  void reconfigure(bool horizontal, bool pinned);

  void setThumbnailSize(int thumbnailSize);

  int thumbnailSize() const { return m_thumbnailSize; }

  const std::string& path() const { return m_path; }
  const std::string& filename() const { return m_filename; }
  void setThumbnail(const os::SurfaceRef& thumbnail) { m_thumbnail = thumbnail; }

protected:
  void onInitTheme(ui::InitThemeEvent& ev) override;
  bool onProcessMessage(ui::Message* msg) override;
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;

private:
  const text::TextBlobRef& filenameBlob();
  const text::TextBlobRef& pathBlob();

  void click();
  void openFolder();
  void remove();
  void togglePin();

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
