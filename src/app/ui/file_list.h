/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifndef APP_UI_FILE_LIST_H_INCLUDED
#define APP_UI_FILE_LIST_H_INCLUDED

#include "app/file_system.h"
#include "base/compiler_specific.h"
#include "base/signal.h"
#include "base/string.h"
#include "ui/timer.h"
#include "ui/widget.h"

namespace app {

  class FileList : public ui::Widget {
  public:
    FileList();
    virtual ~FileList();

    void setExtensions(const char* extensions);

    IFileItem* getCurrentFolder() const { return m_currentFolder; }
    void setCurrentFolder(IFileItem* folder);

    IFileItem* getSelectedFileItem() const { return m_selected; }
    const FileItemList& getFileList() const { return m_list; }

    void goUp();

    Signal0<void> FileSelected;
    Signal0<void> FileAccepted;
    Signal0<void> CurrentFolderChanged;

  protected:
    virtual bool onProcessMessage(ui::Message* msg) OVERRIDE;
    virtual void onPreferredSize(ui::PreferredSizeEvent& ev) OVERRIDE;
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

    IFileItem* m_currentFolder;
    FileItemList m_list;
    bool m_req_valid;
    int m_req_w, m_req_h;
    IFileItem* m_selected;
    base::string m_exts;

    // Incremental-search
    std::string m_isearch;
    int m_isearchClock;

    // Timer to start generating the thumbnail after an item is
    // selected.
    ui::Timer m_generateThumbnailTimer;

    // Monitoring the progress of each thumbnail.
    ui::Timer m_monitoringTimer;

    // Used keep the last-selected item in the list so we know
    // thumbnail to generate when the m_generateThumbnailTimer ticks.
    IFileItem* m_itemToGenerateThumbnail;

  };

} // namespace app

#endif
