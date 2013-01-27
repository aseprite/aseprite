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

#include "config.h"

#include "undo_transaction.h"

#include "base/unique_ptr.h"
#include "document.h"
#include "document_event.h"
#include "document_observer.h"
#include "document_undo.h"
#include "raster/algorithm/flip_image.h"
#include "raster/blend.h"
#include "raster/cel.h"
#include "raster/dirty.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/palette.h"
#include "raster/quantization.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "undo/undo_history.h"
#include "undoers/add_cel.h"
#include "undoers/add_image.h"
#include "undoers/add_layer.h"
#include "undoers/add_palette.h"
#include "undoers/close_group.h"
#include "undoers/dirty_area.h"
#include "undoers/flip_image.h"
#include "undoers/image_area.h"
#include "undoers/move_layer.h"
#include "undoers/open_group.h"
#include "undoers/remove_cel.h"
#include "undoers/remove_image.h"
#include "undoers/remove_layer.h"
#include "undoers/remove_palette.h"
#include "undoers/replace_image.h"
#include "undoers/set_cel_frame.h"
#include "undoers/set_cel_position.h"
#include "undoers/set_current_frame.h"
#include "undoers/set_current_layer.h"
#include "undoers/set_frame_duration.h"
#include "undoers/set_layer_flags.h"
#include "undoers/set_layer_name.h"
#include "undoers/set_mask.h"
#include "undoers/set_mask_position.h"
#include "undoers/set_sprite_pixel_format.h"
#include "undoers/set_sprite_size.h"
#include "undoers/set_stock_pixel_format.h"
#include "undoers/set_total_frames.h"

UndoTransaction::UndoTransaction(Document* document, const char* label, undo::Modification modification)
  : m_label(label)
  , m_modification(modification)
{
  ASSERT(label != NULL);

  m_document = document;
  m_sprite = document->getSprite();
  m_undo = document->getUndo();
  m_closed = false;
  m_committed = false;
  m_enabledFlag = m_undo->isEnabled();

  if (isEnabled())
    m_undo->pushUndoer(new undoers::OpenGroup(getObjects(),
                                              m_label,
                                              m_modification,
                                              m_sprite));
}

UndoTransaction::~UndoTransaction()
{
  try {
    if (isEnabled()) {
      // If it isn't committed, we have to rollback all changes.
      if (!m_committed && !m_closed)
        rollback();

      ASSERT(m_closed);
    }
  }
  catch (...) {
    // Just avoid throwing an exception in the dtor (just in case
    // rollback() failed).

    // TODO logging error
  }
}

void UndoTransaction::closeUndoGroup()
{
  ASSERT(!m_closed);

  if (isEnabled()) {
    // Close the undo information.
    m_undo->pushUndoer(new undoers::CloseGroup(getObjects(),
                                               m_label,
                                               m_modification,
                                               m_sprite));
    m_closed = true;
  }
}

void UndoTransaction::commit()
{
  ASSERT(!m_closed);
  ASSERT(!m_committed);

  if (isEnabled()) {
    closeUndoGroup();

    m_committed = true;
  }
}

void UndoTransaction::rollback()
{
  ASSERT(!m_closed);
  ASSERT(!m_committed);

  if (isEnabled()) {
    closeUndoGroup();

    // Undo the group of operations.
    m_undo->doUndo();

    // Clear the redo (sorry to the user, here we lost the old redo
    // information).
    m_undo->clearRedo();
  }
}

void UndoTransaction::pushUndoer(undo::Undoer* undoer)
{
  m_undo->pushUndoer(undoer);
}

undo::ObjectsContainer* UndoTransaction::getObjects() const
{
  return m_undo->getObjects();
}

void UndoTransaction::setNumberOfFrames(FrameNumber frames)
{
  ASSERT(frames >= 1);

  // Save in undo the current totalFrames property
  if (isEnabled())
    m_undo->pushUndoer(new undoers::SetTotalFrames(m_undo->getObjects(), m_sprite));

  // Change the property
  m_sprite->setTotalFrames(frames);
}

void UndoTransaction::setCurrentFrame(FrameNumber frame)
{
  ASSERT(frame >= 0);

  if (isEnabled())
    m_undo->pushUndoer(new undoers::SetCurrentFrame(m_undo->getObjects(), m_sprite));

  m_sprite->setCurrentFrame(frame);
}

/**
 * Sets the current selected layer in the sprite.
 *
 * @param layer
 *   The layer to select. Can be NULL.
 */
