// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_UNDO_HISTORY_H_INCLUDED
#define APP_FILE_UNDO_HISTORY_H_INCLUDED
#pragma once

namespace app {

class FileOp;

enum class SaveUndoHistory {
  No,
  Embed,
  External,
};

void load_undo_history(FileOp* fop);
void save_undo_history(FileOp* fop);

} // namespace app

#endif
