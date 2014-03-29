/* Aseprite
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document_api.h"

#include "app/document.h"
#include "app/document_event.h"
#include "app/document_observer.h"
#include "app/document_undo.h"
#include "app/undoers/add_cel.h"
#include "app/undoers/add_frame.h"
#include "app/undoers/add_image.h"
#include "app/undoers/add_layer.h"
#include "app/undoers/add_palette.h"
#include "app/undoers/dirty_area.h"
#include "app/undoers/flip_image.h"
#include "app/undoers/image_area.h"
#include "app/undoers/move_layer.h"
#include "app/undoers/move_layer.h"
#include "app/undoers/remove_cel.h"
#include "app/undoers/remove_frame.h"
#include "app/undoers/remove_image.h"
#include "app/undoers/remove_layer.h"
#include "app/undoers/remove_palette.h"
#include "app/undoers/replace_image.h"
#include "app/undoers/set_cel_frame.h"
#include "app/undoers/set_cel_position.h"
#include "app/undoers/set_frame_duration.h"
#include "app/undoers/set_layer_flags.h"
#include "app/undoers/set_layer_flags.h"
#include "app/undoers/set_layer_name.h"
#include "app/undoers/set_layer_name.h"
#include "app/undoers/set_mask.h"
#include "app/undoers/set_mask_position.h"
#include "app/undoers/set_palette_colors.h"
#include "app/undoers/set_sprite_pixel_format.h"
#include "app/undoers/set_sprite_size.h"
#include "app/undoers/set_stock_pixel_format.h"
#include "app/undoers/set_total_frames.h"
#include "base/unique_ptr.h"
#include "raster/algorithm/flip_image.h"
#include "raster/algorithm/shrink_bounds.h"
#include "raster/blend.h"
#include "raster/cel.h"
#include "raster/dirty.h"
#include "raster/image.h"
#include "raster/image_bits.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/palette.h"
#include "raster/quantization.h"
#include "raster/sprite.h"
#include "raster/stock.h"



namespace app {

DocumentApi::DocumentApi(Document* document, undo::UndoersCollector* undoers)
  : m_document(document)
  , m_undoers(undoers)
{
}

undo::ObjectsContainer* DocumentApi::getObjects() const
{
  return m_document->getUndo()->getObjects();
}

void DocumentApi::setSpriteSize(Sprite* sprite, int w, int h)
{
  ASSERT(w > 0);
  ASSERT(h > 0);

  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::SetSpriteSize(getObjects(), sprite));

  sprite->setSize(w, h);

  DocumentEvent ev(m_document);
  ev.sprite(sprite);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onSpriteSizeChanged, ev);
}

void DocumentApi::cropSprite(Sprite* sprite, const gfx::Rect& bounds, int bgcolor)
{
  setSpriteSize(sprite, bounds.w, bounds.h);
  displaceLayers(sprite->getFolder(), -bounds.x, -bounds.y);

  Layer *background_layer = sprite->getBackgroundLayer();
  if (background_layer)
    cropLayer(background_layer, 0, 0, sprite->getWidth(), sprite->getHeight(), bgcolor);

  if (!m_document->getMask()->isEmpty())
    setMaskPosition(m_document->getMask()->getBounds().x-bounds.x,
                    m_document->getMask()->getBounds().y-bounds.y);
}

void DocumentApi::trimSprite(Sprite* sprite, int bgcolor)
{
  gfx::Rect bounds;

  base::UniquePtr<Image> image_wrap(Image::create(sprite->getPixelFormat(),
                                                  sprite->getWidth(),
                                                  sprite->getHeight()));
  Image* image = image_wrap.get();

  for (FrameNumber frame(0); frame<sprite->getTotalFrames(); ++frame) {
    image->clear(0);

    sprite->render(image, 0, 0, frame);

    // TODO configurable (what color pixel to use as "refpixel",
    // here we are using the top-left pixel by default)
    gfx::Rect frameBounds;
    if (raster::algorithm::shrink_bounds(image, frameBounds, get_pixel(image, 0, 0)))
      bounds = bounds.createUnion(frameBounds);
  }

  if (!bounds.isEmpty())
    cropSprite(sprite, bounds, bgcolor);
}

void DocumentApi::setPixelFormat(Sprite* sprite, PixelFormat newFormat, DitheringMethod dithering_method)
{
  Image* old_image;
  Image* new_image;
  int c;

  if (sprite->getPixelFormat() == newFormat)
    return;

  // Change pixel format of the stock of images.
  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::SetStockPixelFormat(getObjects(), sprite->getStock()));

  sprite->getStock()->setPixelFormat(newFormat);

  // TODO Review this, why we use the palette in frame 0?
  FrameNumber frame(0);

  // Use the rgbmap for the specified sprite
  const RgbMap* rgbmap = sprite->getRgbMap(frame);

  for (c=0; c<sprite->getStock()->size(); c++) {
    old_image = sprite->getStock()->getImage(c);
    if (!old_image)
      continue;

    new_image = quantization::convert_pixel_format
      (old_image, newFormat, dithering_method, rgbmap,
       sprite->getPalette(frame),
       sprite->getBackgroundLayer() != NULL);

    replaceStockImage(sprite, c, new_image);
  }

  // Change sprite's pixel format.
  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::SetSpritePixelFormat(getObjects(), sprite));

  sprite->setPixelFormat(newFormat);

  // Regenerate extras
  m_document->destroyExtraCel();

  // When we are converting to grayscale color mode, we've to destroy
  // all palettes and put only one grayscaled-palette at the first
  // frame.
  if (newFormat == IMAGE_GRAYSCALE) {
    // Add undoers to revert all palette changes.
    if (undoEnabled()) {
      PalettesList palettes = sprite->getPalettes();
      for (PalettesList::iterator it = palettes.begin(); it != palettes.end(); ++it) {
        Palette* palette = *it;
        m_undoers->pushUndoer(new undoers::RemovePalette(
            getObjects(), sprite, palette->getFrame()));
      }

      m_undoers->pushUndoer(new undoers::AddPalette(
        getObjects(), sprite, FrameNumber(0)));
    }

    // It's a base::UniquePtr because setPalette'll create a copy of "graypal".
    base::UniquePtr<Palette> graypal(Palette::createGrayscale());

    sprite->resetPalettes();
    sprite->setPalette(graypal, true);
  }
}

void DocumentApi::addFrame(Sprite* sprite, FrameNumber newFrame)
{
  // Move cels, and create copies of the cels in the given "newFrame".
  addFrameForLayer(sprite->getFolder(), newFrame);

  // Add the frame in the sprite structure, it adjusts the total
  // number of frames in the sprite.
  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::AddFrame(getObjects(), m_document, sprite, newFrame));

  sprite->addFrame(newFrame);

  // Notify observers about the new frame.
  DocumentEvent ev(m_document);
  ev.sprite(sprite);
  ev.frame(newFrame);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onAddFrame, ev);
}

void DocumentApi::addFrameForLayer(Layer* layer, FrameNumber frame)
{
  ASSERT(layer);
  ASSERT(frame >= 0);

  Sprite* sprite = layer->getSprite();

  switch (layer->type()) {

    case OBJECT_LAYER_IMAGE:
      // Displace all cels in '>=frame' to the next frame.
      for (FrameNumber c=sprite->getLastFrame(); c>=frame; --c) {
        Cel* cel = static_cast<LayerImage*>(layer)->getCel(c);
        if (cel)
          setCelFramePosition(sprite, cel, cel->getFrame().next());
      }

      copyPreviousFrame(layer, frame);
      break;

    case OBJECT_LAYER_FOLDER: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->getLayerBegin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it)
        addFrameForLayer(*it, frame);
      break;
    }

  }
}

void DocumentApi::copyPreviousFrame(Layer* layer, FrameNumber frame)
{
  ASSERT(layer);
  ASSERT(frame >= 0);

  Sprite* sprite = layer->getSprite();

  // create a copy of the previous cel
  Cel* src_cel = static_cast<LayerImage*>(layer)->getCel(frame.previous());
  Image* src_image = src_cel ? sprite->getStock()->getImage(src_cel->getImage()):
                               NULL;

  // nothing to copy, it will be a transparent cel
  if (!src_image)
    return;

  // copy the image
  Image* dst_image = Image::createCopy(src_image);
  int image_index = addImageInStock(sprite, dst_image);

  // create the new cel
  Cel* dst_cel = new Cel(frame, image_index);
  if (src_cel) {                // copy the data from the previous cel
    dst_cel->setPosition(src_cel->getX(), src_cel->getY());
    dst_cel->setOpacity(src_cel->getOpacity());
  }

  // add the cel in the layer
  addCel(static_cast<LayerImage*>(layer), dst_cel);
}

void DocumentApi::removeFrame(Sprite* sprite, FrameNumber frame)
{
  ASSERT(frame >= 0);

  // Remove cels from this frame (and displace one position backward
  // all next frames)
  removeFrameOfLayer(sprite->getFolder(), frame);

  // Add undoers to restore the removed frame from the sprite (to
  // restore the number and durations of frames).
  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::RemoveFrame(getObjects(), m_document, sprite, frame));

  // Remove the frame from the sprite. This is the low level
  // operation, it modifies the number and duration of frames.
  sprite->removeFrame(frame);

  // Notify observers.
  DocumentEvent ev(m_document);
  ev.sprite(sprite);
  ev.frame(frame);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onRemoveFrame, ev);
}

// Does the hard part of removing a frame: Removes all cels located in
// the given frame, and moves all following cels one frame position back.
void DocumentApi::removeFrameOfLayer(Layer* layer, FrameNumber frame)
{
  ASSERT(layer);
  ASSERT(frame >= 0);

  Sprite* sprite = layer->getSprite();

  switch (layer->type()) {

    case OBJECT_LAYER_IMAGE:
      if (Cel* cel = static_cast<LayerImage*>(layer)->getCel(frame))
        removeCel(static_cast<LayerImage*>(layer), cel);

      for (++frame; frame<sprite->getTotalFrames(); ++frame)
        if (Cel* cel = static_cast<LayerImage*>(layer)->getCel(frame))
          setCelFramePosition(sprite, cel, cel->getFrame().previous());
      break;

    case OBJECT_LAYER_FOLDER: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->getLayerBegin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it)
        removeFrameOfLayer(*it, frame);
      break;
    }

  }
}

void DocumentApi::setTotalFrames(Sprite* sprite, FrameNumber frames)
{
  ASSERT(frames >= 1);

  // Add undoers.
  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::SetTotalFrames(getObjects(), m_document, sprite));

  // Do the action.
  sprite->setTotalFrames(frames);

  // Notify observers.
  DocumentEvent ev(m_document);
  ev.sprite(sprite);
  ev.frame(frames);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onTotalFramesChanged, ev);
}

void DocumentApi::setFrameDuration(Sprite* sprite, FrameNumber frame, int msecs)
{
  // Add undoers.
  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::SetFrameDuration(
        getObjects(), sprite, frame));

  // Do the action.
  sprite->setFrameDuration(frame, msecs);

  // Notify observers.
  DocumentEvent ev(m_document);
  ev.sprite(sprite);
  ev.frame(frame);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onFrameDurationChanged, ev);
}

void DocumentApi::setFrameRangeDuration(Sprite* sprite, FrameNumber from, FrameNumber to, int msecs)
{
  ASSERT(from >= FrameNumber(0));
  ASSERT(from < to);
  ASSERT(to <= sprite->getLastFrame());

  // Add undoers.
  if (undoEnabled()) {
    for (FrameNumber fr(from); fr<=to; ++fr)
      m_undoers->pushUndoer(new undoers::SetFrameDuration(
          getObjects(), sprite, fr));
  }

  // Do the action.
  sprite->setFrameRangeDuration(from, to, msecs);
}

void DocumentApi::moveFrameBefore(Sprite* sprite, FrameNumber frame, FrameNumber beforeFrame)
{
  if (frame != beforeFrame &&
      frame >= 0 &&
      frame <= sprite->getLastFrame() &&
      beforeFrame >= 0 &&
      beforeFrame <= sprite->getLastFrame()) {
    // Change the frame-lengths...
    int frlen_aux = sprite->getFrameDuration(frame);

    // Moving the frame to the future.
    if (frame < beforeFrame) {
      for (FrameNumber c=frame; c<beforeFrame.previous(); ++c)
        setFrameDuration(sprite, c, sprite->getFrameDuration(c.next()));

      setFrameDuration(sprite, beforeFrame.previous(), frlen_aux);
    }
    // Moving the frame to the past.
    else if (beforeFrame < frame) {
      for (FrameNumber c=frame; c>beforeFrame; --c)
        setFrameDuration(sprite, c, sprite->getFrameDuration(c.previous()));

      setFrameDuration(sprite, beforeFrame, frlen_aux);
    }

    // change the cels of position...
    moveFrameBeforeLayer(sprite->getFolder(), frame, beforeFrame);
  }
}

void DocumentApi::moveFrameBeforeLayer(Layer* layer, FrameNumber frame, FrameNumber beforeFrame)
{
  ASSERT(layer);

  switch (layer->type()) {

    case OBJECT_LAYER_IMAGE: {
      CelIterator it = ((LayerImage*)layer)->getCelBegin();
      CelIterator end = ((LayerImage*)layer)->getCelEnd();

      for (; it != end; ++it) {
        Cel* cel = *it;
        FrameNumber newFrame = cel->getFrame();

        // moving the frame to the future
        if (frame < beforeFrame) {
          if (cel->getFrame() == frame) {
            newFrame = beforeFrame.previous();
          }
          else if (cel->getFrame() > frame &&
                   cel->getFrame() < beforeFrame) {
            --newFrame;
          }
        }
        // moving the frame to the past
        else if (beforeFrame < frame) {
          if (cel->getFrame() == frame) {
            newFrame = beforeFrame;
          }
          else if (cel->getFrame() >= beforeFrame &&
                   cel->getFrame() < frame) {
            ++newFrame;
          }
        }

        if (cel->getFrame() != newFrame)
          setCelFramePosition(layer->getSprite(), cel, newFrame);
      }
      break;
    }

    case OBJECT_LAYER_FOLDER: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->getLayerBegin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it)
        moveFrameBeforeLayer(*it, frame, beforeFrame);
      break;
    }

  }
}

void DocumentApi::addCel(LayerImage* layer, Cel* cel)
{
  ASSERT(layer);
  ASSERT(cel);

  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::AddCel(getObjects(), layer, cel));

  layer->addCel(cel);

  DocumentEvent ev(m_document);
  ev.sprite(layer->getSprite());
  ev.layer(layer);
  ev.cel(cel);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onAddCel, ev);
}

void DocumentApi::removeCel(LayerImage* layer, Cel* cel)
{
  ASSERT(layer);
  ASSERT(cel);

  Sprite* sprite = layer->getSprite();

  DocumentEvent ev(m_document);
  ev.sprite(sprite);
  ev.layer(layer);
  ev.cel(cel);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onRemoveCel, ev);

  // find if the image that use the cel to remove, is used by
  // another cels
  bool used = false;
  for (FrameNumber frame(0); frame<sprite->getTotalFrames(); ++frame) {
    Cel* it = layer->getCel(frame);
    if (it && it != cel && it->getImage() == cel->getImage()) {
      used = true;
      break;
    }
  }

  // if the image is only used by this cel,
  // we can remove the image from the stock
  if (!used)
    removeImageFromStock(sprite, cel->getImage());

  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::RemoveCel(getObjects(),
        layer, cel));

  // remove the cel from the layer
  layer->removeCel(cel);

  // and here we destroy the cel
  delete cel;
}

void DocumentApi::setCelFramePosition(Sprite* sprite, Cel* cel, FrameNumber frame)
{
  ASSERT(cel);
  ASSERT(frame >= 0);

  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::SetCelFrame(getObjects(), cel));

  cel->setFrame(frame);

  DocumentEvent ev(m_document);
  ev.sprite(sprite);
  ev.cel(cel);
  ev.frame(frame);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onCelFrameChanged, ev);
}

void DocumentApi::setCelPosition(Sprite* sprite, Cel* cel, int x, int y)
{
  ASSERT(cel);

  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::SetCelPosition(getObjects(), cel));

  cel->setPosition(x, y);

  DocumentEvent ev(m_document);
  ev.sprite(sprite);
  ev.cel(cel);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onCelPositionChanged, ev);
}

void DocumentApi::cropCel(Sprite* sprite, Cel* cel, int x, int y, int w, int h, int bgcolor)
{
  Image* cel_image = sprite->getStock()->getImage(cel->getImage());
  ASSERT(cel_image);

  // create the new image through a crop
  Image* new_image = crop_image(cel_image, x-cel->getX(), y-cel->getY(), w, h, bgcolor);

  // replace the image in the stock that is pointed by the cel
  replaceStockImage(sprite, cel->getImage(), new_image);

  // update the cel's position
  setCelPosition(sprite, cel, x, y);
}

LayerImage* DocumentApi::newLayer(Sprite* sprite)
{
  LayerImage* layer = new LayerImage(sprite);

  addLayer(sprite->getFolder(), layer,
           sprite->getFolder()->getLastLayer());

  return layer;
}

LayerFolder* DocumentApi::newLayerFolder(Sprite* sprite)
{
  LayerFolder* layer = new LayerFolder(sprite);

  addLayer(sprite->getFolder(), layer,
           sprite->getFolder()->getLastLayer());

  return layer;
}

void DocumentApi::addLayer(LayerFolder* folder, Layer* newLayer, Layer* afterThis)
{
  // Add undoers.
  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::AddLayer(getObjects(),
                                                m_document, newLayer));

  // Do the action.
  folder->addLayer(newLayer);
  folder->stackLayer(newLayer, afterThis);

  // Notify observers.
  DocumentEvent ev(m_document);
  ev.sprite(folder->getSprite());
  ev.layer(newLayer);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onAddLayer, ev);
}

void DocumentApi::removeLayer(Layer* layer)
{
  ASSERT(layer != NULL);

  // Notify observers that a layer will be removed (e.g. an Editor can
  // select another layer if the removed layer is the active one).
  DocumentEvent ev(m_document);
  ev.sprite(layer->getSprite());
  ev.layer(layer);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onBeforeRemoveLayer, ev);

  // Add undoers.
  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::RemoveLayer(getObjects(), m_document, layer));

  // Do the action.
  layer->getParent()->removeLayer(layer);

  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onAfterRemoveLayer, ev);

  delete layer;
}

void DocumentApi::configureLayerAsBackground(LayerImage* layer)
{
  // Add undoers.
  if (undoEnabled()) {
    m_undoers->pushUndoer(new undoers::SetLayerFlags(getObjects(), layer));
    m_undoers->pushUndoer(new undoers::SetLayerName(getObjects(), layer));
    m_undoers->pushUndoer(new undoers::MoveLayer(getObjects(), layer));
  }

  // Do the action.
  layer->configureAsBackground();
}

void DocumentApi::restackLayerAfter(Layer* layer, Layer* afterThis)
{
  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::MoveLayer(getObjects(), layer));

  layer->getParent()->stackLayer(layer, afterThis);

  DocumentEvent ev(m_document);
  ev.sprite(layer->getSprite());
  ev.layer(layer);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onLayerRestacked, ev);
}

void DocumentApi::cropLayer(Layer* layer, int x, int y, int w, int h, int bgcolor)
{
  if (!layer->isImage())
    return;

  if (!layer->isBackground())
    bgcolor = 0;

  Sprite* sprite = layer->getSprite();
  CelIterator it = ((LayerImage*)layer)->getCelBegin();
  CelIterator end = ((LayerImage*)layer)->getCelEnd();
  for (; it != end; ++it)
    cropCel(sprite, *it, x, y, w, h, bgcolor);
}

// Moves every frame in @a layer with the offset (@a dx, @a dy).
void DocumentApi::displaceLayers(Layer* layer, int dx, int dy)
{
  switch (layer->type()) {

    case OBJECT_LAYER_IMAGE: {
      CelIterator it = ((LayerImage*)layer)->getCelBegin();
      CelIterator end = ((LayerImage*)layer)->getCelEnd();
      for (; it != end; ++it) {
        Cel* cel = *it;
        setCelPosition(layer->getSprite(), cel, cel->getX()+dx, cel->getY()+dy);
      }
      break;
    }

    case OBJECT_LAYER_FOLDER: {
      LayerIterator it = ((LayerFolder*)layer)->getLayerBegin();
      LayerIterator end = ((LayerFolder*)layer)->getLayerEnd();
      for (; it != end; ++it)
        displaceLayers(*it, dx, dy);
      break;
    }

  }
}

void DocumentApi::backgroundFromLayer(LayerImage* layer, int bgcolor)
{
  ASSERT(layer);
  ASSERT(layer->isImage());
  ASSERT(layer->isReadable());
  ASSERT(layer->isWritable());
  ASSERT(layer->getSprite() != NULL);
  ASSERT(layer->getSprite()->getBackgroundLayer() == NULL);

  Sprite* sprite = layer->getSprite();

  // create a temporary image to draw each frame of the new
  // `Background' layer
  base::UniquePtr<Image> bg_image_wrap(Image::create(sprite->getPixelFormat(),
                                               sprite->getWidth(),
                                               sprite->getHeight()));
  Image* bg_image = bg_image_wrap.get();

  CelIterator it = layer->getCelBegin();
  CelIterator end = layer->getCelEnd();

  for (; it != end; ++it) {
    Cel* cel = *it;
    ASSERT((cel->getImage() > 0) &&
           (cel->getImage() < sprite->getStock()->size()));

    // get the image from the sprite's stock of images
    Image* cel_image = sprite->getStock()->getImage(cel->getImage());
    ASSERT(cel_image);

    clear_image(bg_image, bgcolor);
    composite_image(bg_image, cel_image,
                    cel->getX(),
                    cel->getY(),
                    MID(0, cel->getOpacity(), 255),
                    layer->getBlendMode());

    // now we have to copy the new image (bg_image) to the cel...
    setCelPosition(sprite, cel, 0, 0);

    // same size of cel-image and bg-image
    if (bg_image->getWidth() == cel_image->getWidth() &&
        bg_image->getHeight() == cel_image->getHeight()) {
      if (undoEnabled())
        m_undoers->pushUndoer(new undoers::ImageArea(getObjects(),
          cel_image, 0, 0, cel_image->getWidth(), cel_image->getHeight()));

      copy_image(cel_image, bg_image, 0, 0);
    }
    else {
      replaceStockImage(sprite, cel->getImage(), Image::createCopy(bg_image));
    }
  }

  // Fill all empty cels with a flat-image filled with bgcolor
  for (FrameNumber frame(0); frame<sprite->getTotalFrames(); ++frame) {
    Cel* cel = layer->getCel(frame);
    if (!cel) {
      Image* cel_image = Image::create(sprite->getPixelFormat(), sprite->getWidth(), sprite->getHeight());
      clear_image(cel_image, bgcolor);

      // Add the new image in the stock
      int image_index = addImageInStock(sprite, cel_image);

      // Create the new cel and add it to the new background layer
      cel = new Cel(frame, image_index);
      addCel(layer, cel);
    }
  }

  configureLayerAsBackground(layer);
}

void DocumentApi::layerFromBackground(Layer* layer)
{
  ASSERT(layer != NULL);
  ASSERT(layer->isImage());
  ASSERT(layer->isReadable());
  ASSERT(layer->isWritable());
  ASSERT(layer->isBackground());
  ASSERT(layer->getSprite() != NULL);
  ASSERT(layer->getSprite()->getBackgroundLayer() != NULL);

  if (undoEnabled()) {
    m_undoers->pushUndoer(new undoers::SetLayerFlags(getObjects(), layer));
    m_undoers->pushUndoer(new undoers::SetLayerName(getObjects(), layer));
  }

  layer->setBackground(false);
  layer->setMoveable(true);
  layer->setName("Layer 0");
}

void DocumentApi::flattenLayers(Sprite* sprite, int bgcolor)
{
  Image* cel_image;
  Cel* cel;

  // Create a temporary image.
  base::UniquePtr<Image> image_wrap(Image::create(sprite->getPixelFormat(),
                                            sprite->getWidth(),
                                            sprite->getHeight()));
  Image* image = image_wrap.get();

  // Get the background layer from the sprite.
  LayerImage* background = sprite->getBackgroundLayer();
  if (!background) {
    // If there aren't a background layer we must to create the background.
    background = new LayerImage(sprite);

    addLayer(sprite->getFolder(), background, NULL);
    configureLayerAsBackground(background);
  }

  // Copy all frames to the background.
  for (FrameNumber frame(0); frame<sprite->getTotalFrames(); ++frame) {
    // Clear the image and render this frame.
    clear_image(image, bgcolor);
    layer_render(sprite->getFolder(), image, 0, 0, frame);

    cel = background->getCel(frame);
    if (cel) {
      cel_image = sprite->getStock()->getImage(cel->getImage());
      ASSERT(cel_image != NULL);

      // We have to save the current state of `cel_image' in the undo.
      if (undoEnabled()) {
        Dirty* dirty = new Dirty(cel_image, image, image->getBounds());
        dirty->saveImagePixels(cel_image);
        m_undoers->pushUndoer(new undoers::DirtyArea(
            getObjects(), cel_image, dirty));
        delete dirty;
      }
    }
    else {
      // If there aren't a cel in this frame in the background, we
      // have to create a copy of the image for the new cel.
      cel_image = Image::createCopy(image);
      // TODO error handling: if createCopy throws

      // Here we create the new cel (with the new image `cel_image').
      cel = new Cel(frame, sprite->getStock()->addImage(cel_image));
      // TODO error handling: if new Cel throws

      // And finally we add the cel in the background.
      background->addCel(cel);
    }

    copy_image(cel_image, image, 0, 0);
  }

  // Delete old layers.
  LayerList layers = sprite->getFolder()->getLayersList();
  LayerIterator it = layers.begin();
  LayerIterator end = layers.end();
  for (; it != end; ++it)
    if (*it != background)
      removeLayer(*it);
}

// Adds a new image in the stock. Returns the image index in the
// stock.
int DocumentApi::addImageInStock(Sprite* sprite, Image* image)
{
  ASSERT(image != NULL);

  // Do the action.
  int imageIndex = sprite->getStock()->addImage(image);

  // Add undoers.
  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::AddImage(getObjects(),
        sprite->getStock(), imageIndex));

  return imageIndex;
}

// Removes and destroys the specified image in the stock.
void DocumentApi::removeImageFromStock(Sprite* sprite, int imageIndex)
{
  ASSERT(imageIndex >= 0);

  Image* image = sprite->getStock()->getImage(imageIndex);
  ASSERT(image);

  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::RemoveImage(getObjects(),
        sprite->getStock(), imageIndex));

  sprite->getStock()->removeImage(image);
  delete image;
}

void DocumentApi::replaceStockImage(Sprite* sprite, int imageIndex, Image* newImage)
{
  // Get the current image in the 'image_index' position.
  Image* oldImage = sprite->getStock()->getImage(imageIndex);
  ASSERT(oldImage);

  // Replace the image in the stock.
  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::ReplaceImage(getObjects(),
        sprite->getStock(), imageIndex));

  sprite->getStock()->replaceImage(imageIndex, newImage);
  delete oldImage;
}

Image* DocumentApi::getCelImage(Sprite* sprite, Cel* cel)
{
  if (cel && cel->getImage() >= 0 && cel->getImage() < sprite->getStock()->size())
    return sprite->getStock()->getImage(cel->getImage());
  else
    return NULL;
}

// Clears the mask region in the current sprite with the specified background color.
void DocumentApi::clearMask(Layer* layer, Cel* cel, int bgcolor)
{
  Image* image = getCelImage(layer->getSprite(), cel);
  if (!image)
    return;

  Mask* mask = m_document->getMask();

  // If the mask is empty or is not visible then we have to clear the
  // entire image in the cel.
  if (!m_document->isMaskVisible()) {
    // If the layer is the background then we clear the image.
    if (layer->isBackground()) {
      if (undoEnabled())
        m_undoers->pushUndoer(new undoers::ImageArea(getObjects(),
          image, 0, 0, image->getWidth(), image->getHeight()));

      // clear all
      clear_image(image, bgcolor);
    }
    // If the layer is transparent we can remove the cel (and its
    // associated image).
    else {
      ASSERT(layer->isImage());
      removeCel(static_cast<LayerImage*>(layer), cel);
    }
  }
  else {
    int offset_x = mask->getBounds().x-cel->getX();
    int offset_y = mask->getBounds().y-cel->getY();
    int u, v, putx, puty;
    int x1 = MAX(0, offset_x);
    int y1 = MAX(0, offset_y);
    int x2 = MIN(image->getWidth()-1, offset_x+mask->getBounds().w-1);
    int y2 = MIN(image->getHeight()-1, offset_y+mask->getBounds().h-1);

    // Do nothing
    if (x1 > x2 || y1 > y2)
      return;

    if (undoEnabled())
      m_undoers->pushUndoer(new undoers::ImageArea(getObjects(),
          image, x1, y1, x2-x1+1, y2-y1+1));

    const LockImageBits<BitmapTraits> maskBits(mask->getBitmap());
    LockImageBits<BitmapTraits>::const_iterator it = maskBits.begin();

    // Clear the masked zones
    for (v=0; v<mask->getBounds().h; ++v) {
      for (u=0; u<mask->getBounds().w; ++u, ++it) {
        ASSERT(it != maskBits.end());
        if (*it) {
          putx = u + offset_x;
          puty = v + offset_y;
          put_pixel(image, putx, puty, bgcolor);
        }
      }
    }

    ASSERT(it == maskBits.end());
  }
}

void DocumentApi::flipImage(Image* image, const gfx::Rect& bounds,
  raster::algorithm::FlipType flipType)
{
  // Insert the undo operation.
  if (undoEnabled()) {
    m_undoers->pushUndoer
      (new undoers::FlipImage
       (getObjects(), image, bounds, flipType));
  }

  // Flip the portion of the bitmap.
  raster::algorithm::flip_image(image, bounds, flipType);
}

void DocumentApi::flipImageWithMask(Image* image, const Mask* mask, raster::algorithm::FlipType flipType, int bgcolor)
{
  base::UniquePtr<Image> flippedImage((Image::createCopy(image)));

  // Flip the portion of the bitmap.
  raster::algorithm::flip_image_with_mask(flippedImage, mask, flipType, bgcolor);

  // Insert the undo operation.
  if (undoEnabled()) {
    base::UniquePtr<Dirty> dirty((new Dirty(image, flippedImage, image->getBounds())));
    dirty->saveImagePixels(image);

    m_undoers->pushUndoer(new undoers::DirtyArea(getObjects(), image, dirty));
  }

  // Copy the flipped image into the image specified as argument.
  copy_image(image, flippedImage, 0, 0);
}

void DocumentApi::pasteImage(Sprite* sprite, Cel* cel, const Image* src_image, int x, int y, int opacity)
{
  ASSERT(cel != NULL);

  Image* cel_image = sprite->getStock()->getImage(cel->getImage());
  Image* cel_image2 = Image::createCopy(cel_image);
  composite_image(cel_image2, src_image, x-cel->getX(), y-cel->getY(), opacity, BLEND_MODE_NORMAL);

  replaceStockImage(sprite, cel->getImage(), cel_image2); // TODO fix this, improve, avoid replacing the whole image
}

void DocumentApi::copyToCurrentMask(Mask* mask)
{
  ASSERT(m_document->getMask());
  ASSERT(mask);

  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::SetMask(getObjects(),
        m_document));

  m_document->getMask()->copyFrom(mask);
}

void DocumentApi::setMaskPosition(int x, int y)
{
  ASSERT(m_document->getMask());

  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::SetMaskPosition(getObjects(), m_document));

  m_document->getMask()->setOrigin(x, y);
  m_document->resetTransformation();
}

void DocumentApi::deselectMask()
{
  if (undoEnabled())
    m_undoers->pushUndoer(new undoers::SetMask(getObjects(),
        m_document));

  m_document->setMaskVisible(false);
}

void DocumentApi::setPalette(Sprite* sprite, FrameNumber frame, Palette* newPalette)
{
  Palette* currentSpritePalette = sprite->getPalette(frame); // Sprite current pal
  int from, to;

  // Check differences between current sprite palette and current system palette
  from = to = -1;
  currentSpritePalette->countDiff(newPalette, &from, &to);

  if (from >= 0 && to >= from) {
    // Add undo information to save the range of pal entries that will be modified.
    if (undoEnabled()) {
      m_undoers->pushUndoer
        (new undoers::SetPaletteColors(getObjects(),
                                       sprite, currentSpritePalette,
                                       frame, from, to));
    }

    // Change the sprite palette
    sprite->setPalette(newPalette, false);
  }
}

bool DocumentApi::undoEnabled()
{
  return
    m_undoers != NULL &&
    m_document->getUndo()->isEnabled();
}

} // namespace app