void UndoTransaction::setCurrentLayer(Layer* layer)
{
  if (isEnabled())
    m_undo->pushUndoer(new undoers::SetCurrentLayer(
        m_undo->getObjects(), m_sprite));

  m_sprite->setCurrentLayer(layer);
}

void UndoTransaction::setSpriteSize(int w, int h)
{
  ASSERT(w > 0);
  ASSERT(h > 0);

  if (isEnabled())
    m_undo->pushUndoer(new undoers::SetSpriteSize(m_undo->getObjects(), m_sprite));

  m_sprite->setSize(w, h);

  DocumentEvent ev(m_document, m_sprite);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onSpriteSizeChanged, ev);
}

void UndoTransaction::cropSprite(const gfx::Rect& bounds, int bgcolor)
{
  setSpriteSize(bounds.w, bounds.h);

  displaceLayers(m_sprite->getFolder(), -bounds.x, -bounds.y);

  Layer *background_layer = m_sprite->getBackgroundLayer();
  if (background_layer)
    cropLayer(background_layer, 0, 0, m_sprite->getWidth(), m_sprite->getHeight(), bgcolor);

  if (!m_document->getMask()->isEmpty())
    setMaskPosition(m_document->getMask()->getBounds().x-bounds.x,
                    m_document->getMask()->getBounds().y-bounds.y);
}

void UndoTransaction::trimSprite(int bgcolor)
{
  FrameNumber old_frame = m_sprite->getCurrentFrame();
  gfx::Rect bounds;

  UniquePtr<Image> image_wrap(Image::create(m_sprite->getPixelFormat(),
                                            m_sprite->getWidth(),
                                            m_sprite->getHeight()));
  Image* image = image_wrap.get();

  for (FrameNumber frame(0); frame<m_sprite->getTotalFrames(); ++frame) {
    image->clear(0);

    m_sprite->setCurrentFrame(frame);
    m_sprite->render(image, 0, 0);

    // TODO configurable (what color pixel to use as "refpixel",
    // here we are using the top-left pixel by default)
    gfx::Rect frameBounds;
    if (image_shrink_rect(image, frameBounds, image_getpixel(image, 0, 0)))
      bounds = bounds.createUnion(frameBounds);
  }
  m_sprite->setCurrentFrame(old_frame);

  if (!bounds.isEmpty())
    cropSprite(bounds, bgcolor);
}

void UndoTransaction::setPixelFormat(PixelFormat newFormat, DitheringMethod dithering_method)
{
  Image *old_image;
  Image *new_image;
  int c;

  if (m_sprite->getPixelFormat() == newFormat)
    return;

  // Change pixel format of the stock of images.
  if (isEnabled())
    m_undo->pushUndoer(new undoers::SetStockPixelFormat(m_undo->getObjects(), m_sprite->getStock()));

  m_sprite->getStock()->setPixelFormat(newFormat);

  // Use the rgbmap for the specified sprite
  const RgbMap* rgbmap = m_sprite->getRgbMap();

  for (c=0; c<m_sprite->getStock()->size(); c++) {
    old_image = m_sprite->getStock()->getImage(c);
    if (!old_image)
      continue;

    new_image = quantization::convert_pixel_format(old_image, newFormat, dithering_method, rgbmap,
                                                   // TODO check this out
                                                   m_sprite->getCurrentPalette(),
                                                   m_sprite->getBackgroundLayer() != NULL);

    this->replaceStockImage(c, new_image);
  }

  // Change sprite's pixel format.
  if (isEnabled())
    m_undo->pushUndoer(new undoers::SetSpritePixelFormat(m_undo->getObjects(), m_sprite));

  m_sprite->setPixelFormat(newFormat);

  // Regenerate extras
  m_document->destroyExtraCel();

  // When we are converting to grayscale color mode, we've to destroy
  // all palettes and put only one grayscaled-palette at the first
  // frame.
  if (newFormat == IMAGE_GRAYSCALE) {
    // Add undoers to revert all palette changes.
    if (isEnabled()) {
      PalettesList palettes = m_sprite->getPalettes();
      for (PalettesList::iterator it = palettes.begin(); it != palettes.end(); ++it) {
        Palette* palette = *it;
        m_undo->pushUndoer(new undoers::RemovePalette(
            m_undo->getObjects(), m_sprite, palette->getFrame()));
      }

      m_undo->pushUndoer(new undoers::AddPalette(
        m_undo->getObjects(), m_sprite, FrameNumber(0)));
    }

    // It's a UniquePtr because setPalette'll create a copy of "graypal".
    UniquePtr<Palette> graypal(Palette::createGrayscale());

    m_sprite->resetPalettes();
    m_sprite->setPalette(graypal, true);
  }
}

