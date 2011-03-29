/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "document.h"

#include "base/memory.h"
#include "base/mutex.h"
#include "base/scoped_lock.h"
#include "base/unique_ptr.h"
#include "file/format_options.h"
#include "objects_container_impl.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "undo/undo_history.h"
#include "util/boundary.h"

using namespace undo;

Document::Document(Sprite* sprite)
  : m_id(WithoutDocumentId)
  , m_sprite(sprite)
  , m_objects(new ObjectsContainerImpl)
  , m_undoHistory(new UndoHistory(m_objects))
  , m_filename("Sprite")
  , m_associated_to_file(false)
  , m_mutex(new Mutex)
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
  // Boundary stuff
  m_bound.nseg = 0;
  m_bound.seg = NULL;

  // Preferred edition options
  m_preferred.scroll_x = 0;
  m_preferred.scroll_y = 0;
  m_preferred.zoom = 0;
  m_preferred.virgin = true;
}

Document::~Document()
{
  if (m_bound.seg)
    base_free(m_bound.seg);

  destroyExtraCel();
}

Document* Document::createBasicDocument(int imgtype, int width, int height, int ncolors)
{
  // Create the sprite.
  UniquePtr<Sprite> sprite(new Sprite(imgtype, width, height, ncolors));
  sprite->setTotalFrames(1);

  // Create the main image.
  int indexInStock;
  {
    UniquePtr<Image> image(image_new(imgtype, width, height));

    // Clear the image with mask color.
    image_clear(image, 0);

    // Add image in the sprite's stock.
    indexInStock = sprite->getStock()->addImage(image);
    image.release();		// Release the image because it is in the sprite's stock.
  }

  // Create the first transparent layer.
  {
    UniquePtr<LayerImage> layer(new LayerImage(sprite));
    layer->setName("Layer 1");

    // Create the cel.
    {
      UniquePtr<Cel> cel(new Cel(0, indexInStock));
      cel->setPosition(0, 0);

      // Add the cel in the layer.
      layer->addCel(cel);
      cel.release();		// Release the cel because it is in the layer
    }

    // Add the layer in the sprite.
    sprite->getFolder()->add_layer(layer);

    // Set the layer as the current one.
    sprite->setCurrentLayer(layer.release()); // Release the layer because it's owned by the sprite
  }

  // Create the document with the new sprite.
  UniquePtr<Document> document(new Document(sprite));
  sprite.release();		// Release the sprite because it is in the document.

  document->setFilename("Sprite");
  return document.release();	// Release the document (it does not throw) returning the raw pointer
}

const char* Document::getFilename() const
{
  return m_filename.c_str();
}

void Document::setFilename(const char* filename)
{
  m_filename = filename;
}

bool Document::isModified() const
{
  return !m_undoHistory->isSavedState();
}

bool Document::isAssociatedToFile() const
{
  return m_associated_to_file;
}

void Document::markAsSaved()
{
  m_undoHistory->markSavedState();
  m_associated_to_file = true;
}

//////////////////////////////////////////////////////////////////////
// Loaded options from file

void Document::setFormatOptions(const SharedPtr<FormatOptions>& format_options)
{
  m_format_options = format_options;
}

//////////////////////////////////////////////////////////////////////
// Preferred editor settings

PreferredEditorSettings Document::getPreferredEditorSettings() const
{
  return m_preferred;
}

void Document::setPreferredEditorSettings(const PreferredEditorSettings& settings)
{
  m_preferred = settings;
}

//////////////////////////////////////////////////////////////////////
// Boundaries

int Document::getBoundariesSegmentsCount() const
{
  return m_bound.nseg;
}

