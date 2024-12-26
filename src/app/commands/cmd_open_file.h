// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_CMD_OPEN_FILE_H_INCLUDED
#define APP_COMMANDS_CMD_OPEN_FILE_H_INCLUDED
#pragma once

#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/pref/preferences.h"
#include "base/paths.h"

#include <string>

namespace app {

class OpenFileCommand : public Command {
public:
  OpenFileCommand();

  const base::paths& usedFiles() const { return m_usedFiles; }

  gen::SequenceDecision seqDecision() const { return m_seqDecision; }

protected:
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;
  std::string onGetFriendlyName() const override;

private:
  std::string m_filename;
  std::string m_folder;
  bool m_ui;
  bool m_repeatCheckbox;
  bool m_oneFrame;
  base::paths m_usedFiles;
  gen::SequenceDecision m_seqDecision;
};

} // namespace app

#endif
