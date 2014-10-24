/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#ifndef APP_DOCUMENT_API_H_INCLUDED
#define APP_DOCUMENT_API_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include "doc/algorithm/flip_type.h"
#include "doc/color.h"
#include "doc/dithering_method.h"
#include "doc/frame_number.h"
#include "doc/pixel_format.h"

namespace doc {
  class Cel;
  class Image;
  class Layer;
  class LayerFolder;
  class LayerImage;
  class Mask;
  class Palette;
  class Sprite;
}

namespace undo {
  class UndoersCollector;
  class ObjectsContainer;
}

namespace app {
  class Document;

  using namespace doc;

  // High-level API to modify a document in an undoable and observable way.
  // Each method of this class take care of three important things:
  // 1) Do the given action with low-level operations (doc namespace mainly),
  // 2) Add undoers so the action can be undone in the
  //    future (undoers namespace mainly),
  // 3) Notify observers of the document that a change
  //    was made (call DocumentObserver methods).
  class DocumentApi {
  public:
    DocumentApi(Document* document, undo::UndoersCollector* undoers);

    // Sprite API
    void setSpriteSize(Sprite* sprite, int w, int h);
    void setSpriteTransparentColor(Sprite* sprite, color_t maskColor);
    void cropSprite(Sprite* sprite, const gfx::Rect& bounds);
    void trimSprite(Sprite* sprite);
    void setPixelFormat(Sprite* sprite, PixelFormat newFormat, DitheringMethod dithering_method);

    // Frames API
    void addFrame(Sprite* sprite, FrameNumber newFrame);
    void addEmptyFrame(Sprite* sprite, FrameNumber newFrame);
    void addEmptyFramesTo(Sprite* sprite, FrameNumber newFrame);
    void copyFrame(Sprite* sprite, FrameNumber fromFrame, FrameNumber newFrame);
    void removeFrame(Sprite* sprite, FrameNumber frame);
    void setTotalFrames(Sprite* sprite, FrameNumber frames);
    void setFrameDuration(Sprite* sprite, FrameNumber frame, int msecs);
    void setFrameRangeDuration(Sprite* sprite, FrameNumber from, FrameNumber to, int msecs);
    void moveFrame(Sprite* sprite, FrameNumber frame, FrameNumber beforeFrame);

    // Cels API
    void addCel(LayerImage* layer, Cel* cel);
    void clearCel(LayerImage* layer, FrameNumber frame);
    void clearCel(Cel* cel);
    void setCelPosition(Sprite* sprite, Cel* cel, int x, int y);
    void setCelOpacity(Sprite* sprite, Cel* cel, int newOpacity);
    void cropCel(Sprite* sprite, Cel* cel, int x, int y, int w, int h);
    void moveCel(
      LayerImage* srcLayer, FrameNumber srcFrame,
      LayerImage* dstLayer, FrameNumber dstFrame);
    void copyCel(
      LayerImage* srcLayer, FrameNumber srcFrame,
      LayerImage* dstLayer, FrameNumber dstFrame);
    void swapCel(
      LayerImage* layer, FrameNumber frame1, FrameNumber frame2);

    // Layers API
    LayerImage* newLayer(Sprite* sprite);
    LayerFolder* newLayerFolder(Sprite* sprite);
    void addLayer(LayerFolder* folder, Layer* newLayer, Layer* afterThis);
    void removeLayer(Layer* layer);
    void restackLayerAfter(Layer* layer, Layer* afterThis);
    void restackLayerBefore(Layer* layer, Layer* beforeThis);
    void cropLayer(Layer* layer, int x, int y, int w, int h);
    void displaceLayers(Layer* layer, int dx, int dy);
    void backgroundFromLayer(LayerImage* layer);
    void layerFromBackground(Layer* layer);
    void flattenLayers(Sprite* sprite);
    void duplicateLayerAfter(Layer* sourceLayer, Layer* afterLayer);
    void duplicateLayerBefore(Layer* sourceLayer, Layer* beforeLayer);

    // Images stock API
    Cel* addImage(LayerImage* layer, FrameNumber frameNumber, Image* image);
    int addImageInStock(Sprite* sprite, Image* image);
    void removeImageFromStock(Sprite* sprite, int imageIndex);
    void replaceStockImage(Sprite* sprite, int imageIndex, Image* newImage);

    // Image API
    void clearImage(Image* image, color_t bgcolor);
    void clearMask(Cel* cel);
    void flipImage(Image* image, const gfx::Rect& bounds, doc::algorithm::FlipType flipType);
    void flipImageWithMask(Layer* layer, Image* image, const Mask* mask, doc::algorithm::FlipType flipType);
    void pasteImage(Sprite* sprite, Cel* cel, const Image* src_image, int x, int y, int opacity);

    // Mask API
    void copyToCurrentMask(Mask* mask);
    void setMaskPosition(int x, int y);
    void deselectMask();

    // Palette API
    void setPalette(Sprite* sprite, FrameNumber frame, Palette* newPalette);

  private:
    undo::ObjectsContainer* getObjects() const;
    void removeCel(Cel* cel);
    void setCelFramePosition(LayerImage* layer, Cel* cel, FrameNumber frame);
    void displaceFrames(Layer* layer, FrameNumber frame);
    void copyFrameForLayer(Layer* layer, FrameNumber fromFrame, FrameNumber frame);
    void removeFrameOfLayer(Layer* layer, FrameNumber frame);
    void moveFrameLayer(Layer* layer, FrameNumber frame, FrameNumber beforeFrame);
    void configureLayerAsBackground(LayerImage* layer);
    bool undoEnabled();

    doc::color_t bgColor();
    doc::color_t bgColor(Layer* layer);

    Document* m_document;
    undo::UndoersCollector* m_undoers;
  };

} // namespace app

#endif
