// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_FILE_FORMATS_MANAGER_H_INCLUDED
#define APP_FILE_FILE_FORMATS_MANAGER_H_INCLUDED
#pragma once

#include "dio/file_format.h"

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
  static FileFormatsManager* instance();
  static void destroyInstance();

  virtual ~FileFormatsManager();

  // Iterators to access to the list of formats.
  FileFormatsList::iterator begin();
  FileFormatsList::iterator end();

  FileFormat* getFileFormat(const dio::FileFormat dioFormat) const;

private:
  FileFormatsManager();
  void registerFormat(FileFormat* fileFormat);

  FileFormatsList m_formats;
};

} // namespace app

#endif
