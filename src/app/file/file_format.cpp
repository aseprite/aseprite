// Aseprite
// Copyright (C) 2022-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/file/file_format.h"

#include "app/cmd.h"
#include "app/doc_undo.h"
#include "app/drm.h"
#include "app/file/file.h"
#include "app/file/format_options.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "dio/file_interface.h"
#include "doc/layer.h"
#include "undo/undo_state.h"

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
  bool result = onLoad(fop);
  // Load external undo history .aseprite-undo
  if (result) {
    try {
      load_undo_history(fop);
    }
    catch (std::exception& ex) {
      fop->setError("Error loading undo history: %s", ex.what());
    }
  }
  return result;
}

#ifdef ENABLE_SAVE
bool FileFormat::save(FileOp* fop)
{
  DRM_INVALID return false;

  ASSERT(support(FILE_SUPPORT_SAVE));
  bool result = onSave(fop);

  // Save external undo history .aseprite-undo
  if (fop->config().saveUndoHistory == SaveUndoHistory::External)
    save_undo_history(fop);

  return result;
}
#endif // ENABLE_SAVE

bool FileFormat::postLoad(FileOp* fop)
{
  return onPostLoad(fop);
}

} // namespace app