/**
 * Adds a new image in the stock.
 *
 * @return
 *   The image index in the stock.
 */
int UndoTransaction::addImageInStock(Image* image)
{
  ASSERT(image);

  // add the image in the stock
  int image_index = m_sprite->getStock()->addImage(image);

  if (isEnabled())
    m_undo->pushUndoer(new undoers::AddImage(m_undo->getObjects(),
        m_sprite->getStock(), image_index));

  return image_index;
}

/**
 * Removes and destroys the specified image in the stock.
 */
void UndoTransaction::removeImageFromStock(int image_index)
{
  ASSERT(image_index >= 0);

  Image* image = m_sprite->getStock()->getImage(image_index);
  ASSERT(image);

  if (isEnabled())
    m_undo->pushUndoer(new undoers::RemoveImage(m_undo->getObjects(),
        m_sprite->getStock(), image_index));

  m_sprite->getStock()->removeImage(image);
  image_free(image);
}

void UndoTransaction::replaceStockImage(int image_index, Image* new_image)
{
  // get the current image in the 'image_index' position
  Image* old_image = m_sprite->getStock()->getImage(image_index);
  ASSERT(old_image);

  // replace the image in the stock
  if (isEnabled())
    m_undo->pushUndoer(new undoers::ReplaceImage(m_undo->getObjects(),
        m_sprite->getStock(), image_index));

  m_sprite->getStock()->replaceImage(image_index, new_image);

  // destroy the old image
  image_free(old_image);

  DocumentEvent ev(m_document, m_sprite, NULL, NULL, new_image, image_index);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onImageReplaced, ev);
}

/**
 * Creates a new transparent layer.
 */
LayerImage* UndoTransaction::newLayer()
{
  // new layer
  LayerImage* layer = new LayerImage(m_sprite);

  // add the layer in the sprite set
  if (isEnabled())
    m_undo->pushUndoer(new undoers::AddLayer(m_undo->getObjects(),
        m_sprite->getFolder(), layer));

  m_sprite->getFolder()->addLayer(layer);

  // select the new layer
  setCurrentLayer(layer);

  DocumentEvent ev(m_document, m_sprite, layer);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onAddLayer, ev);

  return layer;
}

/**
 * Removes and destroys the specified layer.
 */
void UndoTransaction::removeLayer(Layer* layer)
{
  ASSERT(layer);

  DocumentEvent ev(m_document, m_sprite, layer);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onRemoveLayer, ev);

  LayerFolder* parent = layer->getParent();

  // if the layer to be removed is the selected layer
  if (layer == m_sprite->getCurrentLayer()) {
    Layer* layer_select = NULL;

    // select: previous layer, or next layer, or parent(if it is not the
    // main layer of sprite set)
    if (layer->getPrevious())
      layer_select = layer->getPrevious();
    else if (layer->getNext())
      layer_select = layer->getNext();
    else if (parent != m_sprite->getFolder())
      layer_select = parent;

    // select other layer
    setCurrentLayer(layer_select);
  }

  // remove the layer
  if (isEnabled())
    m_undo->pushUndoer(new undoers::RemoveLayer(m_undo->getObjects(),
        layer));

  parent->removeLayer(layer);

  // destroy the layer
  delete layer;
}

void UndoTransaction::restackLayerAfter(Layer* layer, Layer* afterThis)
{
  if (isEnabled())
    m_undo->pushUndoer(new undoers::MoveLayer(m_undo->getObjects(), layer));

  layer->getParent()->stackLayer(layer, afterThis);

  DocumentEvent ev(m_document, m_sprite, layer);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onLayerRestacked, ev);
}

void UndoTransaction::cropLayer(Layer* layer, int x, int y, int w, int h, int bgcolor)
{
  if (!layer->isImage())
    return;

  if (!layer->isBackground())
    bgcolor = 0;

  CelIterator it = ((LayerImage*)layer)->getCelBegin();
  CelIterator end = ((LayerImage*)layer)->getCelEnd();
  for (; it != end; ++it)
    cropCel(*it, x, y, w, h, bgcolor);
}

/**
 * Moves every frame in @a layer with the offset (@a dx, @a dy).
 */
