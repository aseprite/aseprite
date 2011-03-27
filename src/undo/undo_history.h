// ASEPRITE Undo Library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UNDO_UNDO_HISTORY_H_INCLUDED
#define UNDO_UNDO_HISTORY_H_INCLUDED

#include "undo/modification.h"
#include "undo/undoers_collector.h"

#include <vector>

// TODO Remove this (it is here only for backward compatibility)
class Cel;
class Dirty;
class Document;
class GfxObj;
class Image;
class Layer;
class Mask;
class Palette;
class Sprite;
class Stock;

namespace undo {

class ObjectsContainer;
class UndoersStack;

class UndoHistory : public UndoersCollector
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

  // Current label for next added Undoers.
  const char* getLabel();
  void setLabel(const char* label);

  // Change the "modify saved status" flag to be assigned for next
  // added items. When it is activated means that each added Undoer
  // modifies the "saved status" of the document.
  Modification getModification();
  void setModification(Modification mod);

  const char* getNextUndoLabel() const;
  const char* getNextRedoLabel() const;

  bool isSavedState() const;
  void markSavedState();

  ObjectsContainer* getObjects() const { return m_objects; }

  // UndoersCollector interface
  void pushUndoer(Undoer* undoer);

  // Backward compatibility methods
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
  void undo_remap_palette(Sprite* sprite, int frame_from, int frame_to, const std::vector<uint8_t>& mapping);
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

private:
  enum Direction { UndoDirection, RedoDirection };

  void runUndo(Direction direction);
  void discardTail();
  void updateUndo();

  ObjectsContainer* m_objects;	// Container of objects to insert & retrieve objects by ID
  UndoersStack* m_undoers;
  UndoersStack* m_redoers;
  int m_groupLevel;
  int m_diffCount;
  int m_diffSaved;
  bool m_enabled;		// Is undo enabled?
  const char* m_label;		// Current label to be applied to all next undo operations.
  Modification m_modification;  // Current label to be applied to all next undo operations.
};

} // namespace undo

#endif	// UNDO_UNDO_HISTORY_H_INCLUDED
