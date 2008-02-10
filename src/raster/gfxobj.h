/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#ifndef RASTER_GFXOBJ_H
#define RASTER_GFXOBJ_H

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

/* struct GfxObjProperty; */

typedef struct GfxObj GfxObj;

struct GfxObj
{
  int type;
  unsigned int id;
/*   struct GfxObjProperty *properties; */
};

bool gfxobj_init(void);
void gfxobj_exit(void);

GfxObj *gfxobj_new(int type, int size);
void gfxobj_free(GfxObj *gfxobj);

GfxObj *gfxobj_find(unsigned int id);

void _gfxobj_set_id(GfxObj *gfxobj, int id);

/* void gfxobj_set_data (GfxObj *gfxobj, const char *key, void *data); */
/* void *gfxobj_get_data (GfxObj *gfxobj, const char *key); */

#endif /* RASTER_GFXOBJ_H */
