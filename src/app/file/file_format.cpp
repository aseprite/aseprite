// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/file/file_format.h"
#include "app/file/format_options.h"

#include <algorithm>

namespace app {

FileFormat::FileFormat()
{
}

FileFormat::~FileFormat()
{
}

const char* FileFormat::name() const
{
  return onGetName();
}

const char* FileFormat::extensions() const
{
  return onGetExtensions();
}

docio::FileFormat FileFormat::docioFormat() const
{
  return onGetDocioFormat();
}

bool FileFormat::load(FileOp* fop)
{
  ASSERT(support(FILE_SUPPORT_LOAD));
  return onLoad(fop);
}

#ifdef ENABLE_SAVE
bool FileFormat::save(FileOp* fop)
{
  ASSERT(support(FILE_SUPPORT_SAVE));
  return onSave(fop);
}
#endif

bool FileFormat::postLoad(FileOp* fop)
{
  return onPostLoad(fop);
}

void FileFormat::destroyData(FileOp* fop)
{
  onDestroyData(fop);
}

} // namespace app
