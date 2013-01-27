/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#ifndef UNDO_TRANSACTION_H_INCLUDED
#define UNDO_TRANSACTION_H_INCLUDED

#include "gfx/rect.h"
#include "raster/algorithm/flip_type.h"
#include "raster/dithering_method.h"
#include "raster/frame_number.h"
#include "raster/pixel_format.h"
#include "undo/modification.h"

class Cel;
class Document;
class DocumentUndo;
class Image;
class Layer;
class LayerImage;
class Mask;
class Sprite;

namespace undo {
  class ObjectsContainer;
  class Undoer;
}

// High-level class to group a set of operations to modify the
// document atomically, adding information in the undo history to
// rollback the whole operation if something fails (with an
// exceptions) in the middle of the procedure.
//
// You have to wrap every call to an undo transaction with a
// DocumentWriter. The preferred usage is as the following:
//
// {
//   DocumentWriter documentWriter(document);
//   UndoTransaction undoTransaction(documentWriter, "My big operation");
//   ...
//   undoTransaction.commit();
// }
//
class UndoTransaction
{
public:

  // Starts a undoable sequence of operations in a transaction that
  // can be committed or rollbacked.  All the operations will be
  // grouped in the sprite's undo as an atomic operation.
  UndoTransaction(Document* document, const char* label, undo::Modification mod = undo::ModifyDocument);
  virtual ~UndoTransaction();

  inline Sprite* getSprite() const { return m_sprite;  }
  inline bool isEnabled() const { return m_enabledFlag; }

  // This must be called to commit all the changes, so the undo will
  // be finally added in the sprite.
  //
  // If you don't use this routine, all the changes will be discarded
  // (if the sprite's undo was enabled when the UndoTransaction was
  // created).
  void commit();

  void pushUndoer(undo::Undoer* undoer);
  undo::ObjectsContainer* getObjects() const;

  // for sprite
  void setNumberOfFrames(FrameNumber frames);
  void setCurrentFrame(FrameNumber frame);
  void setCurrentLayer(Layer* layer);
  void setSpriteSize(int w, int h);
  void cropSprite(const gfx::Rect& bounds, int bgcolor);
  void trimSprite(int bgcolor);
  void setPixelFormat(PixelFormat newFormat, DitheringMethod dithering_method);

  // for images in stock
  int addImageInStock(Image* image);
  void removeImageFromStock(int image_index);
  void replaceStockImage(int image_index, Image* new_image);

  // for layers
  LayerImage* newLayer();
  void removeLayer(Layer* layer);
  void restackLayerAfter(Layer* layer, Layer* afterThis);
  void cropLayer(Layer* layer, int x, int y, int w, int h, int bgcolor);
  void displaceLayers(Layer* layer, int dx, int dy);
  void backgroundFromLayer(LayerImage* layer, int bgcolor);
  void layerFromBackground();
  void flattenLayers(int bgcolor);

  // for frames
  void newFrame();
  void newFrameForLayer(Layer* layer, FrameNumber frame);
  void removeFrame(FrameNumber frame);
  void removeFrameOfLayer(Layer* layer, FrameNumber frame);
  void copyPreviousFrame(Layer* layer, FrameNumber frame);
  void setFrameDuration(FrameNumber frame, int msecs);
  void setConstantFrameRate(int msecs);
  void moveFrameBefore(FrameNumber frame, FrameNumber beforeFrame);
  void moveFrameBeforeLayer(Layer* layer, FrameNumber frame, FrameNumber beforeFrame);

  // for cels
  void addCel(LayerImage* layer, Cel* cel);
  void removeCel(LayerImage* layer, Cel* cel);
  void setCelFramePosition(Cel* cel, FrameNumber frame);
  void setCelPosition(Cel* cel, int x, int y);
  Cel* getCurrentCel();
  void cropCel(Cel* cel, int x, int y, int w, int h, int bgcolor);

  // for image
  Image* getCelImage(Cel* cel);
  void clearMask(int bgcolor);
  void flipImage(Image* image, const gfx::Rect& bounds, raster::algorithm::FlipType flipType);
  void flipImageWithMask(Image* image, const Mask* mask, raster::algorithm::FlipType flipType, int bgcolor);
  void pasteImage(const Image* src_image, int x, int y, int opacity);

  // for mask
  void copyToCurrentMask(Mask* mask);
  void setMaskPosition(int x, int y);
  void deselectMask();

private:
  void configureLayerAsBackground(LayerImage* layer);
  void closeUndoGroup();
  void rollback();

  Document* m_document;
  Sprite* m_sprite;
  DocumentUndo* m_undo;
  bool m_closed;
  bool m_committed;
  bool m_enabledFlag;
  const char* m_label;
  undo::Modification m_modification;
};

#endif
