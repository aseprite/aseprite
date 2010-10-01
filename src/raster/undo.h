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
class Palette;
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
  Undo(Sprite* sprite);
  virtual ~Undo();

  bool isEnabled() const;
  void setEnabled(bool state);

  bool canUndo() const;
  bool canRedo() const;

  void doUndo();
  void doRedo();

  void clearRedo();

  const char* getLabel();
  void setLabel(const char* label);

  const char* getNextUndoLabel() const;
  const char* getNextRedoLabel() const;

  bool isSavedState() const;
  void markSavedState();

  void undo_open();
  void undo_close();
  void undo_data(GfxObj *gfxobj, void *data, int size);
  void undo_image(Image *image, int x, int y, int w, int h);
  void undo_flip(Image *image, int x1, int y1, int x2, int y2, bool horz);
  void undo_dirty(Dirty *dirty);
  void undo_add_image(Stock *stock, int image_index);
  void undo_remove_image(Stock *stock, int image_index);
  void undo_replace_image(Stock *stock, int image_index);
  void undo_set_layer_name(Layer *layer);
  void undo_add_cel(Layer *layer, Cel *cel);
  void undo_remove_cel(Layer *layer, Cel *cel);
  void undo_add_layer(Layer *set, Layer *layer);
  void undo_remove_layer(Layer *layer);
  void undo_move_layer(Layer *layer);
  void undo_set_layer(Sprite* sprite);
  void undo_add_palette(Sprite* sprite, Palette* palette);
  void undo_remove_palette(Sprite* sprite, Palette* palette);
  void undo_remap_palette(Sprite* sprite, int frame_from, int frame_to,
			  const std::vector<int>& mapping);
  void undo_set_mask(Sprite* sprite);
  void undo_set_imgtype(Sprite* sprite);
  void undo_set_size(Sprite* sprite);
  void undo_set_frame(Sprite* sprite);
  void undo_set_frames(Sprite* sprite);
  void undo_set_frlen(Sprite* sprite, int frame);

  void undo_int(GfxObj* gfxobj, int* value_address) {
    undo_data(gfxobj, (void*)(value_address), sizeof(int));
  }

  void undo_double(GfxObj* gfxobj, double* value_address) {
    undo_data(gfxobj, (void*)(value_address), sizeof(double));
  }

private:
  void runUndo(int state);
  void discardTail();
  void updateUndo();

  Sprite* m_sprite;		// Related sprite
  UndoStream* m_undoStream;
  UndoStream* m_redoStream;
  int m_diffCount;
  int m_diffSaved;
  bool m_enabled;		// Is undo enabled?
  const char* m_label;		// Current label to be applied to all next undo operations.
};

#endif
