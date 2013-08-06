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

#ifndef APP_FILE_FILE_FORMATS_MANAGER_H_INCLUDED
#define APP_FILE_FILE_FORMATS_MANAGER_H_INCLUDED

#include <vector>

namespace app {

  class FileFormat;

  // A list of file formats. Used by the FileFormatsManager to keep
  // track of all known file extensions supported by ASE.
  typedef std::vector<FileFormat*> FileFormatsList;

  // Manages the list of known formats by ASEPRITE (image file format that can
  // be loaded and/or saved).
  class FileFormatsManager {
  public:
    // Returns a singleton of this class.
    static FileFormatsManager& instance();

    virtual ~FileFormatsManager();

    void registerAllFormats();

    // Iterators to access to the list of formats.
    FileFormatsList::iterator begin();
    FileFormatsList::iterator end();

  private:
    // Register one format.
    void registerFormat(FileFormat* fileFormat);

    FileFormatsList m_formats;
  };

} // namespace app

#endif
