/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "undoers/set_stock_pixel_format.h"

#include "raster/stock.h"
#include "undo/objects_container.h"
#include "undo/undoers_collector.h"

using namespace undo;
using namespace undoers;

SetStockPixelFormat::SetStockPixelFormat(ObjectsContainer* objects, Stock* stock)
  : m_stockId(objects->addObject(stock))
  , m_format(stock->getPixelFormat())
{
}

void SetStockPixelFormat::dispose()
{
  delete this;
}

void SetStockPixelFormat::revert(ObjectsContainer* objects, UndoersCollector* redoers)
{
  Stock* stock = objects->getObjectT<Stock>(m_stockId);

  // Push another SetStockPixelFormat as redoer
  redoers->pushUndoer(new SetStockPixelFormat(objects, stock));

  stock->setPixelFormat(static_cast<PixelFormat>(m_format));
}
