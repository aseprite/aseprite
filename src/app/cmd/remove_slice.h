// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REMOVE_SLICE_H_INCLUDED
#define APP_CMD_REMOVE_SLICE_H_INCLUDED
#pragma once

#include "app/cmd/add_slice.h"

namespace app { namespace cmd {
using namespace doc;

class RemoveSlice : public AddSlice {
public:
  RemoveSlice(Sprite* sprite, Slice* slice);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
};

}} // namespace app::cmd

#endif
