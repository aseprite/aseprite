// Aseprite
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

namespace app {
namespace cmd {

using namespace base::serialization;
using namespace base::serialization::little_endian;
using namespace doc;

AddCel::AddCel(Layer* layer, Cel* cel)
  : WithLayer(layer)
  , WithCel(cel)
  , m_size(0)
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
  bool has_data = (cel->links() == 0);
  write8(m_stream, has_data ? 1: 0);
  if (has_data) {
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
  bool has_data = (read8(m_stream) != 0);
  if (has_data) {
    ImageRef image(read_image(m_stream));
    io.addImageRef(image);

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
  doc->notify_observers<DocEvent&>(&DocObserver::onRemoveCel, ev);

  static_cast<LayerImage*>(layer)->removeCel(cel);
  layer->incrementVersion();
  delete cel;
}

} // namespace cmd
} // namespace app
