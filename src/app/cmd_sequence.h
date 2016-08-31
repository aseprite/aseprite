// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SEQUENCE_H_INCLUDED
#define APP_CMD_SEQUENCE_H_INCLUDED
#pragma once

#include "app/cmd.h"

#include <vector>

namespace app {

  class CmdSequence : public Cmd {
  public:
    CmdSequence();
    ~CmdSequence();

    void add(Cmd* cmd);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override;

    // Helper to create a CmdSequence in the same onExecute() member
    // function.
    void executeAndAdd(Cmd* cmd);

  private:
    std::vector<Cmd*> m_cmds;
  };

} // namespace app

#endif
