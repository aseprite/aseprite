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

#ifndef UNDO_UNDO_HISTORY_H_INCLUDED
#define UNDO_UNDO_HISTORY_H_INCLUDED

#include "base/exception.h"

#include <vector>

class Cel;
class Dirty;
class Document;
class GfxObj;
class Image;
class Layer;
class Mask;
class ObjectsContainer;
class Palette;
class Sprite;
class Stock;
class UndoStream;

class UndoException : public base::Exception
{
public:
  UndoException(const char* msg) throw() : base::Exception(msg) { }
};

class UndoHistory
{
public:
  UndoHistory(ObjectsContainer* objects);
  virtual ~UndoHistory();

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
  void undo_data(void* object, void* fieldAddress, int fieldSize);
  void undo_image(Image *image, int x, int y, int w, int h);
  void undo_flip(Image *image, int x1, int y1, int x2, int y2, bool horz);
  void undo_dirty(Image* image, Dirty *dirty);
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
  void undo_set_palette_colors(Sprite* sprite, Palette* palette, int from, int to);
  void undo_remap_palette(Sprite* sprite, int frame_from, int frame_to,
			  const std::vector<int>& mapping);
  void undo_set_mask(Document* document);
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

  ObjectsContainer* getObjects() const { return m_objects; }

private:
  void runUndo(int state);
  void discardTail();
  void updateUndo();

  ObjectsContainer* m_objects;	// Container of objects to insert & retrieve objects by ID
  UndoStream* m_undoStream;
  UndoStream* m_redoStream;
  int m_diffCount;
  int m_diffSaved;
  bool m_enabled;		// Is undo enabled?
  const char* m_label;		// Current label to be applied to all next undo operations.
};

#endif
