// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_CEL_OPACITY_H_INCLUDED
#define APP_CMD_SET_CEL_OPACITY_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"

namespace app { namespace cmd {
using namespace doc;

class SetCelOpacity : public Cmd,
                      public WithCel {
public:
  CMDTYPE('o', 'p', 'C', 'l', SetCelOpacity);

  SetCelOpacity(Cel* cel, int opacity);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this); }
  void onSerialize(CmdSerial& s) override;

private:
  void swap();

  int m_value;
};

}} // namespace app::cmd

#endif
