// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document_api.h"

#include "app/cmd/add_cel.h"
#include "app/cmd/add_frame.h"
#include "app/cmd/add_layer.h"
#include "app/cmd/background_from_layer.h"
#include "app/cmd/clear_cel.h"
#include "app/cmd/clear_image.h"
#include "app/cmd/copy_cel.h"
#include "app/cmd/copy_frame.h"
#include "app/cmd/flatten_layers.h"
#include "app/cmd/flip_image.h"
#include "app/cmd/layer_from_background.h"
#include "app/cmd/move_cel.h"
#include "app/cmd/move_layer.h"
#include "app/cmd/remove_cel.h"
#include "app/cmd/remove_frame.h"
#include "app/cmd/remove_frame_tag.h"
#include "app/cmd/remove_layer.h"
#include "app/cmd/replace_image.h"
#include "app/cmd/set_cel_frame.h"
#include "app/cmd/set_cel_opacity.h"
#include "app/cmd/set_cel_position.h"
#include "app/cmd/set_frame_duration.h"
#include "app/cmd/set_frame_tag_range.h"
#include "app/cmd/set_mask.h"
#include "app/cmd/set_mask_position.h"
#include "app/cmd/set_palette.h"
#include "app/cmd/set_pixel_format.h"
#include "app/cmd/set_sprite_size.h"
#include "app/cmd/set_total_frames.h"
#include "app/cmd/set_transparent_color.h"
#include "app/color_target.h"
#include "app/color_utils.h"
#include "app/document.h"
#include "app/document_undo.h"
#include "app/transaction.h"
#include "base/unique_ptr.h"
#include "doc/algorithm/flip_image.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/context.h"
#include "doc/frame_tag.h"
#include "doc/frame_tags.h"
#include "doc/mask.h"
#include "render/quantization.h"
#include "render/render.h"

#include <set>