void UndoTransaction::displaceLayers(Layer* layer, int dx, int dy)
{
  switch (layer->getType()) {

    case GFXOBJ_LAYER_IMAGE: {
      CelIterator it = ((LayerImage*)layer)->getCelBegin();
      CelIterator end = ((LayerImage*)layer)->getCelEnd();
      for (; it != end; ++it) {
        Cel* cel = *it;
        setCelPosition(cel, cel->getX()+dx, cel->getY()+dy);
      }
      break;
    }

    case GFXOBJ_LAYER_FOLDER: {
      LayerIterator it = ((LayerFolder*)layer)->getLayerBegin();
      LayerIterator end = ((LayerFolder*)layer)->getLayerEnd();
      for (; it != end; ++it)
        displaceLayers(*it, dx, dy);
      break;
    }

  }
}

void UndoTransaction::backgroundFromLayer(LayerImage* layer, int bgcolor)
{
  ASSERT(layer);
  ASSERT(layer->isImage());
  ASSERT(layer->isReadable());
  ASSERT(layer->isWritable());
  ASSERT(layer->getSprite() == m_sprite);
  ASSERT(m_sprite->getBackgroundLayer() == NULL);

  // create a temporary image to draw each frame of the new
  // `Background' layer
  UniquePtr<Image> bg_image_wrap(Image::create(m_sprite->getPixelFormat(),
                                               m_sprite->getWidth(),
                                               m_sprite->getHeight()));
  Image* bg_image = bg_image_wrap.get();

  CelIterator it = layer->getCelBegin();
  CelIterator end = layer->getCelEnd();

  for (; it != end; ++it) {
    Cel* cel = *it;
    ASSERT((cel->getImage() > 0) &&
           (cel->getImage() < m_sprite->getStock()->size()));

    // get the image from the sprite's stock of images
    Image* cel_image = m_sprite->getStock()->getImage(cel->getImage());
    ASSERT(cel_image);

    image_clear(bg_image, bgcolor);
    image_merge(bg_image, cel_image,
                cel->getX(),
                cel->getY(),
                MID(0, cel->getOpacity(), 255),
                layer->getBlendMode());

    // now we have to copy the new image (bg_image) to the cel...
    setCelPosition(cel, 0, 0);

    // same size of cel-image and bg-image
    if (bg_image->w == cel_image->w &&
        bg_image->h == cel_image->h) {
      if (isEnabled())
        m_undo->pushUndoer(new undoers::ImageArea(m_undo->getObjects(),
            cel_image, 0, 0, cel_image->w, cel_image->h));

      image_copy(cel_image, bg_image, 0, 0);
    }
    else {
      replaceStockImage(cel->getImage(), Image::createCopy(bg_image));
    }
  }

  // Fill all empty cels with a flat-image filled with bgcolor
  for (FrameNumber frame(0); frame<m_sprite->getTotalFrames(); ++frame) {
    Cel* cel = layer->getCel(frame);
    if (!cel) {
      Image* cel_image = Image::create(m_sprite->getPixelFormat(), m_sprite->getWidth(), m_sprite->getHeight());
      image_clear(cel_image, bgcolor);

      // Add the new image in the stock
      int image_index = addImageInStock(cel_image);

      // Create the new cel and add it to the new background layer
      cel = new Cel(frame, image_index);
      addCel(layer, cel);
    }
  }

  configureLayerAsBackground(layer);
}

void UndoTransaction::layerFromBackground()
{
  ASSERT(m_sprite->getBackgroundLayer() != NULL);
  ASSERT(m_sprite->getCurrentLayer() != NULL);
  ASSERT(m_sprite->getCurrentLayer()->isImage());
  ASSERT(m_sprite->getCurrentLayer()->isReadable());
  ASSERT(m_sprite->getCurrentLayer()->isWritable());
  ASSERT(m_sprite->getCurrentLayer()->isBackground());

  if (isEnabled()) {
    m_undo->pushUndoer(new undoers::SetLayerFlags(m_undo->getObjects(), m_sprite->getCurrentLayer()));
    m_undo->pushUndoer(new undoers::SetLayerName(m_undo->getObjects(), m_sprite->getCurrentLayer()));
  }

  m_sprite->getCurrentLayer()->setBackground(false);
  m_sprite->getCurrentLayer()->setMoveable(true);
  m_sprite->getCurrentLayer()->setName("Layer 0");
}

