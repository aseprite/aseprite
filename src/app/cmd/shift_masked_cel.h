// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SHIFT_MASKED_CEL_H_INCLUDED
#define APP_CMD_SHIFT_MASKED_CEL_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"

namespace app { namespace cmd {
using namespace doc;

class ShiftMaskedCel : public Cmd,
                       public WithCel {
public:
  ShiftMaskedCel(Cel* cel, int dx, int dy);

protected:
  void onExecute() override;
  void onUndo() override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  void shift(int dx, int dy);

  int m_dx, m_dy;
};

}} // namespace app::cmd

#endif
