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

#include "app/document.h"

#include "app/document_api.h"
#include "app/document_event.h"
#include "app/document_observer.h"
#include "app/document_undo.h"
#include "app/file/format_options.h"
#include "app/flatten.h"
#include "app/objects_container_impl.h"
#include "app/undoers/add_image.h"
#include "app/undoers/add_layer.h"
#include "app/util/boundary.h"
#include "base/memory.h"
#include "base/mutex.h"
#include "base/scoped_lock.h"
#include "base/unique_ptr.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "raster/stock.h"

namespace app {

using namespace base;
using namespace raster;

Document::Document(Sprite* sprite)
  : m_sprite(sprite)
  , m_undo(new DocumentUndo)
  , m_associated_to_file(false)
  , m_mutex(new mutex)
  , m_write_lock(false)
  , m_read_locks(0)
    // Information about the file format used to load/save this document
  , m_format_options(NULL)
    // Extra cel
  , m_extraCel(NULL)
  , m_extraImage(NULL)
  // Mask
  , m_mask(new Mask())
  , m_maskVisible(true)
{
  m_document.setFilename("Sprite");

  // Boundary stuff
  m_bound.nseg = 0;
  m_bound.seg = NULL;
}

Document::~Document()
{
  DocumentEvent ev(this);
  ev.sprite(m_sprite);
  notifyObservers<DocumentEvent&>(&DocumentObserver::onRemoveSprite, ev);

  if (m_bound.seg)
    base_free(m_bound.seg);

  destroyExtraCel();
}

DocumentApi Document::getApi(undo::UndoersCollector* undoers)
{
  return DocumentApi(this, undoers ? undoers: m_undo->getDefaultUndoersCollector());
}

Document* Document::createBasicDocument(PixelFormat format, int width, int height, int ncolors)
{
  // Create the sprite.
  base::UniquePtr<Sprite> sprite(new Sprite(format, width, height, ncolors));
  sprite->setTotalFrames(FrameNumber(1));

  // Create the main image.
  int indexInStock;
  {
    base::UniquePtr<Image> image(Image::create(format, width, height));

    // Clear the image with mask color.
    clear_image(image, 0);

    // Add image in the sprite's stock.
    indexInStock = sprite->getStock()->addImage(image);
    image.release();            // Release the image because it is in the sprite's stock.
  }

  // Create the first transparent layer.
  {
    base::UniquePtr<LayerImage> layer(new LayerImage(sprite));
    layer->setName("Layer 1");

    // Create the cel.
    {
      base::UniquePtr<Cel> cel(new Cel(FrameNumber(0), indexInStock));
      cel->setPosition(0, 0);

      // Add the cel in the layer.
      layer->addCel(cel);
      cel.release();            // Release the cel because it is in the layer
    }

    // Add the layer in the sprite.
    sprite->getFolder()->addLayer(layer.release()); // Release the layer because it's owned by the sprite
  }

  // Create the document with the new sprite.
  base::UniquePtr<Document> document(new Document(sprite));
  sprite.release();             // Release the sprite because it is in the document.

  document->setFilename("Sprite");
  return document.release();    // Release the document (it does not throw) returning the raw pointer
}

void Document::addSprite(Sprite* sprite)
{
  ASSERT(m_sprite == NULL);     // TODO add support for more sprites in the future (e.g. for .ico files)
  m_sprite.reset(sprite);

  DocumentEvent ev(this);
  ev.sprite(m_sprite);
  notifyObservers<DocumentEvent&>(&DocumentObserver::onAddSprite, ev);
}

void Document::notifyGeneralUpdate()
{
  DocumentEvent ev(this);
  notifyObservers<DocumentEvent&>(&DocumentObserver::onGeneralUpdate, ev);
}

void Document::notifySpritePixelsModified(Sprite* sprite, const gfx::Region& region)
{
  DocumentEvent ev(this);
  ev.sprite(sprite);
  ev.region(region);
  notifyObservers<DocumentEvent&>(&DocumentObserver::onSpritePixelsModified, ev);
}

void Document::notifyLayerMergedDown(Layer* srcLayer, Layer* targetLayer)
{
  DocumentEvent ev(this);
  ev.sprite(srcLayer->getSprite());
  ev.layer(srcLayer);
  ev.targetLayer(targetLayer);
  notifyObservers<DocumentEvent&>(&DocumentObserver::onLayerMergedDown, ev);
}

void Document::notifyCelMoved(Layer* fromLayer, FrameNumber fromFrame, Layer* toLayer, FrameNumber toFrame)
{
  DocumentEvent ev(this);
  ev.sprite(fromLayer->getSprite());
  ev.layer(fromLayer);
  ev.frame(fromFrame);
  ev.targetLayer(toLayer);
  ev.targetFrame(toFrame);
  notifyObservers<DocumentEvent&>(&DocumentObserver::onCelMoved, ev);
}

void Document::notifyCelCopied(Layer* fromLayer, FrameNumber fromFrame, Layer* toLayer, FrameNumber toFrame)
{
  DocumentEvent ev(this);
  ev.sprite(fromLayer->getSprite());
  ev.layer(fromLayer);
  ev.frame(fromFrame);
  ev.targetLayer(toLayer);
  ev.targetFrame(toFrame);
  notifyObservers<DocumentEvent&>(&DocumentObserver::onCelCopied, ev);
}

void Document::setFilename(const base::string& filename)
{
  m_document.setFilename(filename);
}

bool Document::isModified() const
{
  return !m_undo->isSavedState();
}

bool Document::isAssociatedToFile() const
{
  return m_associated_to_file;
}

void Document::markAsSaved()
{
  m_undo->markSavedState();
  m_associated_to_file = true;
}

//////////////////////////////////////////////////////////////////////
// Loaded options from file

void Document::setFormatOptions(const SharedPtr<FormatOptions>& format_options)
{
  m_format_options = format_options;
}

//////////////////////////////////////////////////////////////////////
// Boundaries

int Document::getBoundariesSegmentsCount() const
{
  return m_bound.nseg;
}

const BoundSeg* Document::getBoundariesSegments() const
{
  return m_bound.seg;
}

void Document::generateMaskBoundaries(Mask* mask)
{
  if (m_bound.seg) {
    base_free(m_bound.seg);
    m_bound.seg = NULL;
    m_bound.nseg = 0;
  }

  // No mask specified? Use the current one in the document
  if (!mask) {
    if (!isMaskVisible())       // The mask is hidden
      return;                   // Done, without boundaries
    else
      mask = getMask();         // Use the document mask
  }

  ASSERT(mask != NULL);

  if (!mask->isEmpty()) {
    m_bound.seg = find_mask_boundary(mask->getBitmap(),
                                     &m_bound.nseg,
                                     IgnoreBounds, 0, 0, 0, 0);
    for (int c=0; c<m_bound.nseg; c++) {
      m_bound.seg[c].x1 += mask->getBounds().x;
      m_bound.seg[c].y1 += mask->getBounds().y;
      m_bound.seg[c].x2 += mask->getBounds().x;
      m_bound.seg[c].y2 += mask->getBounds().y;
    }
  }
}

//////////////////////////////////////////////////////////////////////
// Extra Cel (it is used to draw pen preview, pixels in movement, etc.)

void Document::destroyExtraCel()
{
  delete m_extraCel;
  delete m_extraImage;

  m_extraCel = NULL;
  m_extraImage = NULL;
}

void Document::prepareExtraCel(int x, int y, int w, int h, int opacity)
{
  ASSERT(getSprite() != NULL);

  if (!m_extraCel)
    m_extraCel = new Cel(FrameNumber(0), 0); // Ignored fields for this cell (frame, and image index)

  m_extraCel->setPosition(x, y);
  m_extraCel->setOpacity(opacity);

  if (!m_extraImage ||
      m_extraImage->getPixelFormat() != getSprite()->getPixelFormat() ||
      m_extraImage->getWidth() != w ||
      m_extraImage->getHeight() != h) {
    delete m_extraImage;                // image
    m_extraImage = Image::create(getSprite()->getPixelFormat(), w, h);
    m_extraImage->setMaskColor(0);
    clear_image(m_extraImage, m_extraImage->getMaskColor());
  }
}

Cel* Document::getExtraCel() const
{
  return m_extraCel;
}

Image* Document::getExtraCelImage() const
{
  return m_extraImage;
}

//////////////////////////////////////////////////////////////////////
// Mask

Mask* Document::getMask() const
{
  return m_mask;
}

void Document::setMask(const Mask* mask)
{
  m_mask.reset(new Mask(*mask));
  m_maskVisible = true;

  resetTransformation();
}

bool Document::isMaskVisible() const
{
  return
    m_maskVisible &&            // The mask was not hidden by the user explicitly
    m_mask &&                   // The mask does exist
    !m_mask->isEmpty();         // The mask is not empty
}

void Document::setMaskVisible(bool visible)
{
  m_maskVisible = visible;
}

//////////////////////////////////////////////////////////////////////
// Transformation

gfx::Transformation Document::getTransformation() const
{
  return m_transformation;
}

void Document::setTransformation(const gfx::Transformation& transform)
{
  m_transformation = transform;
}

void Document::resetTransformation()
{
  if (m_mask)
    m_transformation = gfx::Transformation(m_mask->getBounds());
  else
    m_transformation = gfx::Transformation();
}

//////////////////////////////////////////////////////////////////////
// Copying

void Document::copyLayerContent(const Layer* sourceLayer0, Document* destDoc, Layer* destLayer0) const
{
  DocumentUndo* undo = destDoc->getUndo();

  // Copy the layer name
  destLayer0->setName(sourceLayer0->getName());

  if (sourceLayer0->isImage() && destLayer0->isImage()) {
    const LayerImage* sourceLayer = static_cast<const LayerImage*>(sourceLayer0);
    LayerImage* destLayer = static_cast<LayerImage*>(destLayer0);

    // copy cels
    CelConstIterator it = sourceLayer->getCelBegin();
    CelConstIterator end = sourceLayer->getCelEnd();

    for (; it != end; ++it) {
      const Cel* sourceCel = *it;
      base::UniquePtr<Cel> newCel(new Cel(*sourceCel));

      ASSERT((sourceCel->getImage() >= 0) &&
             (sourceCel->getImage() < sourceLayer->getSprite()->getStock()->size()));

      const Image* sourceImage = sourceLayer->getSprite()->getStock()->getImage(sourceCel->getImage());
      ASSERT(sourceImage != NULL);

      Image* newImage = Image::createCopy(sourceImage);
      newCel->setImage(destLayer->getSprite()->getStock()->addImage(newImage));

      if (undo->isEnabled()) {
        undo->pushUndoer(new undoers::AddImage(undo->getObjects(),
            destLayer->getSprite()->getStock(),
            newCel->getImage()));
      }

      destLayer->addCel(newCel);
      newCel.release();
    }
  }
  else if (sourceLayer0->isFolder() && destLayer0->isFolder()) {
    const LayerFolder* sourceLayer = static_cast<const LayerFolder*>(sourceLayer0);
    LayerFolder* destLayer = static_cast<LayerFolder*>(destLayer0);

    LayerConstIterator it = sourceLayer->getLayerBegin();
    LayerConstIterator end = sourceLayer->getLayerEnd();

    for (; it != end; ++it) {
      Layer* sourceChild = *it;
      base::UniquePtr<Layer> destChild(NULL);

      if (sourceChild->isImage()) {
        destChild.reset(new LayerImage(destLayer->getSprite()));
        copyLayerContent(sourceChild, destDoc, destChild);
      }
      else if (sourceChild->isFolder()) {
        destChild.reset(new LayerFolder(destLayer->getSprite()));
        copyLayerContent(sourceChild, destDoc, destChild);
      }
      else {
        ASSERT(false);
      }

      ASSERT(destChild != NULL);

      // Add the new layer in the sprite.
      destDoc->getApi().addLayer(destLayer,
                                 destChild.release(),
                                 destLayer->getLastLayer());
    }
  }
  else  {
    ASSERT(false && "Trying to copy two incompatible layers");
  }
}

Document* Document::duplicate(DuplicateType type) const
{
  const Sprite* sourceSprite = getSprite();
  base::UniquePtr<Sprite> spriteCopyPtr(new Sprite(sourceSprite->getPixelFormat(),
                                             sourceSprite->getWidth(),
                                             sourceSprite->getHeight(),
                                             sourceSprite->getPalette(FrameNumber(0))->size()));
  base::UniquePtr<Document> documentCopy(new Document(spriteCopyPtr));
  Sprite* spriteCopy = spriteCopyPtr.release();

  spriteCopy->setTotalFrames(sourceSprite->getTotalFrames());

  // Copy frames duration
  for (FrameNumber i(0); i < sourceSprite->getTotalFrames(); ++i)
    spriteCopy->setFrameDuration(i, sourceSprite->getFrameDuration(i));

  // Copy color palettes
  {
    PalettesList::const_iterator it = sourceSprite->getPalettes().begin();
    PalettesList::const_iterator end = sourceSprite->getPalettes().end();
    for (; it != end; ++it) {
      const Palette* pal = *it;
      spriteCopy->setPalette(pal, true);
    }
  }

  switch (type) {

    case DuplicateExactCopy:
      // Disable the undo
      documentCopy->getUndo()->setEnabled(false);

      // Copy the layer folder
      copyLayerContent(getSprite()->getFolder(), documentCopy, spriteCopy->getFolder());

      // Re-enable the undo
      documentCopy->getUndo()->setEnabled(true);
      break;

    case DuplicateWithFlattenLayers:
      {
        // Flatten layers
        ASSERT(sourceSprite->getFolder() != NULL);

        LayerImage* flatLayer = create_flatten_layer_copy
            (spriteCopy,
             sourceSprite->getFolder(),
             gfx::Rect(0, 0, sourceSprite->getWidth(), sourceSprite->getHeight()),
             FrameNumber(0), sourceSprite->getLastFrame());

        // Add and select the new flat layer
        spriteCopy->getFolder()->addLayer(flatLayer);

        // Configure the layer as background only if the original
        // sprite has a background layer.
        if (sourceSprite->getBackgroundLayer() != NULL)
          flatLayer->configureAsBackground();
      }
      break;
  }

  documentCopy->setMask(getMask());
  documentCopy->m_maskVisible = m_maskVisible;
  documentCopy->generateMaskBoundaries();

  return documentCopy.release();
}

//////////////////////////////////////////////////////////////////////
// Multi-threading ("sprite wrappers" use this)

bool Document::lock(LockType lockType)
{
  scoped_lock lock(*m_mutex);

  switch (lockType) {

    case ReadLock:
      // If no body is writting the sprite...
      if (!m_write_lock) {
        // We can read it
        ++m_read_locks;
        return true;
      }
      break;

    case WriteLock:
      // If no body is reading and writting...
      if (m_read_locks == 0 && !m_write_lock) {
        // We can start writting the sprite...
        m_write_lock = true;
        return true;
      }
      break;

  }

  return false;
}

bool Document::lockToWrite()
{
  scoped_lock lock(*m_mutex);

  // this only is possible if there are just one reader
  if (m_read_locks == 1) {
    ASSERT(!m_write_lock);
    m_read_locks = 0;
    m_write_lock = true;
    return true;
  }
  else
    return false;
}

void Document::unlockToRead()
{
  scoped_lock lock(*m_mutex);

  ASSERT(m_read_locks == 0);
  ASSERT(m_write_lock);

  m_write_lock = false;
  m_read_locks = 1;
}

void Document::unlock()
{
  scoped_lock lock(*m_mutex);

  if (m_write_lock) {
    m_write_lock = false;
  }
  else if (m_read_locks > 0) {
    --m_read_locks;
  }
  else {
    ASSERT(false);
  }
}

} // namespace app
