// Aseprite
// Copyright (C) 2026-present  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_RECENT_GRID_H_INCLUDED
#define APP_UI_RECENT_GRID_H_INCLUDED
#pragma once

#include "app/ui/inline_search.h"
#include "obs/connection.h"
#include "ui/animated_widget.h"
#include "ui/grid.h"
#include "ui/timer.h"

namespace app {

class RecentGrid : public ui::Grid,
                   public AnimatedWidget {
public:
  enum class Mode : uint8_t { BigThumbnail, SmallThumbnail, List };
  RecentGrid();
  ~RecentGrid() override;

  void setMode(Mode mode);
  static void onClick(const std::string& path);

  void onAnimationFrame() override;

protected:
  bool onProcessMessage(Message* msg) override;
  void onResize(ResizeEvent& ev) override;
  void onInitTheme(InitThemeEvent& ev) override;

private:
  void addRecent(const std::string& path, int thumbnailSize, bool pinned);
  void rebuildGrid(bool force = false);

  InlineSearch m_inline;
  int m_lastColumnCount;
  int m_currentThumbnailSize;
  Mode m_mode;
  Timer m_thumbnailTimer;
  obs::scoped_connection m_recentFilesConn;
};

} // namespace app

#endif