const _BoundSeg* Document::getBoundariesSegments() const
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
    if (!isMaskVisible())	// The mask is hidden
      return;			// Done, without boundaries
    else
      mask = getMask();		// Use the document mask
  }

  ASSERT(mask != NULL);

  if (mask->bitmap) {
    m_bound.seg = find_mask_boundary(mask->bitmap,
				     &m_bound.nseg,
				     IgnoreBounds, 0, 0, 0, 0);
    for (int c=0; c<m_bound.nseg; c++) {
      m_bound.seg[c].x1 += mask->x;
      m_bound.seg[c].y1 += mask->y;
      m_bound.seg[c].x2 += mask->x;
      m_bound.seg[c].y2 += mask->y;
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
    m_extraCel = new Cel(0, 0);	// Ignored fields for this cell (frame, and image index)

  m_extraCel->setPosition(x, y);
  m_extraCel->setOpacity(opacity);

  if (!m_extraImage ||
      m_extraImage->imgtype != getSprite()->getImgType() ||
      m_extraImage->w != w ||
      m_extraImage->h != h) {
    delete m_extraImage;		// image
    m_extraImage = image_new(getSprite()->getImgType(), w, h);
    image_clear(m_extraImage,
		m_extraImage->mask_color = 0);
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
  m_mask.reset(mask_new_copy(mask));
  m_maskVisible = true;
}

bool Document::isMaskVisible() const
{
  return
    m_maskVisible &&		// The mask was not hidden by the user explicitly
    m_mask &&			// The mask does exist
    !m_mask->is_empty();	// The mask is not empty
}

void Document::setMaskVisible(bool visible)
{
  m_maskVisible = visible;
}

//////////////////////////////////////////////////////////////////////
// Copying

void Document::copyLayerContent(const LayerImage* sourceLayer, LayerImage* destLayer) const
{
  // copy cels
  CelConstIterator it = sourceLayer->getCelBegin();
  CelConstIterator end = sourceLayer->getCelEnd();

  for (; it != end; ++it) {
    const Cel* sourceCel = *it;
    Cel* newCel = new Cel(*sourceCel);

    ASSERT((sourceCel->getImage() >= 0) &&
	   (sourceCel->getImage() < sourceLayer->getSprite()->getStock()->size()));

    const Image* sourceImage = sourceLayer->getSprite()->getStock()->getImage(sourceCel->getImage());
    ASSERT(sourceImage != NULL);

    Image* newImage = image_new_copy(sourceImage);

    newCel->setImage(destLayer->getSprite()->getStock()->addImage(newImage));

    if (m_undoHistory->isEnabled())
      m_undoHistory->undo_add_image(destLayer->getSprite()->getStock(), newCel->getImage());

    destLayer->addCel(newCel);
  }
}

Document* Document::duplicate(DuplicateType type) const
{
  const Sprite* sourceSprite = getSprite();
  UniquePtr<Sprite> spriteCopy(new Sprite(sourceSprite->getImgType(),
					  sourceSprite->getWidth(),
					  sourceSprite->getHeight(), sourceSprite->getPalette(0)->size()));

  spriteCopy->setTotalFrames(sourceSprite->getTotalFrames());
  spriteCopy->setCurrentFrame(sourceSprite->getCurrentFrame());

  // Copy frames duration
  for (int i = 0; i < sourceSprite->getTotalFrames(); ++i)
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
      {
	// TODO IMPLEMENT THIS
      }
      break;

    case DuplicateWithFlattenLayers:
      {
	// Flatten layers
	ASSERT(sourceSprite->getFolder() != NULL);

	LayerImage* flatLayer = layer_new_flatten_copy
	    (spriteCopy,
	     sourceSprite->getFolder(),
	     0, 0, sourceSprite->getWidth(), sourceSprite->getHeight(),
	     0, sourceSprite->getTotalFrames()-1);

	// Add and select the new flat layer
	spriteCopy->getFolder()->add_layer(flatLayer);
	spriteCopy->setCurrentLayer(flatLayer);

	// Configure the layer as background only if the original
	// sprite has a background layer.
	if (sourceSprite->getBackgroundLayer() != NULL)
	  flatLayer->configureAsBackground();
      }
      break;
  }

  UniquePtr<Document> documentCopy(new Document(spriteCopy));
  spriteCopy.release();

  documentCopy->setMask(getMask());
  documentCopy->m_maskVisible = m_maskVisible;
  documentCopy->m_preferred = m_preferred;
  documentCopy->generateMaskBoundaries();

  return documentCopy.release();
}

//////////////////////////////////////////////////////////////////////
// Multi-threading ("sprite wrappers" use this)

bool Document::lock(LockType lockType)
{
  ScopedLock lock(*m_mutex);

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
  ScopedLock lock(*m_mutex);

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
  ScopedLock lock(*m_mutex);

  ASSERT(m_read_locks == 0);
  ASSERT(m_write_lock);

  m_write_lock = false;
  m_read_locks = 1;
}

void Document::unlock()
{
  ScopedLock lock(*m_mutex);

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
