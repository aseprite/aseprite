// Aseprite
// Copyright (c) 2025  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_ADD_LAYER_H_INCLUDED
#define APP_CMD_ADD_LAYER_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_layer.h"
#include "app/cmd/with_suspended.h"
#include "doc/layer.h"

namespace app { namespace cmd {
using namespace doc;

class AddLayer : public Cmd {
public:
  AddLayer(Layer* group, Layer* newLayer, Layer* afterThis);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_suspendedLayer.size(); }

private:
  void addLayer(Layer* group, Layer* newLayer, Layer* afterThis);
  void removeLayer(Layer* group, Layer* layer);

  WithLayer m_group;
  WithLayer m_newLayer;
  WithLayer m_afterThis;
  WithSuspended<doc::Layer*> m_suspendedLayer;
};

}} // namespace app::cmd

#endif
