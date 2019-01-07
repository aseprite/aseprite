// Aseprite
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_CMD_OPEN_FILE_H_INCLUDED
#define APP_COMMANDS_CMD_OPEN_FILE_H_INCLUDED
#pragma once

#include "app/commands/command.h"
#include "base/paths.h"

#include <string>

namespace app {

  class OpenFileCommand : public Command {
  public:
    enum class SequenceDecision {
      Ask, Agree, Skip,
    };

    OpenFileCommand();

    const base::paths& usedFiles() const {
      return m_usedFiles;
    }

  protected:
    void onLoadParams(const Params& params) override;
    void onExecute(Context* context) override;

  private:
    std::string m_filename;
    std::string m_folder;
    bool m_repeatCheckbox;
    bool m_oneFrame;
    base::paths m_usedFiles;
    SequenceDecision m_seqDecision;
  };

} // namespace app

#endif
