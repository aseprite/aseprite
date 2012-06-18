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

#ifndef WIDGETS_FILE_LIST_H_INCLUDED
#define WIDGETS_FILE_LIST_H_INCLUDED

#include "base/compiler_specific.h"
#include "base/signal.h"
#include "base/string.h"
#include "file_system.h"
#include "gui/timer.h"
#include "gui/widget.h"

#include "modules/gui.h"        // For monitors

#include <vector>

namespace widgets {

  class FileList : public ui::Widget
  {
  public:
    FileList();
    virtual ~FileList();

    void setExtensions(const char* extensions);

    IFileItem* getCurrentFolder() const { return m_currentFolder; }
    void setCurrentFolder(IFileItem* folder);

    IFileItem* getSelectedFileItem() const { return m_selected; }
    const FileItemList& getFileList() const { return m_list; }

    void goUp();

    void removeMonitor(Monitor* monitor);

    Signal0<void> FileSelected;
    Signal0<void> FileAccepted;
    Signal0<void> CurrentFolderChanged;

  protected:
    virtual bool onProcessMessage(ui::Message* msg) OVERRIDE;
    virtual void onFileSelected();
    virtual void onFileAccepted();
    virtual void onCurrentFolderChanged();

  private:
    gfx::Size getFileItemSize(IFileItem* fi) const;
    void makeSelectedFileitemVisible();
    void regenerateList();
    int getSelectedIndex();
    void selectIndex(int index);
    void generatePreviewOfSelectedItem();
    bool generateThumbnail(IFileItem* fileitem);
    void stopThreads();

    IFileItem* m_currentFolder;
    FileItemList m_list;
    bool m_req_valid;
    int m_req_w, m_req_h;
    IFileItem* m_selected;
    base::string m_exts;

    // Incremental-search
    std::string m_isearch;
    int m_isearchClock;

    /* thumbnail generation process */
    IFileItem* m_itemToGenerateThumbnail;
    ui::Timer m_timer;
    MonitorList m_monitors; // list of monitors watching threads

  };

} // namespace widgets

#endif
