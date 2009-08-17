/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

enum {
  GFXOBJ_CEL,
  GFXOBJ_IMAGE,
  GFXOBJ_LAYER_IMAGE,
  GFXOBJ_LAYER_SET,
  GFXOBJ_MASK,
  GFXOBJ_PALETTE,
  GFXOBJ_PATH,
  GFXOBJ_SPRITE,
  GFXOBJ_STOCK,
  GFXOBJ_UNDO,
};

typedef unsigned int gfxobj_id;

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

void gfxobj_init();
void gfxobj_exit();

GfxObj* gfxobj_find(gfxobj_id id);

void _gfxobj_set_id(GfxObj* gfxobj, gfxobj_id id);

#endif
