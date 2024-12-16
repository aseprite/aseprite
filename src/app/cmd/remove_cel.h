// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REMOVE_CEL_H_INCLUDED
#define APP_CMD_REMOVE_CEL_H_INCLUDED
#pragma once

#include "app/cmd/add_cel.h"

namespace app { namespace cmd {
using namespace doc;

class RemoveCel : public AddCel {
public:
  RemoveCel(Cel* cel);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
};

}} // namespace app::cmd

#endif
