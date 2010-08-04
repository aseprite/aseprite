/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "Vaca/Mutex.h"
#include "Vaca/ScopedLock.h"

#include "raster/gfxobj.h"

using Vaca::Mutex;
using Vaca::ScopedLock;

static Mutex* objects_mutex;
static gfxobj_id object_id = 0;		          // last object ID created
static std::map<gfxobj_id, GfxObj*>* objects_map; // graphics objects map

static void insert_gfxobj(GfxObj* gfxobj);
static void erase_gfxobj(GfxObj* gfxobj);

RasterModule::RasterModule()
{
  objects_map = new std::map<gfxobj_id, GfxObj*>;
  objects_mutex = new Mutex();
}

RasterModule::~RasterModule()
{
  ASSERT(objects_map->empty());
  delete objects_map;
  delete objects_mutex;
}

//////////////////////////////////////////////////////////////////////
// GfxObj class

GfxObj::GfxObj(int type)
{
  this->type = type;
  assign_id();
}

GfxObj::GfxObj(const GfxObj& gfxobj)
{
  this->type = gfxobj.type;
  assign_id();
}

GfxObj::~GfxObj()
{
  ScopedLock lock(*objects_mutex);

  // we have to remove this object from the map
  erase_gfxobj(this);
}

void GfxObj::assign_id()
{
  ScopedLock lock(*objects_mutex);

  // we have to assign an ID for this object
  this->id = ++object_id;

  // and here we add the object in the map of graphics-objects
  insert_gfxobj(this);
}

//////////////////////////////////////////////////////////////////////

GfxObj* gfxobj_find(gfxobj_id id)
{
  GfxObj* ret = NULL;
  {
    ScopedLock lock(*objects_mutex);

    std::map<gfxobj_id, GfxObj*>::iterator
      it = objects_map->find(id);

    if (it != objects_map->end())
      ret = it->second;
  }
  return ret;
}

void _gfxobj_set_id(GfxObj* gfxobj, gfxobj_id id)
{
  ASSERT(gfxobj_find(gfxobj->id) == gfxobj);
  ASSERT(gfxobj_find(id) == NULL);

  ScopedLock lock(*objects_mutex);

  erase_gfxobj(gfxobj);	// remove the object
  gfxobj->id = id;		// change the ID
  insert_gfxobj(gfxobj);	// insert the object again in the map
}

//////////////////////////////////////////////////////////////////////

static void insert_gfxobj(GfxObj* gfxobj)
{
  objects_map->insert(std::make_pair(gfxobj->id, gfxobj));
}

static void erase_gfxobj(GfxObj* gfxobj)
{
  std::map<gfxobj_id, GfxObj*>::iterator
    it = objects_map->find(gfxobj->id);
  
  ASSERT(it != objects_map->end());

  objects_map->erase(it);
}
