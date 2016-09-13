// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_FILE_LIST_H_INCLUDED
#define APP_UI_FILE_LIST_H_INCLUDED
#pragma once

#include "app/file_system.h"
#include "base/time.h"
#include "obs/signal.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <string>

namespace she {
  class Surface;
}

namespace app {

  class FileList : public ui::Widget {
  public:
    FileList();
    virtual ~FileList();

    const std::string& extensions() const { return m_exts; }
    void setExtensions(const char* extensions);

    IFileItem* getCurrentFolder() const { return m_currentFolder; }
    void setCurrentFolder(IFileItem* folder);

    IFileItem* getSelectedFileItem() const { return m_selected; }
    const FileItemList& getFileList() const { return m_list; }

    void goUp();

    gfx::Rect thumbnailBounds();

    obs::signal<void()> FileSelected;
    obs::signal<void()> FileAccepted;
    obs::signal<void()> CurrentFolderChanged;

  protected:
    virtual bool onProcessMessage(ui::Message* msg) override;
    virtual void onPaint(ui::PaintEvent& ev) override;
    virtual void onSizeHint(ui::SizeHintEvent& ev) override;
    virtual void onFileSelected();
    virtual void onFileAccepted();
    virtual void onCurrentFolderChanged();

  private:
    void onGenerateThumbnailTick();
    void onMonitoringTick();
    gfx::Size getFileItemSize(IFileItem* fi) const;
    void makeSelectedFileitemVisible();
    void regenerateList();
    int getSelectedIndex();
    void selectIndex(int index);
    void generatePreviewOfSelectedItem();
    int thumbnailY();

    IFileItem* m_currentFolder;
    FileItemList m_list;
    bool m_req_valid;
    int m_req_w, m_req_h;
    IFileItem* m_selected;
    std::string m_exts;

    // Incremental-search
    std::string m_isearch;
    base::tick_t m_isearchClock;

    // Timer to start generating the thumbnail after an item is
    // selected.
    ui::Timer m_generateThumbnailTimer;

    // Monitoring the progress of each thumbnail.
    ui::Timer m_monitoringTimer;

    // Used keep the last-selected item in the list so we know
    // thumbnail to generate when the m_generateThumbnailTimer ticks.
    IFileItem* m_itemToGenerateThumbnail;

    she::Surface* m_thumbnail;
  };

} // namespace app

#endif
