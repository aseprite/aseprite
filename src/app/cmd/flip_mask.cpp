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

#include "app/cmd/flip_mask.h"

#include "app/document.h"
#include "doc/algorithm/flip_image.h"
#include "doc/mask.h"

namespace app {
namespace cmd {

using namespace doc;

FlipMask::FlipMask(Document* doc, doc::algorithm::FlipType flipType)
  : WithDocument(doc)
  , m_flipType(flipType)
{
}

void FlipMask::onExecute()
{
  swap();
}

void FlipMask::onUndo()
{
  swap();
}

void FlipMask::swap()
{
  Document* document = this->document();
  Mask* mask = document->mask();

  ASSERT(mask->bitmap());
  if (!mask->bitmap())
    return;

  mask->freeze();
  doc::algorithm::flip_image(mask->bitmap(),
    mask->bitmap()->bounds(), m_flipType);
  mask->unfreeze();
}

} // namespace cmd
} // namespace app
