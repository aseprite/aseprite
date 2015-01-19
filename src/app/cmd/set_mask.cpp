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

#include "app/cmd/set_mask.h"

#include "app/document.h"
#include "doc/mask.h"

namespace app {
namespace cmd {

SetMask::SetMask(Document* doc, Mask* newMask)
  : WithDocument(doc)
  , m_oldMask(doc->isMaskVisible() ? new Mask(*doc->mask()): nullptr)
  , m_newMask(newMask && !newMask->isEmpty() ? new Mask(*newMask): nullptr)
{
}

void SetMask::setNewMask(Mask* newMask)
{
  m_newMask.reset(newMask ? new Mask(*newMask): nullptr);
  setMask(m_newMask);
}

void SetMask::onExecute()
{
  setMask(m_newMask);
}

void SetMask::onUndo()
{
  setMask(m_oldMask);
}

size_t SetMask::onMemSize() const
{
  return sizeof(*this) +
    (m_oldMask ? m_oldMask->getMemSize(): 0) +
    (m_newMask ? m_newMask->getMemSize(): 0);
}

void SetMask::setMask(Mask* mask)
{
  app::Document* doc = document();

  if (mask) {
    doc->setMask(mask);
    doc->setMaskVisible(!mask->isEmpty());
  }
  else {
    Mask empty;
    doc->setMask(&empty);
    doc->setMaskVisible(false);
  }
}

} // namespace cmd
} // namespace app
