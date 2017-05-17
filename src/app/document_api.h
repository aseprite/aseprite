// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOCUMENT_API_H_INCLUDED
#define APP_DOCUMENT_API_H_INCLUDED
#pragma once

#include "doc/algorithm/flip_type.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "gfx/rect.h"

namespace doc {
  class Cel;
  class Image;
  class Layer;
  class LayerGroup;
  class LayerImage;
  class Mask;
  class Palette;
  class Sprite;
}

namespace app {
  class Document;
  class Transaction;

  using namespace doc;

  // Old high-level API. The new way is to create Cmds and add them
  // directly to the transaction.
  //
  // TODO refactor this class in several Cmd, don't make it bigger
  class DocumentApi {
  public:
    DocumentApi(Document* document, Transaction& transaction);

    // Sprite API
    void setSpriteSize(Sprite* sprite, int w, int h);
    void setSpriteTransparentColor(Sprite* sprite, color_t maskColor);
    void cropSprite(Sprite* sprite, const gfx::Rect& bounds);
    void trimSprite(Sprite* sprite);

    // Frames API
    void addFrame(Sprite* sprite, frame_t newFrame);
    void addEmptyFrame(Sprite* sprite, frame_t newFrame);
    void addEmptyFramesTo(Sprite* sprite, frame_t newFrame);
    void copyFrame(Sprite* sprite, frame_t fromFrame, frame_t newFrame);
    void removeFrame(Sprite* sprite, frame_t frame);
    void setTotalFrames(Sprite* sprite, frame_t frames);
    void setFrameDuration(Sprite* sprite, frame_t frame, int msecs);
    void setFrameRangeDuration(Sprite* sprite, frame_t from, frame_t to, int msecs);
    void moveFrame(Sprite* sprite, frame_t frame, frame_t beforeFrame);

    // Cels API
    void addCel(LayerImage* layer, Cel* cel);
    Cel* addCel(LayerImage* layer, frame_t frameNumber, const ImageRef& image);
    void clearCel(LayerImage* layer, frame_t frame);
    void clearCel(Cel* cel);
    void setCelPosition(Sprite* sprite, Cel* cel, int x, int y);
    void setCelOpacity(Sprite* sprite, Cel* cel, int newOpacity);
    void moveCel(
      LayerImage* srcLayer, frame_t srcFrame,
      LayerImage* dstLayer, frame_t dstFrame);
    void copyCel(
      LayerImage* srcLayer, frame_t srcFrame,
      LayerImage* dstLayer, frame_t dstFrame);
    void copyCel(
      LayerImage* srcLayer, frame_t srcFrame,
      LayerImage* dstLayer, frame_t dstFrame, bool continuous);
    void swapCel(
      LayerImage* layer, frame_t frame1, frame_t frame2);

    // Layers API
    LayerImage* newLayer(LayerGroup* parent, const std::string& name);
    LayerGroup* newGroup(LayerGroup* parent, const std::string& name);
    void addLayer(LayerGroup* parent, Layer* newLayer, Layer* afterThis);
    void removeLayer(Layer* layer);
    void restackLayerAfter(Layer* layer, LayerGroup* parent, Layer* afterThis);
    void restackLayerBefore(Layer* layer, LayerGroup* parent, Layer* beforeThis);
    void backgroundFromLayer(Layer* layer);
    void layerFromBackground(Layer* layer);
    void flattenLayers(Sprite* sprite);
    Layer* duplicateLayerAfter(Layer* sourceLayer, LayerGroup* parent, Layer* afterLayer);
    Layer* duplicateLayerBefore(Layer* sourceLayer, LayerGroup* parent, Layer* beforeLayer);

    // Images API
    void replaceImage(Sprite* sprite, const ImageRef& oldImage, const ImageRef& newImage);

    // Image API
    void flipImage(Image* image, const gfx::Rect& bounds, doc::algorithm::FlipType flipType);
    void flipImageWithMask(Layer* layer, Image* image, doc::algorithm::FlipType flipType);

    // Mask API
    void copyToCurrentMask(Mask* mask);
    void setMaskPosition(int x, int y);

    // Palette API
    void setPalette(Sprite* sprite, frame_t frame, const Palette* newPalette);

  private:
    void setCelFramePosition(Cel* cel, frame_t frame);
    void moveFrameLayer(Layer* layer, frame_t frame, frame_t beforeFrame);
    void adjustFrameTags(Sprite* sprite, frame_t frame, frame_t delta, bool between);

    Document* m_document;
    Transaction& m_transaction;
  };

} // namespace app

#endif
