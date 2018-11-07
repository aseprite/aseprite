// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_CMD_SAVE_FILE_H_INCLUDED
#define APP_COMMANDS_CMD_SAVE_FILE_H_INCLUDED
#pragma once

#include "app/commands/command.h"
#include "doc/selected_frames.h"

#include <string>

namespace app {
  class Doc;

  class SaveFileBaseCommand : public Command {
  public:
    SaveFileBaseCommand(const char* id, CommandFlags flags);

  protected:
    void onLoadParams(const Params& params) override;
    bool onEnabled(Context* context) override;

    std::string saveAsDialog(
      Context* context,
      const std::string& dlgTitle,
      const std::string& filename,
      const bool markAsSaved,
      const bool saveInBackground = true,
      const std::string& forbiddenFilename = std::string());
    void saveDocumentInBackground(
      const Context* context,
      Doc* document,
      const std::string& filename,
      const bool markAsSaved);

    std::string m_filename;
    std::string m_filenameFormat;
    std::string m_frameTag;
    std::string m_aniDir;
    std::string m_slice;
    doc::SelectedFrames m_selFrames;
    bool m_adjustFramesByFrameTag;
    bool m_useUI;
    bool m_ignoreEmpty;
  };

} // namespace app

#endif
