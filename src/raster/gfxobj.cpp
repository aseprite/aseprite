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

#include <cstring>
#include <map>
#include <utility>

#include "base/mutex.h"
#include "base/scoped_lock.h"
#include "raster/gfxobj.h"

static Mutex* objects_mutex;
static GfxObjId object_id = 0;		         // Last object ID created
static std::map<GfxObjId, GfxObj*>* objects_map; // Graphics objects map

static void insert_gfxobj(GfxObj* gfxobj);
static void erase_gfxobj(GfxObj* gfxobj);

RasterModule::RasterModule()
{
  objects_map = new std::map<GfxObjId, GfxObj*>;
  objects_mutex = new Mutex();
}

RasterModule::~RasterModule()
{
#ifndef MEMLEAK
  ASSERT(objects_map->empty());
#endif

  delete objects_map;
  delete objects_mutex;
}

//////////////////////////////////////////////////////////////////////
// GfxObj class

GfxObj::GfxObj(GfxObjType type)
{
  m_type = type;
  assign_id();
}

GfxObj::GfxObj(const GfxObj& gfxobj)
{
  m_type = gfxobj.m_type;
  assign_id();
}

GfxObj::~GfxObj()
{
  ScopedLock lock(*objects_mutex);

  // we have to remove this object from the map
  erase_gfxobj(this);
}

int GfxObj::getMemSize() const
{
  return sizeof(GfxObj);
}

void GfxObj::assign_id()
{
  ScopedLock lock(*objects_mutex);

  // we have to assign an ID for this object
  m_id = ++object_id;

  // and here we add the object in the map of graphics-objects
  insert_gfxobj(this);
}

// static
GfxObj* GfxObj::find(GfxObjId id)
{
  GfxObj* ret = NULL;
  {
    ScopedLock lock(*objects_mutex);

    std::map<GfxObjId, GfxObj*>::iterator
      it = objects_map->find(id);

    if (it != objects_map->end())
      ret = it->second;
  }
  return ret;
}

void GfxObj::_setGfxObjId(GfxObjId id)
{
  ASSERT(find(m_id) == this);
  ASSERT(find(id) == NULL);

  ScopedLock lock(*objects_mutex);

  erase_gfxobj(this);		// Remove the object from the map
  m_id = id;			// Change the ID
  insert_gfxobj(this);		// Insert the object again in the map
}

static void insert_gfxobj(GfxObj* gfxobj)
{
  objects_map->insert(std::make_pair(gfxobj->getId(), gfxobj));
}

static void erase_gfxobj(GfxObj* gfxobj)
{
  std::map<GfxObjId, GfxObj*>::iterator it
    = objects_map->find(gfxobj->getId());
  
  ASSERT(it != objects_map->end());

  objects_map->erase(it);
}
