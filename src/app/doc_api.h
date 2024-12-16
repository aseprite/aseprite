// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOC_API_H_INCLUDED
#define APP_DOC_API_H_INCLUDED
#pragma once

#include "app/drop_frame_place.h"
#include "app/tags_handling.h"
#include "doc/algorithm/flip_type.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/image_ref.h"
#include "gfx/rect.h"

#include <map>

namespace doc {
class Cel;
class CelData;
class Image;
class Layer;
class LayerGroup;
class LayerImage;
class Mask;
class Palette;
class Sprite;
} // namespace doc

namespace app {
class Doc;
class Transaction;

using namespace doc;

// High-level API to modify a document adding undo information, i.e.
// adding new "Cmd"s in the given transaction.
//
// In the past there were plans to remove this class, but it got
// bigger and bigger and there wasn't a real replacement. Some
// attempts to avoid adding new member functions in DocApi are:
//
// 1. Creating new standalone functions like the ones from
//    doc_range_ops.h, or util/cel_ops.h
// 2. Creating a new Command/QuickCommand, although that only makes
//    sense if we want to expose the functionality to the end-user,
//    and the number of parameters is quite small.
//
// Sometimes complex sprite changes require storing some special
// state until the transaction ends, like HandleLinkedCels and the
// m_linkedCels to map linked cels in the source sprite/layer and
// replicate them in the destination sprite/layer. This requires
// such coordination with moveCel()/copyCel() methods that it would
// be quite hard to replicate that in a standalone function.
//
class DocApi {
public:
  DocApi(Doc* document, Transaction& transaction);

  // Sprite API
  void setSpriteSize(Sprite* sprite, int w, int h);
  void setSpriteTransparentColor(Sprite* sprite, color_t maskColor);
  void cropSprite(Sprite* sprite, const gfx::Rect& bounds, const bool trimOutside = false);
  void trimSprite(Sprite* sprite, const bool byGrid);

  // Frames API
  void addFrame(Sprite* sprite, frame_t newFrame);
  void addEmptyFrame(Sprite* sprite, frame_t newFrame);
  void addEmptyFramesTo(Sprite* sprite, frame_t newFrame);
  void copyFrame(Sprite* sprite,
                 frame_t fromFrame,
                 const frame_t newFrame,
                 const DropFramePlace dropFramePlace,
                 const TagsHandling tagsHandling);
  void removeFrame(Sprite* sprite, frame_t frame);
  void setTotalFrames(Sprite* sprite, frame_t frames);
  void setFrameDuration(Sprite* sprite, frame_t frame, int msecs);
  void setFrameRangeDuration(Sprite* sprite, frame_t from, frame_t to, int msecs);
  void moveFrame(Sprite* sprite,
                 const frame_t frame,
                 frame_t targetFrame,
                 const DropFramePlace dropFramePlace,
                 const TagsHandling tagsHandling);

  // Cels API
  void addCel(LayerImage* layer, Cel* cel);
  Cel* addCel(LayerImage* layer, frame_t frameNumber, const ImageRef& image);
  void clearCel(Layer* layer, frame_t frame);
  void clearCel(Cel* cel);
  void clearCelAndAllLinks(Cel* cel);
  void setCelPosition(Sprite* sprite, Cel* cel, int x, int y);
  void setCelOpacity(Sprite* sprite, Cel* cel, int newOpacity);
  void moveCel(LayerImage* srcLayer, frame_t srcFrame, LayerImage* dstLayer, frame_t dstFrame);
  void copyCel(LayerImage* srcLayer,
               frame_t srcFrame,
               LayerImage* dstLayer,
               frame_t dstFrame,
               const bool* forceContinuous = nullptr);
  void swapCel(LayerImage* layer, frame_t frame1, frame_t frame2);

  // Layers API
  LayerImage* newLayer(LayerGroup* parent, const std::string& name);
  LayerGroup* newGroup(LayerGroup* parent, const std::string& name);
  void addLayer(LayerGroup* parent, Layer* newLayer, Layer* afterThis);
  void removeLayer(Layer* layer);
  void restackLayerAfter(Layer* layer, LayerGroup* parent, Layer* afterThis);
  void restackLayerBefore(Layer* layer, LayerGroup* parent, Layer* beforeThis);
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
  void cropImageLayer(LayerImage* layer, const gfx::Rect& bounds, const bool trimOutside);
  bool cropCel(LayerImage* layer, Cel* cel, const gfx::Rect& bounds, const bool trimOutside);
  void setCelFramePosition(Cel* cel, frame_t frame);
  void moveFrameLayer(Layer* layer, frame_t frame, frame_t beforeFrame);
  void adjustTags(Sprite* sprite,
                  const frame_t frame,
                  const frame_t delta,
                  const DropFramePlace dropFramePlace,
                  const TagsHandling tagsHandling);

  class HandleLinkedCels {
  public:
    HandleLinkedCels(DocApi& api,
                     doc::LayerImage* srcLayer,
                     const doc::frame_t srcFrame,
                     doc::LayerImage* dstLayer,
                     const doc::frame_t dstFrame);
    ~HandleLinkedCels();
    bool linkWasCreated() { return m_created; }

  private:
    DocApi& m_api;
    doc::ObjectId m_srcDataId;
    doc::Layer* m_dstLayer;
    doc::frame_t m_dstFrame;
    bool m_created;
  };

  bool copyFromLinkedCels(Cel** srcCel, doc::ObjectId& srcDataId);

  Doc* m_document;
  Transaction& m_transaction;

  // Map used in copyCel() to re-create the original set of linked
  // cels from the src layers when we copy a block of cels.
  // map: ObjectId of CelData -> Cel*
  std::map<doc::ObjectId, doc::Cel*> m_linkedCels;
};

} // namespace app

#endif
