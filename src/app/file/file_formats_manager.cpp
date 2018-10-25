// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/file_formats_manager.h"

#include "app/file/file_format.h"
#include "app/file/format_options.h"
#include "base/string.h"
#include "dio/detect_format.h"

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
extern FileFormat* CreateSvgFormat();
extern FileFormat* CreateTgaFormat();

#ifdef ASEPRITE_WITH_WEBP_SUPPORT
extern FileFormat* CreateWebPFormat();
#endif

static FileFormatsManager* singleton = NULL;

// static
FileFormatsManager* FileFormatsManager::instance()
{
  if (!singleton)
    singleton = new FileFormatsManager;
  return singleton;
}

// static
void FileFormatsManager::destroyInstance()
{
  delete singleton;
  singleton = NULL;
}

FileFormatsManager::FileFormatsManager()
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
  registerFormat(CreateSvgFormat());
  registerFormat(CreateTgaFormat());

#ifdef ASEPRITE_WITH_WEBP_SUPPORT
  registerFormat(CreateWebPFormat());
#endif
}

FileFormatsManager::~FileFormatsManager()
{
  FileFormatsList::iterator end = this->end();
  for (FileFormatsList::iterator it = begin(); it != end; ++it) {
    delete (*it);               // delete the FileFormat
  }
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

FileFormat* FileFormatsManager::getFileFormat(const dio::FileFormat dioFormat) const
{
  for (FileFormat* ff : m_formats)
    if (ff->dioFormat() == dioFormat)
      return ff;
  return nullptr;
}

} // namespace app
