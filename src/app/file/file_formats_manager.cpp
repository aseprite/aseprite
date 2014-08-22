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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <algorithm>

#include "app/file/file_formats_manager.h"
#include "app/file/file_format.h"
#include "app/file/format_options.h"

namespace app {

extern FileFormat* CreateAseFormat();
extern FileFormat* CreateBmpFormat();
extern FileFormat* CreateFliFormat();
extern FileFormat* CreateGifFormat();
extern FileFormat* CreateIcoFormat();
extern FileFormat* CreateJpegFormat();
extern FileFormat* CreatePcxFormat();
extern FileFormat* CreatePngFormat();
extern FileFormat* CreateTgaFormat();

static FileFormatsManager* singleton = NULL;

// static
FileFormatsManager* FileFormatsManager::instance()
{
  if (!singleton)
    singleton = new FileFormatsManager();
  return singleton;
}

// static
void FileFormatsManager::destroyInstance()
{
  delete singleton;
  singleton = NULL;
}

FileFormatsManager::~FileFormatsManager()
{
  FileFormatsList::iterator end = this->end();
  for (FileFormatsList::iterator it = begin(); it != end; ++it) {
    delete (*it);               // delete the FileFormat
  }
}

void FileFormatsManager::registerAllFormats()
{
  registerFormat(CreateAseFormat());
  registerFormat(CreateBmpFormat());
  registerFormat(CreateFliFormat());
  registerFormat(CreateGifFormat());
  registerFormat(CreateIcoFormat());
  registerFormat(CreateJpegFormat());
  registerFormat(CreatePcxFormat());
  registerFormat(CreatePngFormat());
  registerFormat(CreateTgaFormat());
}

void FileFormatsManager::registerFormat(FileFormat* fileFormat)
{
  m_formats.push_back(fileFormat);
}

FileFormatsList::iterator FileFormatsManager::begin()
{
  return m_formats.begin();
}

FileFormatsList::iterator FileFormatsManager::end()
{
  return m_formats.end();
}

FileFormat* FileFormatsManager::getFileFormatByExtension(const char* extension) const
{
  char buf[512], *tok;

  for (FileFormat* ff : m_formats) {
    strcpy(buf, ff->extensions());

    for (tok=strtok(buf, ","); tok;
         tok=strtok(NULL, ",")) {
      if (stricmp(extension, tok) == 0)
        return ff;
    }
  }

  return NULL;
}

} // namespace app
