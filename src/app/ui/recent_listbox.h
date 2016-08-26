// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_RECENT_LISTBOX_H_INCLUDED
#define APP_UI_RECENT_LISTBOX_H_INCLUDED
#pragma once

#include "base/connection.h"
#include "ui/listbox.h"

namespace app {

  class RecentFileItem;

  class RecentListBox : public ui::ListBox {
    friend class RecentFileItem;
  public:
    RecentListBox();

  protected:
    virtual void onRebuildList() = 0;
    virtual void onClick(const std::string& path) = 0;

  private:
    void rebuildList();

    base::ScopedConnection m_recentFilesConn;
    base::ScopedConnection m_showFullPathConn;
  };

  class RecentFilesListBox : public RecentListBox {
  public:
    RecentFilesListBox();

  protected:
    void onRebuildList() override;
    void onClick(const std::string& path) override;
  };

  class RecentFoldersListBox : public RecentListBox {
  public:
    RecentFoldersListBox();

  protected:
    void onRebuildList() override;
    void onClick(const std::string& path) override;
  };

} // namespace app

#endif
