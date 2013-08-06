/* ASEPRITE
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

#ifndef APP_UI_FILE_SELECTOR_H_INCLUDED
#define APP_UI_FILE_SELECTOR_H_INCLUDED

#include "base/string.h"
#include "base/unique_ptr.h"
#include "ui/window.h"

namespace ui {
  class Button;
  class ComboBox;
  class Entry;
}

namespace app {
  class CustomFileNameEntry;
  class FileList;
  class IFileItem;

  class FileSelector : public ui::Window {
  public:
    FileSelector();

    // Shows the dialog to select a file in the program.
    base::string show(const base::string& title,
                      const base::string& initialPath,
                      const base::string& showExtensions);

  private:
    void updateLocation();
    void updateNavigationButtons();
    void addInNavigationHistory(IFileItem* folder);
    void selectFileTypeFromFileName();
    void onGoBack();
    void onGoForward();
    void onGoUp();
    void onLocationChange();
    void onFileTypeChange();
    void onFileListFileSelected();
    void onFileListFileAccepted();
    void onFileListCurrentFolderChanged();

    ui::Button* m_goBack;
    ui::Button* m_goForward;
    ui::Button* m_goUp;
    ui::ComboBox* m_location;
    ui::ComboBox* m_fileType;
    CustomFileNameEntry* m_fileName;
    FileList* m_fileList;
  };

} // namespace app

#endif
