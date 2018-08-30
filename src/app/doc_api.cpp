// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/doc_api.h"

#include "app/cmd/add_cel.h"
#include "app/cmd/add_frame.h"
#include "app/cmd/add_layer.h"
#include "app/cmd/background_from_layer.h"
#include "app/cmd/clear_cel.h"
#include "app/cmd/clear_image.h"
#include "app/cmd/copy_cel.h"
#include "app/cmd/copy_frame.h"
#include "app/cmd/flip_image.h"
#include "app/cmd/layer_from_background.h"
#include "app/cmd/move_cel.h"
#include "app/cmd/move_layer.h"
#include "app/cmd/remove_cel.h"
#include "app/cmd/remove_frame.h"
#include "app/cmd/remove_frame_tag.h"
#include "app/cmd/remove_layer.h"
#include "app/cmd/replace_image.h"
#include "app/cmd/set_cel_bounds.h"
#include "app/cmd/set_cel_frame.h"
#include "app/cmd/set_cel_opacity.h"
#include "app/cmd/set_cel_position.h"
#include "app/cmd/set_frame_duration.h"
#include "app/cmd/set_frame_tag_range.h"
#include "app/cmd/set_mask.h"
#include "app/cmd/set_mask_position.h"
#include "app/cmd/set_palette.h"
#include "app/cmd/set_slice_key.h"
#include "app/cmd/set_sprite_size.h"
#include "app/cmd/set_total_frames.h"
#include "app/cmd/set_transparent_color.h"
#include "app/color_target.h"
#include "app/color_utils.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/doc_undo.h"
#include "app/transaction.h"
#include "doc/algorithm/flip_image.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/frame_tag.h"
#include "doc/frame_tags.h"
#include "doc/mask.h"
#include "doc/palette.h"
#include "doc/slice.h"
#include "render/render.h"

#include <set>

#define TRACE_DOCAPI(...)

namespace app {

DocApi::DocApi(Doc* document, Transaction& transaction)
  : m_document(document)
  , m_transaction(transaction)
{
}

void DocApi::setSpriteSize(Sprite* sprite, int w, int h)
{
  m_transaction.execute(new cmd::SetSpriteSize(sprite, w, h));
}

void DocApi::setSpriteTransparentColor(Sprite* sprite, color_t maskColor)
{
  m_transaction.execute(new cmd::SetTransparentColor(sprite, maskColor));
}

void DocApi::cropSprite(Sprite* sprite, const gfx::Rect& bounds)
{
  setSpriteSize(sprite, bounds.w, bounds.h);

  Doc* doc = static_cast<Doc*>(sprite->document());
  LayerList layers = sprite->allLayers();
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
      else if (layer->isReference()) {
        // Update the ref cel's bounds
        gfx::RectF newBounds = cel->boundsF();
        newBounds.x -= bounds.x;
        newBounds.y -= bounds.y;
        m_transaction.execute(new cmd::SetCelBoundsF(cel, newBounds));
      }
      else {
        // Update the cel's position
        setCelPosition(sprite, cel,
          cel->x()-bounds.x, cel->y()-bounds.y);
      }
    }
  }

  // Update mask position
  if (!m_document->mask()->isEmpty())
    setMaskPosition(m_document->mask()->bounds().x-bounds.x,
                    m_document->mask()->bounds().y-bounds.y);

  // Update slice positions
  if (bounds.origin() != gfx::Point(0, 0)) {
    for (auto& slice : m_document->sprite()->slices()) {
      for (auto& k : *slice) {
        const SliceKey& key = *k.value();
        if (key.isEmpty())
          continue;

        SliceKey newKey = key;
        newKey.setBounds(
          gfx::Rect(newKey.bounds()).offset(-bounds.origin()));

        // As SliceKey::center() and pivot() properties are relative
        // to the bounds(), we don't need to adjust them.

        m_transaction.execute(
          new cmd::SetSliceKey(slice, k.frame(), newKey));
      }
    }
  }
}

