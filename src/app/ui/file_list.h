// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_FILE_LIST_H_INCLUDED
#define APP_UI_FILE_LIST_H_INCLUDED
#pragma once

#include "app/file_system.h"
#include "base/paths.h"
#include "base/time.h"
#include "obs/signal.h"
#include "ui/animated_widget.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <deque>
#include <string>
#include <vector>

namespace os {
  class Surface;
}

namespace app {

  class FileList : public ui::Widget
                 , private ui::AnimatedWidget {
  public:
    FileList();
    virtual ~FileList();

    const base::paths& extensions() const { return m_exts; }
    void setExtensions(const base::paths& extensions);

    IFileItem* currentFolder() const { return m_currentFolder; }
    void setCurrentFolder(IFileItem* folder);

    IFileItem* selectedFileItem() const { return m_selected; }
    const FileItemList& fileList() const { return m_list; }
    FileItemList selectedFileItems() const;
    void deselectedFileItems();

    bool multipleSelection() { return m_multiselect; }
    void setMultipleSelection(bool multiple);

    void goUp();

    gfx::Rect mainThumbnailBounds();

    double zoom() const { return m_zoom; }
    void setZoom(const double zoom);
    void animateToZoom(const double zoom);

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
    enum {
      ANI_NONE,
      ANI_ZOOM,
    };

    struct ItemInfo {
      gfx::Rect bounds;
      gfx::Rect text;
      gfx::Rect thumbnail;
    };

    void paintItem(ui::Graphics* g, IFileItem* fi, const int i);
    void onGenerateThumbnailTick();
    void onMonitoringTick();
    void recalcAllFileItemInfo();
    ItemInfo calcFileItemInfo(int i) const;
    ItemInfo getFileItemInfo(int i) const { return m_info[i]; }
    void makeSelectedFileitemVisible();
    void regenerateList();
    int selectedIndex() const;
    void selectIndex(int index);
    void generateThumbnailForFileItem(IFileItem* fi);
    void delayThumbnailGenerationForSelectedItem();
    bool hasThumbnailsPerItem() const { return m_zoom > 1.0; }
    bool isListView() const { return !hasThumbnailsPerItem(); }
    bool isIconView() const { return hasThumbnailsPerItem(); }

    // AnimatedWidget impl
    void onAnimationStop(int animation) override;
    void onAnimationFrame() override;

    IFileItem* m_currentFolder;
    FileItemList m_list;
    std::vector<ItemInfo> m_info;

    bool m_req_valid;
    int m_req_w, m_req_h;
    IFileItem* m_selected;
    std::vector<bool> m_selectedItems;
    base::paths m_exts;

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

    // List of thumbnails to generate in the next m_monitoringTimer in
    // a isIconView()
    std::deque<IFileItem*> m_generateThumbnailsForTheseItems;

    // True if this listbox accepts selecting multiple items at the
    // same time.
    bool m_multiselect;

    double m_zoom;
    double m_fromZoom;
    double m_toZoom;

    int m_itemsPerRow;
  };

} // namespace app

#endif