namespace app {

DocumentApi::DocumentApi(Document* document, Transaction& transaction)
  : m_document(document)
  , m_transaction(transaction)
{
}

void DocumentApi::setSpriteSize(Sprite* sprite, int w, int h)
{
  m_transaction.execute(new cmd::SetSpriteSize(sprite, w, h));
}

void DocumentApi::setSpriteTransparentColor(Sprite* sprite, color_t maskColor)
{
  m_transaction.execute(new cmd::SetTransparentColor(sprite, maskColor));
}

void DocumentApi::cropSprite(Sprite* sprite, const gfx::Rect& bounds)
{
  setSpriteSize(sprite, bounds.w, bounds.h);

  app::Document* doc = static_cast<app::Document*>(sprite->document());
  std::vector<Layer*> layers;
  sprite->getLayersList(layers);
  for (Layer* layer : layers) {
    if (!layer->isImage())
      continue;

    std::set<ObjectId> visited;
    CelIterator it = ((LayerImage*)layer)->getCelBegin();
    CelIterator end = ((LayerImage*)layer)->getCelEnd();
    for (; it != end; ++it) {
      Cel* cel = *it;
      if (visited.find(cel->data()->id()) != visited.end())
        continue;
      visited.insert(cel->data()->id());

      if (layer->isBackground()) {
        Image* image = cel->image();
        if (image && !cel->link()) {
          ASSERT(cel->x() == 0);
          ASSERT(cel->y() == 0);

          // Create the new image through a crop
          ImageRef new_image(
            crop_image(image,
              bounds.x, bounds.y,
              bounds.w, bounds.h,
              doc->bgColor(layer)));

          // Replace the image in the stock that is pointed by the cel
          replaceImage(sprite, cel->imageRef(), new_image);
        }
      }
      else {
        // Update the cel's position
        setCelPosition(sprite, cel,
          cel->x()-bounds.x, cel->y()-bounds.y);
      }
    }
  }

  if (!m_document->mask()->isEmpty())
    setMaskPosition(m_document->mask()->bounds().x-bounds.x,
                    m_document->mask()->bounds().y-bounds.y);
}

void DocumentApi::trimSprite(Sprite* sprite)
{
  gfx::Rect bounds;

  base::UniquePtr<Image> image_wrap(Image::create(sprite->pixelFormat(),
                                                  sprite->width(),
                                                  sprite->height()));
  Image* image = image_wrap.get();
  render::Render render;

  for (frame_t frame(0); frame<sprite->totalFrames(); ++frame) {
    render.renderSprite(image, sprite, frame);

    // TODO configurable (what color pixel to use as "refpixel",
    // here we are using the top-left pixel by default)
    gfx::Rect frameBounds;
    if (doc::algorithm::shrink_bounds(image, frameBounds, get_pixel(image, 0, 0)))
      bounds = bounds.createUnion(frameBounds);
  }

  if (!bounds.isEmpty())
    cropSprite(sprite, bounds);
}

void DocumentApi::setPixelFormat(Sprite* sprite, PixelFormat newFormat, DitheringMethod dithering)
{
  if (sprite->pixelFormat() == newFormat)
    return;

  m_transaction.execute(new cmd::SetPixelFormat(sprite, newFormat, dithering));
}

void DocumentApi::addFrame(Sprite* sprite, frame_t newFrame)
{
  copyFrame(sprite, newFrame-1, newFrame);
}

void DocumentApi::addEmptyFrame(Sprite* sprite, frame_t newFrame)
{
  m_transaction.execute(new cmd::AddFrame(sprite, newFrame));
  adjustFrameTags(sprite, newFrame, +1, false);
}

void DocumentApi::addEmptyFramesTo(Sprite* sprite, frame_t newFrame)
{
  while (sprite->totalFrames() <= newFrame)
    addEmptyFrame(sprite, sprite->totalFrames());
}

void DocumentApi::copyFrame(Sprite* sprite, frame_t fromFrame, frame_t newFrame)
{
  ASSERT(sprite);
  m_transaction.execute(new cmd::CopyFrame(sprite, fromFrame, newFrame));
  adjustFrameTags(sprite, newFrame, +1, false);
}

void DocumentApi::removeFrame(Sprite* sprite, frame_t frame)
{
  ASSERT(frame >= 0);
  m_transaction.execute(new cmd::RemoveFrame(sprite, frame));
  adjustFrameTags(sprite, frame, -1, false);
}

void DocumentApi::setTotalFrames(Sprite* sprite, frame_t frames)
{
  ASSERT(frames >= 1);
  m_transaction.execute(new cmd::SetTotalFrames(sprite, frames));
}

void DocumentApi::setFrameDuration(Sprite* sprite, frame_t frame, int msecs)
{
  m_transaction.execute(new cmd::SetFrameDuration(sprite, frame, msecs));
}

void DocumentApi::setFrameRangeDuration(Sprite* sprite, frame_t from, frame_t to, int msecs)
{
  ASSERT(from >= frame_t(0));
  ASSERT(from < to);
  ASSERT(to <= sprite->lastFrame());

  for (frame_t fr=from; fr<=to; ++fr)
    m_transaction.execute(new cmd::SetFrameDuration(sprite, fr, msecs));
}

void DocumentApi::moveFrame(Sprite* sprite, frame_t frame, frame_t beforeFrame)
{
  if (frame != beforeFrame &&
      frame >= 0 &&
      frame <= sprite->lastFrame() &&
      beforeFrame >= 0 &&
      beforeFrame <= sprite->lastFrame()+1) {
    // Change the frame-lengths.
    int frlen_aux = sprite->frameDuration(frame);

    // Moving the frame to the future.
    if (frame < beforeFrame) {
      for (frame_t c=frame; c<beforeFrame-1; ++c)
        setFrameDuration(sprite, c, sprite->frameDuration(c+1));
      setFrameDuration(sprite, beforeFrame-1, frlen_aux);
    }
    // Moving the frame to the past.
    else if (beforeFrame < frame) {
      for (frame_t c=frame; c>beforeFrame; --c)
        setFrameDuration(sprite, c, sprite->frameDuration(c-1));
      setFrameDuration(sprite, beforeFrame, frlen_aux);
    }

    adjustFrameTags(sprite, frame, -1, true);
    adjustFrameTags(sprite, beforeFrame, +1, true);

    // Change cel positions.
    moveFrameLayer(sprite->folder(), frame, beforeFrame);
  }
}

void DocumentApi::moveFrameLayer(Layer* layer, frame_t frame, frame_t beforeFrame)
{
  ASSERT(layer);

  switch (layer->type()) {

    case ObjectType::LayerImage: {
      LayerImage* imglayer = static_cast<LayerImage*>(layer);

      CelList cels;
      imglayer->getCels(cels);

      CelIterator it = cels.begin();
      CelIterator end = cels.end();

      for (; it != end; ++it) {
        Cel* cel = *it;
        frame_t celFrame = cel->frame();
        frame_t newFrame = celFrame;

        // moving the frame to the future
        if (frame < beforeFrame) {
          if (celFrame == frame) {
            newFrame = beforeFrame-1;
          }
          else if (celFrame > frame &&
                   celFrame < beforeFrame) {
            --newFrame;
          }
        }
        // moving the frame to the past
        else if (beforeFrame < frame) {
          if (celFrame == frame) {
            newFrame = beforeFrame;
          }
          else if (celFrame >= beforeFrame &&
                   celFrame < frame) {
            ++newFrame;
          }
        }

        if (celFrame != newFrame)
          setCelFramePosition(cel, newFrame);
      }
      break;
    }

    case ObjectType::LayerFolder: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->getLayerBegin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it)
        moveFrameLayer(*it, frame, beforeFrame);
      break;
    }

  }
}