void DocApi::trimSprite(Sprite* sprite)
{
  gfx::Rect bounds;

  std::unique_ptr<Image> image_wrap(Image::create(sprite->pixelFormat(),
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

void DocApi::addFrame(Sprite* sprite, frame_t newFrame)
{
  copyFrame(sprite, newFrame-1, newFrame,
            kDropBeforeFrame,
            kDefaultTagsAdjustment);
}

void DocApi::addEmptyFrame(Sprite* sprite, frame_t newFrame)
{
  m_transaction.execute(new cmd::AddFrame(sprite, newFrame));
  adjustFrameTags(sprite, newFrame, +1,
                  kDropBeforeFrame,
                  kDefaultTagsAdjustment);
}

void DocApi::addEmptyFramesTo(Sprite* sprite, frame_t newFrame)
{
  while (sprite->totalFrames() <= newFrame)
    addEmptyFrame(sprite, sprite->totalFrames());
}

void DocApi::copyFrame(Sprite* sprite,
                            const frame_t fromFrame,
                            const frame_t newFrame,
                            const DropFramePlace dropFramePlace,
                            const TagsHandling tagsHandling)
{
  ASSERT(sprite);
  m_transaction.execute(
    new cmd::CopyFrame(
      sprite, fromFrame,
      (dropFramePlace == kDropBeforeFrame ? newFrame:
                                            newFrame+1)));

  adjustFrameTags(sprite, newFrame, +1,
                  dropFramePlace,
                  tagsHandling);
}

void DocApi::removeFrame(Sprite* sprite, frame_t frame)
{
  ASSERT(frame >= 0);
  m_transaction.execute(new cmd::RemoveFrame(sprite, frame));
  adjustFrameTags(sprite, frame, -1,
                  kDropBeforeFrame,
                  kDefaultTagsAdjustment);
}

void DocApi::setTotalFrames(Sprite* sprite, frame_t frames)
{
  ASSERT(frames >= 1);
  m_transaction.execute(new cmd::SetTotalFrames(sprite, frames));
}

void DocApi::setFrameDuration(Sprite* sprite, frame_t frame, int msecs)
{
  m_transaction.execute(new cmd::SetFrameDuration(sprite, frame, msecs));
}

void DocApi::setFrameRangeDuration(Sprite* sprite, frame_t from, frame_t to, int msecs)
{
  ASSERT(from >= frame_t(0));
  ASSERT(from < to);
  ASSERT(to <= sprite->lastFrame());

  for (frame_t fr=from; fr<=to; ++fr)
    m_transaction.execute(new cmd::SetFrameDuration(sprite, fr, msecs));
}

void DocApi::moveFrame(Sprite* sprite,
                            const frame_t frame,
                            frame_t targetFrame,
                            const DropFramePlace dropFramePlace,
                            const TagsHandling tagsHandling)
{
  const frame_t beforeFrame =
    (dropFramePlace == kDropBeforeFrame ? targetFrame: targetFrame+1);

  if (frame       >= 0 && frame       <= sprite->lastFrame()   &&
      beforeFrame >= 0 && beforeFrame <= sprite->lastFrame()+1 &&
      ((frame != beforeFrame) ||
       (!sprite->frameTags().empty() &&
        tagsHandling != kDontAdjustTags))) {
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

    if (tagsHandling != kDontAdjustTags) {
      adjustFrameTags(sprite, frame, -1, dropFramePlace, tagsHandling);
      if (targetFrame >= frame)
        --targetFrame;
      adjustFrameTags(sprite, targetFrame, +1, dropFramePlace, tagsHandling);
    }

    // Change cel positions.
    if (frame != beforeFrame)
      moveFrameLayer(sprite->root(), frame, beforeFrame);
  }
}

void DocApi::moveFrameLayer(Layer* layer, frame_t frame, frame_t beforeFrame)
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

    case ObjectType::LayerGroup: {
      for (Layer* child : static_cast<LayerGroup*>(layer)->layers())
        moveFrameLayer(child, frame, beforeFrame);
      break;
    }

  }
}

void DocApi::addCel(LayerImage* layer, Cel* cel)
{
  ASSERT(layer);
  ASSERT(cel);

  m_transaction.execute(new cmd::AddCel(layer, cel));
}

void DocApi::setCelFramePosition(Cel* cel, frame_t frame)
{
  ASSERT(cel);
  ASSERT(frame >= 0);

  m_transaction.execute(new cmd::SetCelFrame(cel, frame));
}

void DocApi::setCelPosition(Sprite* sprite, Cel* cel, int x, int y)
{
  ASSERT(cel);

  m_transaction.execute(new cmd::SetCelPosition(cel, x, y));
}

void DocApi::setCelOpacity(Sprite* sprite, Cel* cel, int newOpacity)
{
  ASSERT(cel);
  ASSERT(sprite->supportAlpha());

  m_transaction.execute(new cmd::SetCelOpacity(cel, newOpacity));
}

void DocApi::clearCel(LayerImage* layer, frame_t frame)
{
  if (Cel* cel = layer->cel(frame))
    clearCel(cel);
}

void DocApi::clearCel(Cel* cel)
{
  ASSERT(cel);
  m_transaction.execute(new cmd::ClearCel(cel));
}

void DocApi::moveCel(
  LayerImage* srcLayer, frame_t srcFrame,
  LayerImage* dstLayer, frame_t dstFrame)
{
  ASSERT(srcLayer != dstLayer || srcFrame != dstFrame);
  m_transaction.execute(new cmd::MoveCel(
      srcLayer, srcFrame,
      dstLayer, dstFrame, dstLayer->isContinuous()));
}

void DocApi::copyCel(
  LayerImage* srcLayer, frame_t srcFrame,
  LayerImage* dstLayer, frame_t dstFrame)
{
  copyCel(
    srcLayer, srcFrame,
    dstLayer, dstFrame, dstLayer->isContinuous());
}

void DocApi::copyCel(
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

void DocApi::swapCel(
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

LayerImage* DocApi::newLayer(LayerGroup* parent, const std::string& name)
{
  LayerImage* newLayer = new LayerImage(parent->sprite());
  newLayer->setName(name);

  addLayer(parent, newLayer, parent->lastLayer());
  return newLayer;
}

LayerGroup* DocApi::newGroup(LayerGroup* parent, const std::string& name)
{
  LayerGroup* newLayerGroup = new LayerGroup(parent->sprite());
  newLayerGroup->setName(name);

  addLayer(parent, newLayerGroup, parent->lastLayer());
  return newLayerGroup;
}

void DocApi::addLayer(LayerGroup* parent, Layer* newLayer, Layer* afterThis)
{
  m_transaction.execute(new cmd::AddLayer(parent, newLayer, afterThis));
}

void DocApi::removeLayer(Layer* layer)
{
  ASSERT(layer);

  m_transaction.execute(new cmd::RemoveLayer(layer));
}

void DocApi::restackLayerAfter(Layer* layer, LayerGroup* parent, Layer* afterThis)
{
  ASSERT(parent);

  if (layer == afterThis)
    return;

  m_transaction.execute(new cmd::MoveLayer(layer, parent, afterThis));
}

void DocApi::restackLayerBefore(Layer* layer, LayerGroup* parent, Layer* beforeThis)
{
  ASSERT(parent);

  if (layer == beforeThis)
    return;

  Layer* afterThis;
  if (beforeThis)
    afterThis = beforeThis->getPrevious();
  else
    afterThis = parent->lastLayer();

  restackLayerAfter(layer, parent, afterThis);
}

void DocApi::backgroundFromLayer(Layer* layer)
{
  m_transaction.execute(new cmd::BackgroundFromLayer(layer));
}

void DocApi::layerFromBackground(Layer* layer)
{
  m_transaction.execute(new cmd::LayerFromBackground(layer));
}

Layer* DocApi::duplicateLayerAfter(Layer* sourceLayer, LayerGroup* parent, Layer* afterLayer)
{
  ASSERT(parent);
  std::unique_ptr<Layer> newLayerPtr;

  if (sourceLayer->isImage())
    newLayerPtr.reset(new LayerImage(sourceLayer->sprite()));
  else if (sourceLayer->isGroup())
    newLayerPtr.reset(new LayerGroup(sourceLayer->sprite()));
  else
    throw std::runtime_error("Invalid layer type");

  m_document->copyLayerContent(sourceLayer, m_document, newLayerPtr.get());

  newLayerPtr->setName(newLayerPtr->name() + " Copy");

  addLayer(parent, newLayerPtr.get(), afterLayer);

  // Release the pointer as it is owned by the sprite now.
  return newLayerPtr.release();
}

Layer* DocApi::duplicateLayerBefore(Layer* sourceLayer, LayerGroup* parent, Layer* beforeLayer)
{
  ASSERT(parent);
  Layer* afterThis = (beforeLayer ? beforeLayer->getPreviousBrowsable(): nullptr);
  Layer* newLayer = duplicateLayerAfter(sourceLayer, parent, afterThis);
  if (newLayer)
    restackLayerBefore(newLayer, parent, beforeLayer);
  return newLayer;
}

Cel* DocApi::addCel(LayerImage* layer, frame_t frameNumber, const ImageRef& image)
{
  ASSERT(layer->cel(frameNumber) == NULL);

  std::unique_ptr<Cel> cel(new Cel(frameNumber, image));

  addCel(layer, cel.get());
  return cel.release();
}

void DocApi::replaceImage(Sprite* sprite, const ImageRef& oldImage, const ImageRef& newImage)
{
  ASSERT(oldImage);
  ASSERT(newImage);
  ASSERT(oldImage->maskColor() == newImage->maskColor());

  m_transaction.execute(new cmd::ReplaceImage(
      sprite, oldImage, newImage));
}

void DocApi::flipImage(Image* image, const gfx::Rect& bounds,
  doc::algorithm::FlipType flipType)
{
  m_transaction.execute(new cmd::FlipImage(image, bounds, flipType));
}

void DocApi::copyToCurrentMask(Mask* mask)
{
  ASSERT(m_document->mask());
  ASSERT(mask);

  m_transaction.execute(new cmd::SetMask(m_document, mask));
}

void DocApi::setMaskPosition(int x, int y)
{
  ASSERT(m_document->mask());

  m_transaction.execute(new cmd::SetMaskPosition(m_document, gfx::Point(x, y)));
}

void DocApi::setPalette(Sprite* sprite, frame_t frame, const Palette* newPalette)
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

void DocApi::adjustFrameTags(Sprite* sprite,
                                  const frame_t frame,
                                  const frame_t delta,
                                  const DropFramePlace dropFramePlace,
                                  const TagsHandling tagsHandling)
{
  TRACE_DOCAPI(
    "\n  adjustFrameTags %s frame %d delta=%d tags=%s:\n",
    (dropFramePlace == kDropBeforeFrame ? "before": "after"),
    frame, delta,
    (tagsHandling == kDefaultTagsAdjustment ? "default":
     tagsHandling == kFitInsideTags ? "fit-inside":
                                      "fit-outside"));
  ASSERT(tagsHandling != kDontAdjustTags);

  // As FrameTag::setFrameRange() changes m_frameTags, we need to use
  // a copy of this collection
  std::vector<FrameTag*> tags(sprite->frameTags().begin(), sprite->frameTags().end());

  for (FrameTag* tag : tags) {
    frame_t from = tag->fromFrame();
    frame_t to = tag->toFrame();

    TRACE_DOCAPI(" - [from to]=[%d %d] ->", from, to);

    // When delta = +1, frame = beforeFrame
    if (delta == +1) {
      switch (tagsHandling) {
        case kDefaultTagsAdjustment:
          if (frame <= from) { ++from; }
          if (frame <= to+1) { ++to; }
          break;
        case kFitInsideTags:
          if (frame < from) { ++from; }
          if (frame <= to) { ++to; }
          break;
        case kFitOutsideTags:
          if ((frame < from) ||
              (frame == from &&
               dropFramePlace == kDropBeforeFrame)) {
            ++from;
          }
          if ((frame < to) ||
              (frame == to &&
               dropFramePlace == kDropBeforeFrame)) {
            ++to;
          }
          break;
      }
    }
    else if (delta == -1) {
      if (frame < from) { --from; }
      if (frame <= to) { --to; }
    }

    TRACE_DOCAPI(" [%d %d]\n", from, to);

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
