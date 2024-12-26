// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_UNLINK_CEL_H_INCLUDED
#define APP_CMD_UNLINK_CEL_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"

namespace app { namespace cmd {
using namespace doc;

class UnlinkCel : public Cmd,
                  public WithCel {
public:
  UnlinkCel(Cel* cel);

protected:
  void onExecute() override;
  void onUndo() override;

private:
  ObjectId m_newImageId;
  ObjectId m_oldCelDataId;
  ObjectId m_newCelDataId;
};

}} // namespace app::cmd

#endif