void DocumentApi::addCel(LayerImage* layer, Cel* cel)
{
  ASSERT(layer);
  ASSERT(cel);

  m_transaction.execute(new cmd::AddCel(layer, cel));
}

void DocumentApi::setCelFramePosition(Cel* cel, frame_t frame)
{
  ASSERT(cel);
  ASSERT(frame >= 0);

  m_transaction.execute(new cmd::SetCelFrame(cel, frame));
}

void DocumentApi::setCelPosition(Sprite* sprite, Cel* cel, int x, int y)
{
  ASSERT(cel);

  m_transaction.execute(new cmd::SetCelPosition(cel, x, y));
}

void DocumentApi::setCelOpacity(Sprite* sprite, Cel* cel, int newOpacity)
{
  ASSERT(cel);
  ASSERT(sprite->supportAlpha());

  m_transaction.execute(new cmd::SetCelOpacity(cel, newOpacity));
}

void DocumentApi::clearCel(LayerImage* layer, frame_t frame)
{
  if (Cel* cel = layer->cel(frame))
    clearCel(cel);
}

void DocumentApi::clearCel(Cel* cel)
{
  ASSERT(cel);
  m_transaction.execute(new cmd::ClearCel(cel));
}

void DocumentApi::moveCel(
  LayerImage* srcLayer, frame_t srcFrame,
  LayerImage* dstLayer, frame_t dstFrame)
{
  ASSERT(srcLayer != dstLayer || srcFrame != dstFrame);
  m_transaction.execute(new cmd::MoveCel(
      srcLayer, srcFrame,
      dstLayer, dstFrame, dstLayer->isContinuous()));
}

void DocumentApi::copyCel(
  LayerImage* srcLayer, frame_t srcFrame,
  LayerImage* dstLayer, frame_t dstFrame)
{
  copyCel(
    srcLayer, srcFrame,
    dstLayer, dstFrame, dstLayer->isContinuous());
}

void DocumentApi::copyCel(
  LayerImage* srcLayer, frame_t srcFrame,
  LayerImage* dstLayer, frame_t dstFrame, bool continuous)
{
  ASSERT(srcLayer != dstLayer || srcFrame != dstFrame);

  if (srcLayer == dstLayer && srcFrame == dstFrame)
    return;                     // Nothing to be done

  m_transaction.execute(
    new cmd::CopyCel(
      srcLayer, srcFrame,
      dstLayer, dstFrame, continuous));
}

void DocumentApi::swapCel(
  LayerImage* layer, frame_t frame1, frame_t frame2)
{
  ASSERT(frame1 != frame2);

  Sprite* sprite = layer->sprite();
  ASSERT(sprite != NULL);
  ASSERT(frame1 >= 0 && frame1 < sprite->totalFrames());
  ASSERT(frame2 >= 0 && frame2 < sprite->totalFrames());
  (void)sprite;              // To avoid unused variable warning on Release mode

  Cel* cel1 = layer->cel(frame1);
  Cel* cel2 = layer->cel(frame2);

  if (cel1) setCelFramePosition(cel1, frame2);
  if (cel2) setCelFramePosition(cel2, frame1);
}

LayerImage* DocumentApi::newLayer(Sprite* sprite, const std::string& name)
{
  LayerImage* layer = new LayerImage(sprite);
  layer->setName(name);

  addLayer(sprite->folder(), layer,
           sprite->folder()->getLastLayer());

  return layer;
}

LayerFolder* DocumentApi::newLayerFolder(Sprite* sprite)
{
  LayerFolder* layer = new LayerFolder(sprite);

  addLayer(sprite->folder(), layer,
           sprite->folder()->getLastLayer());

  return layer;
}

void DocumentApi::addLayer(LayerFolder* folder, Layer* newLayer, Layer* afterThis)
{
  m_transaction.execute(new cmd::AddLayer(folder, newLayer, afterThis));
}

void DocumentApi::removeLayer(Layer* layer)
{
  ASSERT(layer);

  m_transaction.execute(new cmd::RemoveLayer(layer));
}

void DocumentApi::restackLayerAfter(Layer* layer, Layer* afterThis)
{
  m_transaction.execute(new cmd::MoveLayer(layer, afterThis));
}

