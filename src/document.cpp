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
      UniquePtr<Cel> cel(cel_new(0, indexInStock));
      cel_set_position(cel, 0, 0);

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

  m_extraCel->x = x;
  m_extraCel->y = y;
  m_extraCel->opacity = opacity; 

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
// Clonning

Document* Document::duplicate(DuplicateType type) const
{
  const Sprite* sourceSprite = getSprite();
  UniquePtr<Sprite> spriteCopy(new Sprite(sourceSprite->getImgType(),
					  sourceSprite->getWidth(),
					  sourceSprite->getHeight(), sourceSprite->getPalette(0)->size()));

  // TODO IMPLEMENT THIS
  
  UniquePtr<Document> documentCopy(new Document(spriteCopy));
  spriteCopy.release();
  return documentCopy;
}

#if 0
Sprite* Sprite::createFlattenCopy(const Sprite& src_sprite)
{
  Sprite* dst_sprite = new Sprite();
  SpriteImpl* dst_sprite_impl = SpriteImpl::copyBase(dst_sprite, src_sprite.m_impl);
  dst_sprite->m_impl = dst_sprite_impl;

  // Flatten layers
  ASSERT(src_sprite.getFolder() != NULL);

  Layer* flat_layer;
  try {
    flat_layer = layer_new_flatten_copy(dst_sprite,
					src_sprite.getFolder(),
					0, 0, src_sprite.getWidth(), src_sprite.getHeight(),
					0, src_sprite.getTotalFrames()-1);
  }
  catch (const std::bad_alloc&) {
    delete dst_sprite;
    throw;
  }

  // Add and select the new flat layer
  dst_sprite->getFolder()->add_layer(flat_layer);
  dst_sprite->setCurrentLayer(flat_layer);

  return dst_sprite;
}

/**
 * Makes a copy "sprite" without the layers (only with the empty layer set)
 */
SpriteImpl* SpriteImpl::copyBase(Sprite* new_sprite, const SpriteImpl* src_sprite)
{
  SpriteImpl* dst_sprite = new SpriteImpl(new_sprite,
					  src_sprite->m_imgtype,
					  src_sprite->m_width, src_sprite->m_height,
					  src_sprite->getPalette(0)->size());


  // Delete the original empty stock from the dst_sprite
  delete dst_sprite->m_stock;

  // Clone the src_sprite stock
  dst_sprite->m_stock = new Stock(*src_sprite->m_stock);

  // Copy general properties
  dst_sprite->m_filename = src_sprite->m_filename;
  dst_sprite->setTotalFrames(src_sprite->m_frames);
  std::copy(src_sprite->m_frlens.begin(),
	    src_sprite->m_frlens.end(),
	    dst_sprite->m_frlens.begin());

  // Copy color palettes
  {
    PalettesList::const_iterator end = src_sprite->m_palettes.end();
    PalettesList::const_iterator it = src_sprite->m_palettes.begin();
    for (; it != end; ++it) {
      Palette* pal = *it;
      dst_sprite->setPalette(pal, true);
    }
  }

  // Copy path
  if (dst_sprite->m_path) {
    delete dst_sprite->m_path;
    dst_sprite->m_path = NULL;
  }

  if (src_sprite->m_path)
    dst_sprite->m_path = new Path(*src_sprite->m_path);

  // Copy mask
  if (dst_sprite->m_mask) {
    delete dst_sprite->m_mask;
    dst_sprite->m_mask = NULL;
  }

  if (src_sprite->m_mask)
    dst_sprite->m_mask = mask_new_copy(src_sprite->m_mask);

  return dst_sprite;
}

SpriteImpl* SpriteImpl::copyLayers(SpriteImpl* dst_sprite, const SpriteImpl* src_sprite)
{
  // Copy layers
  if (dst_sprite->m_folder) {
    delete dst_sprite->m_folder; // delete
    dst_sprite->m_folder = NULL;
  }

  ASSERT(src_sprite->getFolder() != NULL);

  // Disable undo temporarily
  dst_sprite->getUndo()->setEnabled(false);
  dst_sprite->m_folder = src_sprite->getFolder()->duplicate_for(dst_sprite->m_self);
  dst_sprite->getUndo()->setEnabled(true);

  if (dst_sprite->m_folder == NULL) {
    delete dst_sprite;
    return NULL;
  }

  // Selected layer
  if (src_sprite->getCurrentLayer() != NULL) { 
    int selected_layer = src_sprite->layerToIndex(src_sprite->getCurrentLayer());
    dst_sprite->setCurrentLayer(dst_sprite->indexToLayer(selected_layer));
  }

  dst_sprite->generateMaskBoundaries();
  return dst_sprite;
}

#endif

//////////////////////////////////////////////////////////////////////
// Multi-threading ("sprite wrappers" use this)

bool Document::lock(bool write)
{
  ScopedLock lock(*m_mutex);

  // read-only
  if (!write) {
    // If no body is writting the sprite...
    if (!m_write_lock) {
      // We can read it
      ++m_read_locks;
      return true;
    }
  }
  // read and write
  else {
    // If no body is reading and writting...
    if (m_read_locks == 0 && !m_write_lock) {
      // We can start writting the sprite...
      m_write_lock = true;
      return true;
    }
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
