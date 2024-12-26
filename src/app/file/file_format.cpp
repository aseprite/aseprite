// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/drm.h"
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

void FileFormat::getExtensions(base::paths& exts) const
{
  onGetExtensions(exts);
}

dio::FileFormat FileFormat::dioFormat() const
{
  return onGetDioFormat();
}

bool FileFormat::load(FileOp* fop)
{
  ASSERT(support(FILE_SUPPORT_LOAD));
  return onLoad(fop);
}

#ifdef ENABLE_SAVE
bool FileFormat::save(FileOp* fop)
{
  DRM_INVALID return false;

  ASSERT(support(FILE_SUPPORT_SAVE));
  return onSave(fop);
}
#endif

bool FileFormat::postLoad(FileOp* fop)
{
  return onPostLoad(fop);
}

} // namespace app
