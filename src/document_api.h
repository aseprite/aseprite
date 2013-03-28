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

#ifndef DOCUMENT_API_H_INCLUDED
#define DOCUMENT_API_H_INCLUDED

#include "gfx/rect.h"
#include "raster/algorithm/flip_type.h"
#include "raster/dithering_method.h"
#include "raster/frame_number.h"
#include "raster/pixel_format.h"

class Cel;
class Document;
class Image;
class Layer;
class LayerFolder;
class LayerImage;
class Mask;
class Sprite;

namespace undo {
  class UndoersCollector;
  class ObjectsContainer;
}

// High-level API to modify a document in an undoable and observable way.
// Each method of this class take care of three important things:
// 1) Do the given action with low-level operations (raster
//    namespace mainly),
// 2) Add undoers so the action can be undone in the
//    future (undoers namespace mainly),
// 3) Notify observers of the document that a change
//    was made (call DocumentObserver methods).
class DocumentApi {
public:
  DocumentApi(Document* document, undo::UndoersCollector* undoers);

  // Sprite API
  void setSpriteSize(Sprite* sprite, int w, int h);
  void cropSprite(Sprite* sprite, const gfx::Rect& bounds, int bgcolor);
  void trimSprite(Sprite* sprite, int bgcolor);
  void setPixelFormat(Sprite* sprite, PixelFormat newFormat, DitheringMethod dithering_method);

  // Frames API
  void addFrame(Sprite* sprite, FrameNumber newFrame);
  void removeFrame(Sprite* sprite, FrameNumber frame);
  void setTotalFrames(Sprite* sprite, FrameNumber frames);
  void setFrameDuration(Sprite* sprite, FrameNumber frame, int msecs);
  void setConstantFrameRate(Sprite* sprite, int msecs);
  void moveFrameBefore(Sprite* sprite, FrameNumber frame, FrameNumber beforeFrame);

  // Cels API
  void addCel(LayerImage* layer, Cel* cel);
  void removeCel(LayerImage* layer, Cel* cel);
  void setCelFramePosition(Sprite* sprite, Cel* cel, FrameNumber frame);
  void setCelPosition(Sprite* sprite, Cel* cel, int x, int y);
  void cropCel(Sprite* sprite, Cel* cel, int x, int y, int w, int h, int bgcolor);

  // Layers API
  LayerImage* newLayer(Sprite* sprite);
  LayerFolder* newLayerFolder(Sprite* sprite);
  void addLayer(LayerFolder* folder, Layer* newLayer, Layer* afterThis);
  void removeLayer(Layer* layer);
  void restackLayerAfter(Layer* layer, Layer* afterThis);
  void cropLayer(Layer* layer, int x, int y, int w, int h, int bgcolor);
  void displaceLayers(Layer* layer, int dx, int dy);
  void backgroundFromLayer(LayerImage* layer, int bgcolor);
  void layerFromBackground(Layer* layer);
  void flattenLayers(Sprite* sprite, int bgcolor);

  // Images stock API
  int addImageInStock(Sprite* sprite, Image* image);
  void removeImageFromStock(Sprite* sprite, int imageIndex);
  void replaceStockImage(Sprite* sprite, int imageIndex, Image* newImage);

  // Image API
  Image* getCelImage(Sprite* sprite, Cel* cel);
  void clearMask(Layer* layer, Cel* cel, int bgcolor);
  void flipImage(Image* image, const gfx::Rect& bounds, raster::algorithm::FlipType flipType);
  void flipImageWithMask(Image* image, const Mask* mask, raster::algorithm::FlipType flipType, int bgcolor);
  void pasteImage(Sprite* sprite, Cel* cel, const Image* src_image, int x, int y, int opacity);

  // Mask API
  void copyToCurrentMask(Mask* mask);
  void setMaskPosition(int x, int y);
  void deselectMask();

private:
  undo::ObjectsContainer* getObjects() const;
  void addFrameForLayer(Layer* layer, FrameNumber frame);
  void removeFrameOfLayer(Layer* layer, FrameNumber frame);
  void copyPreviousFrame(Layer* layer, FrameNumber frame);
  void moveFrameBeforeLayer(Layer* layer, FrameNumber frame, FrameNumber beforeFrame);
  void configureLayerAsBackground(LayerImage* layer);

  Document* m_document;
  undo::UndoersCollector* m_undoers;
};

#endif