void UndoTransaction::flattenLayers(int bgcolor)
{
  Image* cel_image;
  Cel* cel;

  // create a temporary image
  UniquePtr<Image> image_wrap(Image::create(m_sprite->getPixelFormat(),
                                            m_sprite->getWidth(),
                                            m_sprite->getHeight()));
  Image* image = image_wrap.get();

  /* get the background layer from the sprite */
  LayerImage* background = m_sprite->getBackgroundLayer();
  if (!background) {
    /* if there aren't a background layer we must to create the background */
    background = new LayerImage(m_sprite);

    if (isEnabled())
      m_undo->pushUndoer(new undoers::AddLayer(m_undo->getObjects(),
          m_sprite->getFolder(), background));

    m_sprite->getFolder()->addLayer(background);

    if (isEnabled())
      m_undo->pushUndoer(new undoers::MoveLayer(m_undo->getObjects(),
          background));

    background->configureAsBackground();
  }

  /* copy all frames to the background */
  for (FrameNumber frame(0); frame<m_sprite->getTotalFrames(); ++frame) {
    /* clear the image and render this frame */
    image_clear(image, bgcolor);
    layer_render(m_sprite->getFolder(), image, 0, 0, frame);

    cel = background->getCel(frame);
    if (cel) {
      cel_image = m_sprite->getStock()->getImage(cel->getImage());
      ASSERT(cel_image != NULL);

      /* we have to save the current state of `cel_image' in the undo */
      if (isEnabled()) {
        Dirty* dirty = new Dirty(cel_image, image);
        dirty->saveImagePixels(cel_image);
        m_undo->pushUndoer(new undoers::DirtyArea(
            m_undo->getObjects(), cel_image, dirty));
        delete dirty;
      }
    }
    else {
      /* if there aren't a cel in this frame in the background, we
         have to create a copy of the image for the new cel */
      cel_image = Image::createCopy(image);
      /* TODO error handling: if (!cel_image) { ... } */

      /* here we create the new cel (with the new image `cel_image') */
      cel = new Cel(frame, m_sprite->getStock()->addImage(cel_image));
      /* TODO error handling: if (!cel) { ... } */

      /* and finally we add the cel in the background */
      background->addCel(cel);
    }

    image_copy(cel_image, image, 0, 0);
  }

  /* select the background */
  if (m_sprite->getCurrentLayer() != background) {
    if (isEnabled())
      m_undo->pushUndoer(new undoers::SetCurrentLayer(
          m_undo->getObjects(), m_sprite));

    m_sprite->setCurrentLayer(background);
  }

  // Remove old layers.
  LayerList layers = m_sprite->getFolder()->getLayersList();
  LayerIterator it = layers.begin();
  LayerIterator end = layers.end();

  for (; it != end; ++it) {
    if (*it != background) {
      Layer* old_layer = *it;

      // Remove the layer
      if (isEnabled())
        m_undo->pushUndoer(new undoers::RemoveLayer(m_undo->getObjects(),
            old_layer));

      m_sprite->getFolder()->removeLayer(old_layer);

      // Destroy the layer
      delete old_layer;
    }
  }
}

void UndoTransaction::configureLayerAsBackground(LayerImage* layer)
{
  if (isEnabled()) {
    m_undo->pushUndoer(new undoers::SetLayerFlags(m_undo->getObjects(), layer));
    m_undo->pushUndoer(new undoers::SetLayerName(m_undo->getObjects(), layer));
    m_undo->pushUndoer(new undoers::MoveLayer(m_undo->getObjects(), layer));
  }

  layer->configureAsBackground();
}

void UndoTransaction::newFrame()
{
  FrameNumber newFrame = m_sprite->getCurrentFrame().next();

  // add a new cel to every layer
  newFrameForLayer(m_sprite->getFolder(), newFrame);

  // increment frames counter in the sprite
  setNumberOfFrames(m_sprite->getTotalFrames() + FrameNumber(1));

  // go to next frame (the new one)
  setCurrentFrame(newFrame);

  DocumentEvent ev(m_document, m_sprite, NULL, NULL, NULL, -1, newFrame);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onAddFrame, ev);
}

void UndoTransaction::newFrameForLayer(Layer* layer, FrameNumber frame)
{
  ASSERT(layer);
  ASSERT(frame >= 0);

  switch (layer->getType()) {

    case GFXOBJ_LAYER_IMAGE:
      // displace all cels in '>=frame' to the next frame
      for (FrameNumber c=m_sprite->getLastFrame(); c>=frame; --c) {
        Cel* cel = static_cast<LayerImage*>(layer)->getCel(c);
        if (cel)
          setCelFramePosition(cel, cel->getFrame().next());
      }

      copyPreviousFrame(layer, frame);
      break;

    case GFXOBJ_LAYER_FOLDER: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->getLayerBegin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it)
        newFrameForLayer(*it, frame);
      break;
    }

  }
}

