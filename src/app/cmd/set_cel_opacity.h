// Aseprite
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
  SetCelOpacity(Cel* cel, int opacity);

protected:
  void onExecute() override;
  void onUndo() override;
  void onFireNotifications() override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  int m_oldOpacity;
  int m_newOpacity;
};

}} // namespace app::cmd

#endif