void DocumentApi::restackLayerBefore(Layer* layer, Layer* beforeThis)
{
  LayerIndex beforeThisIdx = layer->sprite()->layerToIndex(beforeThis);
  LayerIndex afterThisIdx = beforeThisIdx.previous();

  restackLayerAfter(layer, layer->sprite()->indexToLayer(afterThisIdx));
}

void DocumentApi::backgroundFromLayer(Layer* layer)
{
  m_transaction.execute(new cmd::BackgroundFromLayer(layer));
}

void DocumentApi::layerFromBackground(Layer* layer)
{
  m_transaction.execute(new cmd::LayerFromBackground(layer));
}

void DocumentApi::flattenLayers(Sprite* sprite)
{
  m_transaction.execute(new cmd::FlattenLayers(sprite));
}

void DocumentApi::duplicateLayerAfter(Layer* sourceLayer, Layer* afterLayer)
{
  base::UniquePtr<LayerImage> newLayerPtr(new LayerImage(sourceLayer->sprite()));

  m_document->copyLayerContent(sourceLayer, m_document, newLayerPtr);

  newLayerPtr->setName(newLayerPtr->name() + " Copy");

  addLayer(sourceLayer->parent(), newLayerPtr, afterLayer);

  // Release the pointer as it is owned by the sprite now.
  newLayerPtr.release();
}

void DocumentApi::duplicateLayerBefore(Layer* sourceLayer, Layer* beforeLayer)
{
  LayerIndex beforeThisIdx = sourceLayer->sprite()->layerToIndex(beforeLayer);
  LayerIndex afterThisIdx = beforeThisIdx.previous();

  duplicateLayerAfter(sourceLayer, sourceLayer->sprite()->indexToLayer(afterThisIdx));
}

Cel* DocumentApi::addCel(LayerImage* layer, frame_t frameNumber, const ImageRef& image)
{
  ASSERT(layer->cel(frameNumber) == NULL);

  base::UniquePtr<Cel> cel(new Cel(frameNumber, image));

  addCel(layer, cel);
  cel.release();

  return cel;
}

void DocumentApi::replaceImage(Sprite* sprite, const ImageRef& oldImage, const ImageRef& newImage)
{
  ASSERT(oldImage);
  ASSERT(newImage);
  ASSERT(oldImage->maskColor() == newImage->maskColor());

  m_transaction.execute(new cmd::ReplaceImage(
      sprite, oldImage, newImage));
}

void DocumentApi::flipImage(Image* image, const gfx::Rect& bounds,
  doc::algorithm::FlipType flipType)
{
  m_transaction.execute(new cmd::FlipImage(image, bounds, flipType));
}

void DocumentApi::copyToCurrentMask(Mask* mask)
{
  ASSERT(m_document->mask());
  ASSERT(mask);

  m_transaction.execute(new cmd::SetMask(m_document, mask));
}

void DocumentApi::setMaskPosition(int x, int y)
{
  ASSERT(m_document->mask());

  m_transaction.execute(new cmd::SetMaskPosition(m_document, gfx::Point(x, y)));
}

void DocumentApi::setPalette(Sprite* sprite, frame_t frame, const Palette* newPalette)
{
  Palette* currentSpritePalette = sprite->palette(frame); // Sprite current pal
  int from, to;

  // Check differences between current sprite palette and current system palette
  from = to = -1;
  currentSpritePalette->countDiff(newPalette, &from, &to);

  if (from >= 0 && to >= from) {
    m_transaction.execute(new cmd::SetPalette(
        sprite, frame, newPalette));
  }
}

void DocumentApi::adjustFrameTags(Sprite* sprite, frame_t frame, frame_t delta, bool between)
{
  // As FrameTag::setFrameRange() changes m_frameTags, we need to use
  // a copy of this collection
  std::vector<FrameTag*> tags(sprite->frameTags().begin(), sprite->frameTags().end());

  for (FrameTag* tag : tags) {
    frame_t from = tag->fromFrame();
    frame_t to = tag->toFrame();

    if (delta == +1) {
      if (frame <= from) { ++from; }
      if (frame <= to+1) { ++to; }
    }
    else if (delta == -1) {
      if (frame < from) { --from; }
      if (frame <= to) { --to; }
    }

    if (from != tag->fromFrame() ||
        to != tag->toFrame()) {
      if (from > to)
        m_transaction.execute(new cmd::RemoveFrameTag(sprite, tag));
      else
        m_transaction.execute(new cmd::SetFrameTagRange(tag, from, to));
    }
  }
}

} // namespace app
