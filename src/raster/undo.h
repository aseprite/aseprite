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

#ifndef RASTER_UNDO_H_INCLUDED
#define RASTER_UNDO_H_INCLUDED

#include <string.h>		/* for strlen */
#include <vector>

#include "raster/gfxobj.h"
#include "ase_exception.h"

class Cel;
class Image;
class Layer;
class Mask;
class Sprite;
class Stock;

struct Dirty;
struct UndoStream;

class undo_exception : public ase_exception
{
public:
  undo_exception(const char* msg) throw() : ase_exception(msg) { }
};

class Undo : public GfxObj
{
public:
  Sprite* sprite;		/* related sprite */
  UndoStream *undo_stream;
  UndoStream *redo_stream;
  int diff_count;
  int diff_saved;
  unsigned enabled : 1;		/* is undo enabled? */
  const char *label;		/* current label to be applied to all
				   next undo operations */

  Undo(Sprite* sprite);
  virtual ~Undo();

};

Undo* undo_new(Sprite* sprite);
void undo_free(Undo* undo);

int undo_get_memsize(const Undo* undo);

void undo_enable(Undo* undo);
void undo_disable(Undo* undo);

bool undo_is_enabled(const Undo* undo);
bool undo_is_disabled(const Undo* undo);

bool undo_can_undo(const Undo* undo);
bool undo_can_redo(const Undo* undo);

void undo_do_undo(Undo* undo);
void undo_do_redo(Undo* undo);

void undo_clear_redo(Undo* undo);

void undo_set_label(Undo* undo, const char *label);
const char* undo_get_next_undo_label(const Undo* undo);
const char* undo_get_next_redo_label(const Undo* undo);

void undo_open(Undo* undo);
void undo_close(Undo* undo);
void undo_data(Undo* undo, GfxObj *gfxobj, void *data, int size);
void undo_image(Undo* undo, Image *image, int x, int y, int w, int h);
void undo_flip(Undo* undo, Image *image, int x1, int y1, int x2, int y2, bool horz);
void undo_dirty(Undo* undo, Dirty *dirty);
void undo_add_image(Undo* undo, Stock *stock, int image_index);
void undo_remove_image(Undo* undo, Stock *stock, int image_index);
void undo_replace_image(Undo* undo, Stock *stock, int image_index);
void undo_set_layer_name(Undo* undo, Layer *layer);
void undo_add_cel(Undo* undo, Layer *layer, Cel *cel);
void undo_remove_cel(Undo* undo, Layer *layer, Cel *cel);
void undo_add_layer(Undo* undo, Layer *set, Layer *layer);
void undo_remove_layer(Undo* undo, Layer *layer);
void undo_move_layer(Undo* undo, Layer *layer);
void undo_set_layer(Undo* undo, Sprite* sprite);
void undo_add_palette(Undo* undo, Sprite* sprite, Palette* palette);
void undo_remove_palette(Undo* undo, Sprite* sprite, Palette* palette);
void undo_remap_palette(Undo* undo, Sprite* sprite, int frame_from, int frame_to,
			const std::vector<int>& mapping);
void undo_set_mask(Undo* undo, Sprite* sprite);
void undo_set_imgtype(Undo* undo, Sprite* sprite);
void undo_set_size(Undo* undo, Sprite* sprite);
void undo_set_frame(Undo* undo, Sprite* sprite);
void undo_set_frames(Undo* undo, Sprite* sprite);
void undo_set_frlen(Undo* undo, Sprite* sprite, int frame);

#define undo_int(undo, gfxobj, value_address) \
  undo_data((undo), (gfxobj), (void *)(value_address), sizeof(int))

#define undo_double(undo, gfxobj, value_address) \
  undo_data((undo), (gfxobj), (void *)(value_address), sizeof(double))

#endif
