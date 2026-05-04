// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_CEL_POSITION_H_INCLUDED
#define APP_CMD_SET_CEL_POSITION_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "gfx/point.h"

namespace app { namespace cmd {
using namespace doc;

class SetCelPosition : public Cmd,
                       public WithCel {
public:
  SetCelPosition(Cel* cel, const gfx::Point& newPosition);
  SetCelPosition(Cel* cel, int x, int y) : SetCelPosition(cel, gfx::Point(x, y)) {}

protected:
  void onExecute() override;
  void onUndo() override;
  void onFireNotifications() override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  gfx::Point m_old, m_new;
};

}} // namespace app::cmd

#endif
