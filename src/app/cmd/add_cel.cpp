/* Aseprite
 * Copyright (C) 2001-2015  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/add_cel.h"

#include "app/cmd/object_io.h"
#include "doc/cel.h"
#include "doc/document.h"
#include "doc/document_event.h"
#include "doc/layer.h"

namespace app {
namespace cmd {

using namespace doc;

AddCel::AddCel(Layer* layer, Cel* cel)
  : WithLayer(layer)
  , WithCel(cel)
{
}

void AddCel::onExecute()
{
  Layer* layer = this->layer();
  Cel* cel = this->cel();

  addCel(layer, cel);
}

void AddCel::onUndo()
{
  Layer* layer = this->layer();
  Cel* cel = this->cel();

  ObjectIO(layer->sprite()).write_cel(m_stream, cel);

  removeCel(layer, cel);
}

void AddCel::onRedo()
{
  Layer* layer = this->layer();
  Cel* cel = ObjectIO(layer->sprite()).read_cel(m_stream);

  addCel(layer, cel);

  m_stream.str(std::string());
  m_stream.clear();
}

void AddCel::addCel(Layer* layer, Cel* cel)
{
  static_cast<LayerImage*>(layer)->addCel(cel);

  Document* doc = cel->document();
  DocumentEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  ev.cel(cel);
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onAddCel, ev);
}

void AddCel::removeCel(Layer* layer, Cel* cel)
{
  Document* doc = cel->document();
  DocumentEvent ev(doc);
  ev.sprite(layer->sprite());
  ev.layer(layer);
  ev.cel(cel);
  doc->notifyObservers<DocumentEvent&>(&DocumentObserver::onRemoveCel, ev);

  static_cast<LayerImage*>(layer)->removeCel(cel);
  delete cel;
}

} // namespace cmd
} // namespace app
