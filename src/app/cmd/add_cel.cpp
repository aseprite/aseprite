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
#include "doc/cel_data_io.h"
#include "doc/cel_io.h"
#include "doc/image_io.h"
#include "doc/layer.h"
#include "doc/subobjects_io.h"

namespace app { namespace cmd {

using namespace base::serialization;
using namespace base::serialization::little_endian;
using namespace doc;

AddCel::AddCel(Layer* layer, Cel* cel) : WithLayer(layer), WithCel(cel), m_size(0)
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

  // Save the CelData only if the cel isn't linked
  const bool has_data = (cel->links() == 0);
  const bool has_image = (cel->imageId() != NullId);
  write8(m_stream, (has_data ? 1 : 0) | (has_image ? 2 : 0));
  if (has_data) {
    if (has_image)
      write_image(m_stream, cel->image());
    write_celdata(m_stream, cel->data());
  }
  write_cel(m_stream, cel);
  m_size = size_t(m_stream.tellp());

  removeCel(layer, cel);
}

void AddCel::onRedo()
{
  Layer* layer = this->layer();
  ASSERT(layer);

  SubObjectsFromSprite io(layer->sprite());
  const uint8_t flags = read8(m_stream);
  const bool has_data = (flags & 1 ? true : false);
  const bool has_image = (flags & 2 ? true : false);
  if (has_data) {
    if (has_image) {
      ImageRef image(read_image(m_stream));
      io.addImageRef(image);
    }

    CelDataRef celdata(read_celdata(m_stream, &io));
    io.addCelDataRef(celdata);
  }
  Cel* cel = read_cel(m_stream, &io);
  ASSERT(cel);

  addCel(layer, cel);

  m_stream.str(std::string());
  m_stream.clear();
  m_size = 0;
}

void AddCel::addCel(Layer* layer, Cel* cel)
{
  layer->addCel(cel);
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

  layer->removeCel(cel);
  layer->incrementVersion();

  doc->notify_observers<DocEvent&>(&DocObserver::onAfterRemoveCel, ev);

  delete cel;
}

}} // namespace app::cmd