void UndoTransaction::removeFrame(FrameNumber frame)
{
  ASSERT(frame >= 0);

  DocumentEvent ev(m_document, m_sprite, NULL, NULL, NULL, -1, frame);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onRemoveFrame, ev);

  // Remove cels from this frame (and displace one position backward
  // all next frames)
  removeFrameOfLayer(m_sprite->getFolder(), frame);

  // New value for totalFrames propety
  FrameNumber newTotalFrames = m_sprite->getTotalFrames() - FrameNumber(1);

  // Move backward if we will be outside the range of frames
  if (m_sprite->getCurrentFrame() >= newTotalFrames)
    setCurrentFrame(newTotalFrames.previous());

  // Decrement frames counter in the sprite
  setNumberOfFrames(newTotalFrames);
}

void UndoTransaction::removeFrameOfLayer(Layer* layer, FrameNumber frame)
{
  ASSERT(layer);
  ASSERT(frame >= 0);

  switch (layer->getType()) {

    case GFXOBJ_LAYER_IMAGE:
      if (Cel* cel = static_cast<LayerImage*>(layer)->getCel(frame))
        removeCel(static_cast<LayerImage*>(layer), cel);

      for (++frame; frame<m_sprite->getTotalFrames(); ++frame)
        if (Cel* cel = static_cast<LayerImage*>(layer)->getCel(frame))
          setCelFramePosition(cel, cel->getFrame().previous());
      break;

    case GFXOBJ_LAYER_FOLDER: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->getLayerBegin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it)
        removeFrameOfLayer(*it, frame);
      break;
    }

  }
}

/**
 * Copies the previous cel of @a frame to @frame.
 */
void UndoTransaction::copyPreviousFrame(Layer* layer, FrameNumber frame)
{
  ASSERT(layer);
  ASSERT(frame > 0);

  // create a copy of the previous cel
  Cel* src_cel = static_cast<LayerImage*>(layer)->getCel(frame.previous());
  Image* src_image = src_cel ? m_sprite->getStock()->getImage(src_cel->getImage()):
                               NULL;

  // nothing to copy, it will be a transparent cel
  if (!src_image)
    return;

  // copy the image
  Image* dst_image = Image::createCopy(src_image);
  int image_index = addImageInStock(dst_image);

  // create the new cel
  Cel* dst_cel = new Cel(frame, image_index);
  if (src_cel) {                // copy the data from the previous cel
    dst_cel->setPosition(src_cel->getX(), src_cel->getY());
    dst_cel->setOpacity(src_cel->getOpacity());
  }

  // add the cel in the layer
  addCel(static_cast<LayerImage*>(layer), dst_cel);
}

void UndoTransaction::addCel(LayerImage* layer, Cel* cel)
{
  ASSERT(layer);
  ASSERT(cel);

  if (isEnabled())
    m_undo->pushUndoer(new undoers::AddCel(m_undo->getObjects(),
        layer, cel));

  layer->addCel(cel);

  DocumentEvent ev(m_document, m_sprite, layer, cel);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onAddCel, ev);
}

void UndoTransaction::removeCel(LayerImage* layer, Cel* cel)
{
  ASSERT(layer);
  ASSERT(cel);

  DocumentEvent ev(m_document, m_sprite, layer, cel);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onRemoveCel, ev);

  // find if the image that use the cel to remove, is used by
  // another cels
  bool used = false;
  for (FrameNumber frame(0); frame<m_sprite->getTotalFrames(); ++frame) {
    Cel* it = layer->getCel(frame);
    if (it && it != cel && it->getImage() == cel->getImage()) {
      used = true;
      break;
    }
  }

  // if the image is only used by this cel,
  // we can remove the image from the stock
  if (!used)
    removeImageFromStock(cel->getImage());

  if (isEnabled())
    m_undo->pushUndoer(new undoers::RemoveCel(m_undo->getObjects(),
        layer, cel));

  // remove the cel from the layer
  layer->removeCel(cel);

  // and here we destroy the cel
  delete cel;
}

void UndoTransaction::setCelFramePosition(Cel* cel, FrameNumber frame)
{
  ASSERT(cel);
  ASSERT(frame >= 0);

  if (isEnabled())
    m_undo->pushUndoer(new undoers::SetCelFrame(m_undo->getObjects(), cel));

  cel->setFrame(frame);

  DocumentEvent ev(m_document, m_sprite, NULL, cel);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onCelFrameChanged, ev);
}

