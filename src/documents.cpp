/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "documents.h"
#include "raster/sprite.h"

#include <algorithm>

Documents::Documents()
{
}

Documents::~Documents()
{
  deleteAll();
}

void Documents::addDocument(Sprite* sprite)
{
  ASSERT(sprite != NULL);

  m_documents.insert(begin(), sprite);
}

void Documents::removeDocument(Sprite* sprite)
{
  iterator it = std::find(begin(), end(), sprite);
  ASSERT(it != end());

  if (it != end())
    m_documents.erase(it);
}

void Documents::moveDocument(Sprite* sprite, int index)
{
  removeDocument(sprite);

  m_documents.insert(begin()+index, sprite);
}

void Documents::deleteAll()
{
  for (iterator it = begin(), end = this->end(); it != end; ++it) {
    Sprite* sprite = *it;
    delete sprite;
  }
  m_documents.clear();
}
