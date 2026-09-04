// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_LAYER_BLEND_MODE_H_INCLUDED
#define APP_CMD_SET_LAYER_BLEND_MODE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_layer.h"
#include "doc/blend_mode.h"

namespace doc {
class LayerImage;
}

namespace app { namespace cmd {
using namespace doc;

class SetLayerBlendMode : public Cmd,
                          public WithLayer {
public:
  CMDTYPE('b', 'm', 'L', 'y');

  SetLayerBlendMode(LayerImage* layer, BlendMode blendMode);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onFireNotifications(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  BlendMode m_oldBlendMode;
  BlendMode m_newBlendMode;
};

}} // namespace app::cmd

#endif
