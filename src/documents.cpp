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

#include "base/mutex.h"
#include "document.h"
#include "raster/sprite.h"
#include "raster/undo_history.h"

#include <algorithm>

Documents::Documents()
{
  m_idCounter = 0;
}

Documents::~Documents()
{
  deleteAll();
}

void Documents::addDocument(Document* document)
{
  ASSERT(document != NULL);
  ASSERT(document->getId() == WithoutDocumentId);

  m_documents.insert(begin(), document);

  document->setId(++m_idCounter);
}

void Documents::removeDocument(Document* document)
{
  iterator it = std::find(begin(), end(), document);
  ASSERT(it != end());

  if (it != end())
    m_documents.erase(it);
}

void Documents::moveDocument(Document* document, int index)
{
  removeDocument(document);

  m_documents.insert(begin()+index, document);
}

void Documents::deleteAll()
{
  for (iterator it = begin(), end = this->end(); it != end; ++it) {
    Document* document = *it;
    delete document;
  }
  m_documents.clear();
}

Document* Documents::getById(DocumentId id) const
{
  for (const_iterator it = begin(), end = this->end(); it != end; ++it) {
    if ((*it)->getId() == id)
      return *it;
  }
  return NULL;
}
