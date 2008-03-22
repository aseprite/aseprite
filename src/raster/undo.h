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

#ifndef RASTER_UNDO_H
#define RASTER_UNDO_H

#include <string.h>		/* for strlen */

#include "raster/gfxobj.h"

struct Cel;
struct Dirty;
struct Image;
struct Layer;
struct Mask;
struct Sprite;
struct Stock;
struct UndoStream;

typedef struct Undo Undo;

struct Undo
{
  GfxObj gfxobj;
  struct Sprite *sprite;	/* related sprite */
  struct UndoStream *undo_stream;
  struct UndoStream *redo_stream;
  int diff_count;
  int diff_saved;
  unsigned enabled : 1;		/* is undo enabled? */
  int size_limit;		/* limit for undo stream size */
};

Undo *undo_new(struct Sprite *sprite);
void undo_free(Undo *undo);

void undo_enable(Undo *undo);
void undo_disable(Undo *undo);

bool undo_is_enabled(Undo *undo);
bool undo_is_disabled(Undo *undo);

bool undo_can_undo(Undo *undo);
bool undo_can_redo(Undo *undo);

void undo_undo(Undo *undo);
void undo_redo(Undo *undo);

const char *undo_get_next_undo_label(Undo *undo);
const char *undo_get_next_redo_label(Undo *undo);

void undo_open(Undo *undo);
void undo_close(Undo *undo);
void undo_data(Undo *undo, GfxObj *gfxobj, void *data, int size);
void undo_image(Undo *undo, struct Image *image, int x, int y, int w, int h);
void undo_flip(Undo *undo, struct Image *image, int x1, int y1, int x2, int y2, int horz);
void undo_dirty(Undo *undo, struct Dirty *dirty);
void undo_add_image(Undo *undo, struct Stock *stock, struct Image *image);
void undo_remove_image(Undo *undo, struct Stock *stock, struct Image *image);
void undo_replace_image(Undo *undo, struct Stock *stock, int index);
void undo_add_cel(Undo *undo, struct Layer *layer, struct Cel *cel);
void undo_remove_cel(Undo *undo, struct Layer *layer, struct Cel *cel);
void undo_add_layer(Undo *undo, struct Layer *set, struct Layer *layer);
void undo_remove_layer(Undo *undo, struct Layer *layer);
void undo_move_layer(Undo *undo, struct Layer *layer);
void undo_set_layer(Undo *undo, struct Sprite *sprite);
void undo_set_mask(Undo *undo, struct Sprite *sprite);
void undo_set_frames(Undo *undo, struct Sprite *sprite);

#define undo_int(undo, gfxobj, value_address) \
  undo_data((undo), (gfxobj), (void *)(value_address), sizeof(int))

#define undo_double(undo, gfxobj, value_address) \
  undo_data((undo), (gfxobj), (void *)(value_address), sizeof(double))

#define undo_string(undo, gfxobj, string) \
  undo_data((undo), (gfxobj), (void *)(string), strlen(string)+1)

#endif /* RASTER_UNDO_H */
