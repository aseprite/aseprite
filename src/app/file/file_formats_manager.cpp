// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/file_formats_manager.h"

#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "base/string.h"

#include <algorithm>
#include <cstring>

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
extern FileFormat* CreateWebPFormat();

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
  // The first format is the default image format in FileSelector
  registerFormat(CreateAseFormat());
  registerFormat(CreateBmpFormat());
  registerFormat(CreateFliFormat());
  registerFormat(CreateGifFormat());
  registerFormat(CreateIcoFormat());
  registerFormat(CreateJpegFormat());
  registerFormat(CreatePcxFormat());
  registerFormat(CreatePngFormat());
  registerFormat(CreateTgaFormat());
  registerFormat(CreateWebPFormat());
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
    std::strcpy(buf, ff->extensions());

    for (tok=std::strtok(buf, ","); tok;
         tok=std::strtok(NULL, ",")) {
      if (base::utf8_icmp(extension, tok) == 0)
        return ff;
    }
  }

  return NULL;
}

} // namespace app
