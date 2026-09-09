// Aseprite
// Copyright (C) 2023-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_CEL_ZINDEX_H_INCLUDED
#define APP_CMD_SET_CEL_ZINDEX_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"

namespace app { namespace cmd {
using namespace doc;

class SetCelZIndex : public Cmd,
                     public WithCel {
public:
  CMDTYPE('z', 'i', 'C', 'l', SetCelZIndex);

  SetCelZIndex(Cel* cel, int zindex);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onFireNotifications(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this); }
  void onSerialize(CmdSerial& s) override;

private:
  void swap();

  int m_value;
};

}} // namespace app::cmd

#endif
