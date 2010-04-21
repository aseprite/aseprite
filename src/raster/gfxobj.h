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

#ifndef RASTER_GFXOBJ_H_INCLUDED
#define RASTER_GFXOBJ_H_INCLUDED

#include "jinete/jbase.h"
#include <list>

enum {
  GFXOBJ_CEL,
  GFXOBJ_IMAGE,
  GFXOBJ_LAYER_IMAGE,
  GFXOBJ_LAYER_FOLDER,
  GFXOBJ_MASK,
  GFXOBJ_PALETTE,
  GFXOBJ_PATH,
  GFXOBJ_SPRITE,
  GFXOBJ_STOCK,
  GFXOBJ_UNDO,
  GFXOBJ_RGBMAP,
};

typedef unsigned int gfxobj_id;

class Cel;
class Layer;

typedef std::list<Cel*> CelList;
typedef std::list<Cel*>::iterator CelIterator;
typedef std::list<Cel*>::const_iterator CelConstIterator;

typedef std::list<Layer*> LayerList;
typedef std::list<Layer*>::iterator LayerIterator;
typedef std::list<Layer*>::const_iterator LayerConstIterator;

class GfxObj
{
public:
  int type;
  gfxobj_id id;

  GfxObj(int type);
  GfxObj(const GfxObj& gfxobj);
  virtual ~GfxObj();

private:
  void assign_id();
};

class RasterModule
{
public:
  RasterModule();
  ~RasterModule();
};

GfxObj* gfxobj_find(gfxobj_id id);

void _gfxobj_set_id(GfxObj* gfxobj, gfxobj_id id);

#endif
