/* ASE - Allegro Sprite Editor
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

#include "objects_container_impl.h"

using namespace undo;

ObjectsContainerImpl::ObjectsContainerImpl()
{
  m_idCounter = 0;
}

ObjectsContainerImpl::~ObjectsContainerImpl()
{
}

ObjectId ObjectsContainerImpl::addObject(void* object)
{
  // First we check if the object is already in the container.
  std::map<void*, ObjectId>::iterator it = m_ptrToId.find(object);
  if (it != m_ptrToId.end())
    return it->second;          // So we return the already assigned ID

  // In other case we add the new object
  ObjectId id = ++m_idCounter;

  m_idToPtr[id] = object;
  m_ptrToId[object] = id;

  return id;
}

void ObjectsContainerImpl::insertObject(ObjectId id, void* object)
{
  std::map<ObjectId, void*>::iterator it1 = m_idToPtr.find(id);
  if (it1 != m_idToPtr.end())
    throw ExistentObjectException();

  std::map<void*, ObjectId>::iterator it2 = m_ptrToId.find(object);
  if (it2 != m_ptrToId.end())
    throw ExistentObjectException();

  m_idToPtr[id] = object;
  m_ptrToId[object] = id;
}

void ObjectsContainerImpl::removeObject(ObjectId id)
{
  std::map<ObjectId, void*>::iterator it1 = m_idToPtr.find(id);
  if (it1 == m_idToPtr.end())
    throw ObjectNotFoundException();

  void* ptr = it1->second;
  std::map<void*, ObjectId>::iterator it2 = m_ptrToId.find(ptr);
  if (it2 == m_ptrToId.end())
    throw ObjectNotFoundException();

  m_idToPtr.erase(it1);
  m_ptrToId.erase(it2);
}

void* ObjectsContainerImpl::getObject(ObjectId id)
{
  std::map<ObjectId, void*>::iterator it = m_idToPtr.find(id);
  if (it == m_idToPtr.end())
    throw ObjectNotFoundException();

  return it->second;
}