void UndoTransaction::setCelPosition(Cel* cel, int x, int y)
{
  ASSERT(cel);

  if (isEnabled())
    m_undo->pushUndoer(new undoers::SetCelPosition(m_undo->getObjects(), cel));

  cel->setPosition(x, y);

  DocumentEvent ev(m_document, m_sprite, NULL, cel);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onCelPositionChanged, ev);
}

void UndoTransaction::setFrameDuration(FrameNumber frame, int msecs)
{
  if (isEnabled())
    m_undo->pushUndoer(new undoers::SetFrameDuration(
        m_undo->getObjects(), m_sprite, frame));

  m_sprite->setFrameDuration(frame, msecs);

  DocumentEvent ev(m_document, m_sprite, NULL, NULL, NULL, -1, frame);
  m_document->notifyObservers<DocumentEvent&>(&DocumentObserver::onFrameDurationChanged, ev);
}

void UndoTransaction::setConstantFrameRate(int msecs)
{
  if (isEnabled()) {
    for (FrameNumber fr(0); fr<m_sprite->getTotalFrames(); ++fr)
      m_undo->pushUndoer(new undoers::SetFrameDuration(
          m_undo->getObjects(), m_sprite, fr));
  }

  m_sprite->setDurationForAllFrames(msecs);
}

void UndoTransaction::moveFrameBefore(FrameNumber frame, FrameNumber beforeFrame)
{
  if (frame != beforeFrame &&
      frame >= 0 &&
      frame <= m_sprite->getLastFrame() &&
      beforeFrame >= 0 &&
      beforeFrame <= m_sprite->getLastFrame()) {
    // Change the frame-lengths...
    int frlen_aux = m_sprite->getFrameDuration(frame);

    // Moving the frame to the future.
    if (frame < beforeFrame) {
      for (FrameNumber c=frame; c<beforeFrame.previous(); ++c)
        setFrameDuration(c, m_sprite->getFrameDuration(c.next()));

      setFrameDuration(beforeFrame.previous(), frlen_aux);
    }
    // Moving the frame to the past.
    else if (beforeFrame < frame) {
      for (FrameNumber c=frame; c>beforeFrame; --c)
        setFrameDuration(c, m_sprite->getFrameDuration(c.previous()));

      setFrameDuration(beforeFrame, frlen_aux);
    }

    // change the cels of position...
    moveFrameBeforeLayer(m_sprite->getFolder(), frame, beforeFrame);
  }
}

void UndoTransaction::moveFrameBeforeLayer(Layer* layer, FrameNumber frame, FrameNumber beforeFrame)
{
  ASSERT(layer);

  switch (layer->getType()) {

    case GFXOBJ_LAYER_IMAGE: {
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
          setCelFramePosition(cel, newFrame);
      }
      break;
    }

    case GFXOBJ_LAYER_FOLDER: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->getLayerBegin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it)
        moveFrameBeforeLayer(*it, frame, beforeFrame);
      break;
    }

  }
}

Cel* UndoTransaction::getCurrentCel()
{
  if (m_sprite->getCurrentLayer() && m_sprite->getCurrentLayer()->isImage())
    return static_cast<LayerImage*>(m_sprite->getCurrentLayer())->getCel(m_sprite->getCurrentFrame());
  else
    return NULL;
}

void UndoTransaction::cropCel(Cel* cel, int x, int y, int w, int h, int bgcolor)
{
  Image* cel_image = m_sprite->getStock()->getImage(cel->getImage());
  ASSERT(cel_image);

  // create the new image through a crop
  Image* new_image = image_crop(cel_image, x-cel->getX(), y-cel->getY(), w, h, bgcolor);

  // replace the image in the stock that is pointed by the cel
  replaceStockImage(cel->getImage(), new_image);

  // update the cel's position
  setCelPosition(cel, x, y);
}

Image* UndoTransaction::getCelImage(Cel* cel)
{
  if (cel && cel->getImage() >= 0 && cel->getImage() < m_sprite->getStock()->size())
    return m_sprite->getStock()->getImage(cel->getImage());
  else
    return NULL;
}

