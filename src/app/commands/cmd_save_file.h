// Aseprite
// Copyright (C) 2001-2016  David Capello
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
  class Document;
  class FileSelectorDelegate;

  class SaveFileBaseCommand : public Command {
  public:
    SaveFileBaseCommand(const char* shortName, const char* friendlyName, CommandFlags flags);

    std::string selectedFilename() const {
      return m_selectedFilename;
    }

  protected:
    void onLoadParams(const Params& params) override;
    bool onEnabled(Context* context) override;

    bool saveAsDialog(Context* context, const char* dlgTitle,
                      FileSelectorDelegate* delegate = nullptr);
    void saveDocumentInBackground(const Context* context,
                                  const app::Document* document,
                                  bool markAsSaved) const;

    std::string m_filename;
    std::string m_filenameFormat;
    std::string m_selectedFilename;
    std::string m_frameTag;
    doc::SelectedFrames m_selFrames;
    bool m_adjustFramesByFrameTag;
  };

} // namespace app

#endif
