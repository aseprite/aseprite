// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_OPEN_BATCH_H_INCLUDED
#define APP_UTIL_OPEN_BATCH_H_INCLUDED
#pragma once

#include "app/commands/cmd_open_file.h"
#include "app/context.h"

namespace app {

  // Helper class to a batch of files and handle the loading of image
  // sequences and repeat the selected action of the user (Agree/Skip
  // to open the sequence, and Repeat the action for all other
  // elements)
  class OpenBatchOfFiles {
  public:
    void open(Context* ctx,
              const std::string& fn,
              const bool oneFrame) {
      Params params;
      params.set("filename", fn.c_str());

      if (oneFrame)
        params.set("oneframe", "true");
      else {
        switch (m_lastDecision) {
          case OpenFileCommand::SequenceDecision::Ask:
            params.set("sequence", "ask");
            params.set("repeat_checkbox", "true");
            break;
          case OpenFileCommand::SequenceDecision::Skip:
            params.set("sequence", "skip");
            break;
          case OpenFileCommand::SequenceDecision::Agree:
            params.set("sequence", "agree");
            break;
        }
      }

      ctx->executeCommandFromMenuOrShortcut(&m_cmd, params);

      // Future decision for other files in the CLI
      auto d = m_cmd.seqDecision();
      if (d != OpenFileCommand::SequenceDecision::Ask)
        m_lastDecision = d;
    }

    const base::paths& usedFiles() const {
      return m_cmd.usedFiles();
    }

  private:
    OpenFileCommand m_cmd;
    OpenFileCommand::SequenceDecision m_lastDecision = OpenFileCommand::SequenceDecision::Ask;
  };

} // namespace app

#endif
