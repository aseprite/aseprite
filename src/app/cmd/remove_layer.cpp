// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/remove_layer.h"

#include "doc/layer.h"

namespace app { namespace cmd {

using namespace doc;

RemoveLayer::RemoveLayer(Layer* layer) : AddLayer(layer->parent(), layer, layer->getPrevious())
{
}

void RemoveLayer::onExecute(Context* ctx)
{
  AddLayer::onUndo(ctx);
}

void RemoveLayer::onUndo(Context* ctx)
{
  AddLayer::onRedo(ctx);
}

void RemoveLayer::onRedo(Context* ctx)
{
  AddLayer::onUndo(ctx);
}

}} // namespace app::cmd
