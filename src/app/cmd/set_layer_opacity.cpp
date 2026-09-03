// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_layer_opacity.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

SetLayerOpacity::SetLayerOpacity(LayerImage* layer, int opacity)
  : WithLayer(layer)
  , m_value(opacity)
{
}

void SetLayerOpacity::onExecute(Context* ctx)
{
  swap();
}

void SetLayerOpacity::onUndo(Context* ctx)
{
  swap();
}

void SetLayerOpacity::onSerialize(CmdSerial& s)
{
  Cmd::onSerialize(s);
  serializeLayerId(s);
  s(m_value);
}

void SetLayerOpacity::swap()
{
  Layer* layer = this->layer();

  auto current = layer->opacity();
  std::swap(current, m_value);
  layer->setOpacity(current);
  layer->incrementVersion();

  Doc* doc = static_cast<Doc*>(layer->sprite()->document());
  DocEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  doc->notify_observers<DocEvent&>(&DocObserver::onLayerOpacityChange, ev);
}

}} // namespace app::cmd
