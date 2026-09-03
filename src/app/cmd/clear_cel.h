// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_CLEAR_CEL_H_INCLUDED
#define APP_CMD_CLEAR_CEL_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/sequence.h"
#include "app/cmd/with_cel.h"

namespace app { namespace cmd {
using namespace doc;

class ClearCel : public Cmd,
                 public WithCel {
public:
  CMDTYPE2('c', 'l', 'C', 'l', ClearCel);

  ClearCel(Cel* cel);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onRedo(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this) + m_seq.memSize(); }
  void onSerialize(CmdSerial& s) override;

private:
  CmdSequence m_seq;
};

}} // namespace app::cmd

#endif