// clears the mask region in the current sprite with the specified background color
void UndoTransaction::clearMask(int bgcolor)
{
  Cel* cel = getCurrentCel();
  if (!cel)
    return;

  Image* image = getCelImage(cel);
  if (!image)
    return;

  Mask* mask = m_document->getMask();

  // If the mask is empty or is not visible then we have to clear the
  // entire image in the cel.
  if (!m_document->isMaskVisible()) {
    // If the layer is the background then we clear the image.
    if (m_sprite->getCurrentLayer()->isBackground()) {
      if (isEnabled())
        m_undo->pushUndoer(new undoers::ImageArea(m_undo->getObjects(),
            image, 0, 0, image->w, image->h));

      // clear all
      image_clear(image, bgcolor);
    }
    // If the layer is transparent we can remove the cel (and its
    // associated image).
    else {
      removeCel(static_cast<LayerImage*>(m_sprite->getCurrentLayer()), cel);
    }
  }
  else {
    int offset_x = mask->getBounds().x-cel->getX();
    int offset_y = mask->getBounds().y-cel->getY();
    int u, v, putx, puty;
    int x1 = MAX(0, offset_x);
    int y1 = MAX(0, offset_y);
    int x2 = MIN(image->w-1, offset_x+mask->getBounds().w-1);
    int y2 = MIN(image->h-1, offset_y+mask->getBounds().h-1);

    // do nothing
    if (x1 > x2 || y1 > y2)
      return;

    if (isEnabled())
      m_undo->pushUndoer(new undoers::ImageArea(m_undo->getObjects(),
          image, x1, y1, x2-x1+1, y2-y1+1));

    // clear the masked zones
    for (v=0; v<mask->getBounds().h; v++) {
      div_t d = div(0, 8);
      uint8_t* address = ((uint8_t**)mask->getBitmap()->line)[v]+d.quot;

      for (u=0; u<mask->getBounds().w; u++) {
        if ((*address & (1<<d.rem))) {
          putx = u + offset_x;
          puty = v + offset_y;
          image_putpixel(image, putx, puty, bgcolor);
        }

        _image_bitmap_next_bit(d, address);
      }
    }
  }
}

void UndoTransaction::flipImage(Image* image,
                                const gfx::Rect& bounds,
                                raster::algorithm::FlipType flipType)
{
  // Insert the undo operation.
  if (isEnabled()) {
    m_undo->pushUndoer
      (new undoers::FlipImage
       (m_undo->getObjects(), image, bounds, flipType));
  }

  // Flip the portion of the bitmap.
  raster::algorithm::flip_image(image, bounds, flipType);
}

void UndoTransaction::flipImageWithMask(Image* image, const Mask* mask, raster::algorithm::FlipType flipType, int bgcolor)
{
  UniquePtr<Image> flippedImage((Image::createCopy(image)));

  // Flip the portion of the bitmap.
  raster::algorithm::flip_image_with_mask(flippedImage, mask, flipType, bgcolor);

  // Insert the undo operation.
  if (isEnabled()) {
    UniquePtr<Dirty> dirty((new Dirty(image, flippedImage)));
    dirty->saveImagePixels(image);

    m_undo->pushUndoer(new undoers::DirtyArea(m_undo->getObjects(), image, dirty));
  }

  // Copy the flipped image into the image specified as argument.
  image_copy(image, flippedImage, 0, 0);
}

void UndoTransaction::pasteImage(const Image* src_image, int x, int y, int opacity)
{
  const Layer* layer = m_sprite->getCurrentLayer();

  ASSERT(layer);
  ASSERT(layer->isImage());
  ASSERT(layer->isReadable());
  ASSERT(layer->isWritable());

  Cel* cel = ((LayerImage*)layer)->getCel(m_sprite->getCurrentFrame());
  ASSERT(cel);

  Image* cel_image = m_sprite->getStock()->getImage(cel->getImage());
  Image* cel_image2 = Image::createCopy(cel_image);
  image_merge(cel_image2, src_image, x-cel->getX(), y-cel->getY(), opacity, BLEND_MODE_NORMAL);

  replaceStockImage(cel->getImage(), cel_image2); // TODO fix this, improve, avoid replacing the whole image
}

void UndoTransaction::copyToCurrentMask(Mask* mask)
{
  ASSERT(m_document->getMask());
  ASSERT(mask);

  if (isEnabled())
    m_undo->pushUndoer(new undoers::SetMask(m_undo->getObjects(),
        m_document));

  m_document->getMask()->copyFrom(mask);
}

void UndoTransaction::setMaskPosition(int x, int y)
{
  ASSERT(m_document->getMask());

  if (isEnabled())
    m_undo->pushUndoer(new undoers::SetMaskPosition(m_undo->getObjects(), m_document));

  m_document->getMask()->setOrigin(x, y);
  m_document->resetTransformation();
}

void UndoTransaction::deselectMask()
{
  if (isEnabled())
    m_undo->pushUndoer(new undoers::SetMask(m_undo->getObjects(),
        m_document));

  m_document->setMaskVisible(false);
}
