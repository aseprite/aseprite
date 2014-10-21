/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#include "app/undoers/remove_cel.h"

#include "app/undoers/add_cel.h"
#include "app/undoers/object_io.h"
#include "doc/cel.h"
#include "doc/cel_io.h"
#include "doc/layer.h"
#include "doc/stock.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

namespace app {
namespace undoers {

using namespace doc;
using namespace undo;

RemoveCel::RemoveCel(ObjectsContainer* objects, Layer* layer, Cel* cel)
  : m_layerId(objects->addObject(layer))
{
  write_object(objects, m_stream, cel, doc::write_cel);
}

void RemoveCel::dispose()
{
  delete this;
}

void RemoveCel::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  LayerImage* layer = objects->getObjectT<LayerImage>(m_layerId);
  Cel* cel = read_object<Cel>(objects, m_stream, doc::read_cel);

  // Push an AddCel as redoer
  redoers->pushUndoer(new AddCel(objects, layer, cel));

  layer->addCel(cel);
}

} // namespace undoers
} // namespace app
