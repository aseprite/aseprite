// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_LAYER_OPACITY_H_INCLUDED
#define APP_CMD_SET_LAYER_OPACITY_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_layer.h"

namespace doc {
class LayerImage;
}

namespace app { namespace cmd {
using namespace doc;

class SetLayerOpacity : public Cmd,
                        public WithLayer {
public:
  CMDTYPE('o', 'p', 'L', 'y');

  SetLayerOpacity(LayerImage* layer, int opacity);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onFireNotifications(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  int m_oldOpacity;
  int m_newOpacity;
};

}} // namespace app::cmd

#endif
