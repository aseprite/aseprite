// Aseprite
// Copyright (C) 2023-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SEQUENCE_H_INCLUDED
#define APP_CMD_SEQUENCE_H_INCLUDED
#pragma once

#include "app/cmd.h"

#include <vector>

namespace app { namespace cmd {

class CmdSequence : public Cmd {
public:
  CMDTYPE2('[', ' ', ' ', ' ', CmdSequence);

  CmdSequence();
  ~CmdSequence();

  void add(Cmd* cmd);
  void addAndExecute(Context* ctx, Cmd* cmd);

  // Helper to create a CmdSequence in the same onExecute() member
  // function.
  void executeAndAdd(Context* ctx, Cmd* cmd);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onRedo(Context* ctx) override;
  size_t onMemSize() const override;
  bool onIsSerializable() const override;
  void onSerialize(CmdSerial& s) override;

private:
  std::vector<Cmd*> m_cmds;
};

}} // namespace app::cmd

#endif
