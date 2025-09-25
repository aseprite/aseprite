// Aseprite
// Copyright (C) 2020-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/add_cel.h"

#include "app/doc.h"
#include "app/doc_event.h"
#include "base/serialization.h"
#include "doc/cel.h"
#include "doc/layer.h"

namespace app { namespace cmd {

using namespace base::serialization;
using namespace base::serialization::little_endian;
using namespace doc;

AddCel::AddCel(Layer* layer, Cel* cel) : WithLayer(layer), WithCel(cel)
{
}

void AddCel::onExecute()
{
  Layer* layer = this->layer();
  Cel* cel = this->cel();
  ASSERT(layer);
  ASSERT(cel);

  addCel(layer, cel);
}

void AddCel::onUndo()
{
  Layer* layer = this->layer();
  Cel* cel = this->cel();
  ASSERT(layer);
  ASSERT(cel);

  // TODO we must suspend the cel after removing the cel, we cannot trigger onBeforeRemoveCel() with
  // a cel without ID
  m_suspendedCel.suspend(cel);
  removeCel(layer, cel);
}

void AddCel::onRedo()
{
  Layer* layer = this->layer();
  Cel* cel = m_suspendedCel.restore();
  ASSERT(layer);
  ASSERT(cel);

  addCel(layer, cel);
}

void AddCel::addCel(Layer* layer, Cel* cel)
{
  static_cast<LayerImage*>(layer)->addCel(cel);
  layer->incrementVersion();

  Doc* doc = static_cast<Doc*>(cel->document());
  DocEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  ev.cel(cel);
  doc->notify_observers<DocEvent&>(&DocObserver::onAddCel, ev);
}

void AddCel::removeCel(Layer* layer, Cel* cel)
{
  Doc* doc = static_cast<Doc*>(cel->document());
  DocEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  ev.cel(cel);
  doc->notify_observers<DocEvent&>(&DocObserver::onBeforeRemoveCel, ev);

  static_cast<LayerImage*>(layer)->removeCel(cel);
  layer->incrementVersion();

  doc->notify_observers<DocEvent&>(&DocObserver::onAfterRemoveCel, ev);
}

}} // namespace app::cmd
