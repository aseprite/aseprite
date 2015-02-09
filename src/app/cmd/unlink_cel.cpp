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

#include "app/cmd/unlink_cel.h"

#include "doc/cel.h"
#include "doc/image.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

UnlinkCel::UnlinkCel(Cel* cel)
  : WithCel(cel)
  , m_newImageId(0)
  , m_oldCelDataId(cel->dataRef()->id())
  , m_newCelDataId(0)
{
  ASSERT(cel->links());
}

void UnlinkCel::onExecute()
{
  Cel* cel = this->cel();
  CelDataRef oldCelData = cel->sprite()->getCelDataRef(m_oldCelDataId);
  ASSERT(oldCelData);

  ImageRef imgCopy(Image::createCopy(oldCelData->image()));
  CelDataRef celDataCopy(new CelData(*oldCelData));
  celDataCopy->setImage(imgCopy);

  if (m_newImageId) {
    imgCopy->setId(m_newImageId);
    celDataCopy->setId(m_newCelDataId);
  }
  else {
    m_newImageId = imgCopy->id();
    m_newCelDataId = celDataCopy->id();
  }

  cel->setDataRef(celDataCopy);
}

void UnlinkCel::onUndo()
{
  Cel* cel = this->cel();
  CelDataRef oldCelData = cel->sprite()->getCelDataRef(m_oldCelDataId);
  ASSERT(oldCelData);

  cel->setDataRef(oldCelData);
}

} // namespace cmd
} // namespace app
