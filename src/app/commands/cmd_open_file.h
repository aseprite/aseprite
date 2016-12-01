// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_CMD_OPEN_FILE_H_INCLUDED
#define APP_COMMANDS_CMD_OPEN_FILE_H_INCLUDED
#pragma once

#include "app/commands/command.h"

#include <string>
#include <vector>

namespace app {

  class OpenFileCommand : public Command {
  public:
    enum class SequenceDecision {
      Ask, Agree, Skip,
    };

    OpenFileCommand();
    Command* clone() const override { return new OpenFileCommand(*this); }

    const std::vector<std::string>& usedFiles() const {
      return m_usedFiles;
    }

  protected:
    void onLoadParams(const Params& params) override;
    void onExecute(Context* context) override;

  private:
    std::string m_filename;
    std::string m_folder;
    bool m_repeatCheckbox;
    std::vector<std::string> m_usedFiles;
    SequenceDecision m_seqDecision;
  };

} // namespace app

#endif
