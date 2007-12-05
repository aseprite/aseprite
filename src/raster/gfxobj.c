/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <string.h>

#include "jinete/jbase.h"
#include "jinete/jlist.h"

#include "raster/gfxobj.h"

#endif

/* typedef struct GfxObjProperty */
/* { */
/*   char *key; */
/*   void *data; */
/* } Property; */

static unsigned int object_id = 0;	/* last object ID created */
static JList objects;			/* graphics objects list */

GfxObj *gfxobj_new (int type, int size)
{
  GfxObj *gfxobj;

  gfxobj = jmalloc (size);
  if (!gfxobj)
    return NULL;

  gfxobj->type = type;
  gfxobj->id = ++object_id;
/*   gfxobj->properties = NULL; */

  if (!objects)
    objects = jlist_new();
  jlist_append(objects, gfxobj);

  return gfxobj;
}

void gfxobj_free (GfxObj *gfxobj)
{
  jlist_remove (objects, gfxobj);
  jfree (gfxobj);
}

GfxObj *gfxobj_find (unsigned int id)
{
  JLink link;

  JI_LIST_FOR_EACH(objects, link)
    if (((GfxObj *)link->data)->id == id)
      return (GfxObj *)link->data;

  return NULL;
}

void _gfxobj_set_id (GfxObj *gfxobj, int id)
{
  /* TODO */
  /* ji_assert (!gfxobj_find (id)); */

  gfxobj->id = id;
}

#if 0
void gfxobj_set_data (GfxObj *gfxobj, const char *key, void *data)
{
  Property *property;

  for (property=gfxobj->properties; property; property=property->next) {
    /* existent property? */
    if (strcmp(property->key, key) == 0) {
      /* change data */
      if (data)
	property->data = data;
      /* if data == NULL, we can remove this property */
      else {
	jfree(property->key);
	jfree(property);
	jlist_delete_link(gfxobj->properties, link);
      }
      return;
    }
  }

  /* new property */
  property = jnew(Property, 1);
  property->key = jstrdup(key);
  property->data = data;

  jlist_append(gfxobj->properties, property);
}

void *gfxobj_get_data(GfxObj *gfxobj, const char *key)
{
  Property *property;
  JLink link;

  for (it=gfxobj->properties; it; it=it->next) {
    property = it->data;

    if (strcmp (property->key, key) == 0)
      return property->data;
  }

  return NULL;
}
#endif
