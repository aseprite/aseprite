/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include <cstring>
#include <vector>

#include "jinete/jlist.h"
#include "Vaca/Mutex.h"
#include "Vaca/ScopedLock.h"

#include "file/format_options.h"
#include "modules/palettes.h"
#include "raster/raster.h"
#include "util/boundary.h"

#ifdef _MSC_VER
  #pragma warning(disable: 4355)
#endif

using Vaca::Mutex;
using Vaca::ScopedLock;

static Layer* index2layer(const Layer* layer, int index, int* index_count);
static int layer2index(const Layer* layer, const Layer* find_layer, int* index_count);

//////////////////////////////////////////////////////////////////////
// SpriteImpl

class SpriteImpl
{
public:
  SpriteImpl(Sprite* sprite, int imgtype, int width, int height, int ncolors);
  ~SpriteImpl();

  static SpriteImpl* copyBase(Sprite* new_sprite, const SpriteImpl* src_sprite);
  static SpriteImpl* copyLayers(SpriteImpl* dst_sprite, const SpriteImpl* src_sprite);

  bool lock(bool write);
  bool lockToWrite();
  void unlockToRead();
  void unlock();

  int getImgType() const { 
    return m_imgtype;
  }

  void setImgType(int imgtype) {
    m_imgtype = imgtype;
  }

  int getWidth() const {
    return m_width;
  }

  int getHeight() const {
    return m_height;
  }

  void setSize(int width, int height) {
    ASSERT(width > 0);
    ASSERT(height > 0);

    m_width = width;
    m_height = height;
  }

  const char* getFilename() const {
    return m_filename.c_str();
  }

  void setFilename(const char* filename) {
    m_filename = filename;
  }

  bool isModified() const {
    return (m_undo->diff_count ==
	    m_undo->diff_saved) ? false: true;
  }

  bool isAssociatedToFile() const {
    return m_associated_to_file;
  }

  void markAsSaved() {
    m_undo->diff_saved = m_undo->diff_count;
    m_associated_to_file = true;
  }

  bool needAlpha() const {
    switch (m_imgtype) {
      case IMAGE_RGB:
      case IMAGE_GRAYSCALE:
	return (getBackgroundLayer() == NULL);
    }
    return false;
  }

  int getMemSize() const;

  const LayerFolder* getFolder() const {
    return m_folder;
  }

  LayerFolder* getFolder() {
    return m_folder;
  }

  const LayerImage* getBackgroundLayer() const;
  LayerImage* getBackgroundLayer();

  const Layer* getCurrentLayer() const {
    return m_layer;
  }

  Layer* getCurrentLayer() {
    return m_layer;
  }

  void setCurrentLayer(Layer* layer) {
    m_layer = layer;
  }

  int countLayers() const {
    return getFolder()->get_layers_count();
  }

  const Layer* indexToLayer(int index) const {
    int index_count = -1;
    return index2layer(getFolder(), index, &index_count);
  }

  Layer* indexToLayer(int index) {
    int index_count = -1;
    return index2layer(getFolder(), index, &index_count);
  }

  int layerToIndex(const Layer* layer) const {
    int index_count = -1;
    return layer2index(getFolder(), layer, &index_count);
  }

  const Palette* getPalette(int frame) const;

  Palette* getPalette(int frame);

  JList getPalettes() {
    return m_palettes;
  }

  void setPalette(Palette* pal, bool truncate);
  void resetPalettes();
  void deletePalette(Palette* pal);

  const Palette* getCurrentPalette() const {
    return getPalette(getCurrentFrame());
  }

  Palette* getCurrentPalette() {
    return getPalette(getCurrentFrame());
  }

  RgbMap* getRgbMap(int frame) {
    if (m_rgbMap == NULL) {
      m_rgbMap = new RgbMap();
      m_rgbMap->regenerate(getPalette(frame));
    }
    else if (!m_rgbMap->match(getPalette(frame))) {
      m_rgbMap->regenerate(getPalette(frame));
    }
    return m_rgbMap;
  }

  int getTotalFrames() const {
    return m_frames;
  }

  void setTotalFrames(int frames);

  int getFrameDuration(int frame) const {
    if (frame >= 0 && frame < m_frames)
      return m_frlens[frame];
    else
      return 0;
  }

  void setFrameDuration(int frame, int msecs) {
    if (frame >= 0 && frame < m_frames)
      m_frlens[frame] = MID(1, msecs, 65535);
  }

  void setDurationForAllFrames(int msecs) {
    std::fill(m_frlens.begin(), m_frlens.end(), MID(1, msecs, 65535));
  }

  int getCurrentFrame() const {
    return m_frame;
  }

  void setCurrentFrame(int frame) {
    m_frame = frame;
  }

  const Stock* getStock() const {
    return m_stock;
  }

  Stock* getStock() {
    return m_stock;
  }

  const Image* getCurrentImage(int* x, int* y, int* opacity) const {
    const Image* image = NULL;

    if (getCurrentLayer() != NULL &&
	getCurrentLayer()->is_image()) {
      const Cel* cel = static_cast<const LayerImage*>(getCurrentLayer())->get_cel(getCurrentFrame());
      if (cel) {
	ASSERT((cel->image >= 0) &&
	       (cel->image < getStock()->nimage));

	image = getStock()->image[cel->image];

	if (x) *x = cel->x;
	if (y) *y = cel->y;
	if (opacity) *opacity = MID(0, cel->opacity, 255);
      }
    }

    return image;
  }

  Image* getCurrentImage(int* x, int* y, int* opacity) {
    Image* image = NULL;

    if (getCurrentLayer() != NULL &&
	getCurrentLayer()->is_image()) {
      Cel* cel = static_cast<LayerImage*>(getCurrentLayer())->get_cel(getCurrentFrame());
      if (cel) {
	ASSERT((cel->image >= 0) &&
	       (cel->image < getStock()->nimage));

	image = getStock()->image[cel->image];

	if (x) *x = cel->x;
	if (y) *y = cel->y;
	if (opacity) *opacity = MID(0, cel->opacity, 255);
      }
    }

    return image;
  }

  void getCels(CelList& cels) {
    getFolder()->get_cels(cels);
  }

  void remapImages(int frame_from, int frame_to, const std::vector<int>& mapping) {
    ASSERT(m_imgtype == IMAGE_INDEXED);
    ASSERT(mapping.size() == 256);

    CelList cels;
    getCels(cels);

    for (CelIterator it = cels.begin(); it != cels.end(); ++it) {
      Cel* cel = *it;

      // Remap this Cel because is inside the specified range
      if (cel->frame >= frame_from && 
	  cel->frame <= frame_to) {
	Image* image = stock_get_image(getStock(), cel->image);

	for (int y=0; y<image->h; ++y) {
	  IndexedTraits::address_t ptr = image_address_fast<IndexedTraits>(image, 0, y);
	  for (int x=0; x<image->w; ++x, ++ptr)
	    *ptr = mapping[*ptr];
	}
      }
    }
  }

  const Undo* getUndo() const {
    return m_undo;
  }

  Undo* getUndo() {
    return m_undo;
  }

  const Mask* getMask() const {
    return m_mask;
  }

  Mask* getMask() {
    return m_mask;
  }

  void setMask(const Mask* mask) {
    if (m_mask)
      mask_free(m_mask);

    m_mask = mask_new_copy(mask);
  }

  void addMask(Mask* mask) {
    if (!mask->name) // You can't add masks to repository without name
      return;
    
    if (requestMask(mask->name)) // You can't add a mask that already exists
      return;

    // And add the new mask
    jlist_append(m_repository.masks, mask);
  }

  void removeMask(Mask* mask) {
    // Remove the mask from the repository
    jlist_remove(m_repository.masks, mask);
  }

  Mask* requestMask(const char* name) const;

  void generateMaskBoundaries(Mask* mask = NULL);

  JList getMasksRepository() {
    return m_repository.masks;
  }

  void addPath(Path* path) {
    jlist_append(m_repository.paths, path);
  }

  void removePath(Path* path) {
    jlist_remove(m_repository.paths, path);
  }

  void setPath(const Path* path) {
    if (m_path)
      path_free(m_path);

    m_path = path_new_copy(path);
  }

  JList getPathsRepository() {
    return m_repository.paths;
  }

  void setFormatOptions(FormatOptions* format_options) {
    if (m_format_options)
      jfree(m_format_options);

    m_format_options = format_options;
  }

  void render(Image* image, int x, int y) const {
    image_rectfill(image, x, y, x+m_width-1, y+m_height-1, 0);
    layer_render(getFolder(), image, x, y, getCurrentFrame());
  }

  int getPixel(int x, int y) const;

  PreferredEditorSettings getPreferredEditorSettings() const {
    return m_preferred;
  }

  void setPreferredEditorSettings(const PreferredEditorSettings& settings) {
    m_preferred = settings;
  }

  int getBoundariesSegmentsCount() const {
    return m_bound.nseg;
  }

  const _BoundSeg* getBoundariesSegments() const {
    return m_bound.seg;
  }

  void prepareExtraCel(int x, int y, int w, int h, int opacity);
  void destroyExtraCel();
  Cel* getExtraCel() const;
  Image* getExtraCelImage() const;

private:
  Sprite* m_self;			 // pointer to the Sprite
  int m_imgtype;			 // image type
  int m_width;				 // image width (in pixels)
  int m_height;				 // image height (in pixels)
  std::string m_filename;		 // sprite's file name
  bool m_associated_to_file;		 // true if this sprite is associated to a file in the file-system
  int m_frames;				 // how many frames has this sprite
  std::vector<int> m_frlens;		 // duration per frame
  int m_frame;				 // current frame, range [0,frames)
  JList m_palettes;			 // list of palettes
  Stock* m_stock;			 // stock to get images
  LayerFolder* m_folder;		 // main folder of layers
  Layer* m_layer;			 // current layer
  Path* m_path;				 // working path
  Mask* m_mask;				 // selected mask region
  Undo* m_undo;				 // undo stack
  struct {
    JList paths;				// paths
    JList masks;				// masks
  } m_repository;

  // Selected mask region boundaries
  struct {
    int nseg;
    _BoundSeg* seg;
  } m_bound;

  PreferredEditorSettings m_preferred;

  Cel* m_extraCel;        // Extra cel used to draw extra stuff (e.g. editor's pen preview, pixels in movement, etc.)
  Image* m_extraImage;	  // Image of the extra cel 

  // Mutex to modify the 'locked' flag.
  Vaca::Mutex* m_mutex;

  // True if some thread is writing the sprite.
  bool m_write_lock;

  // Greater than zero when one or more threads are reading the sprite.
  int m_read_locks;

  // Data to save the file in the same format that it was loaded
  FormatOptions* m_format_options;

  // Current rgb map
  RgbMap* m_rgbMap;
};

SpriteImpl::SpriteImpl(Sprite* sprite, int imgtype, int width, int height, int ncolors)
  : m_self(sprite)
  , m_imgtype(imgtype)
  , m_width(width)
  , m_height(height)
  , m_filename("Sprite")
{
  ASSERT(width > 0 && height > 0);

  m_associated_to_file = false;
  m_frames = 1;
  m_frlens.push_back(100);	// First frame with 100 msecs of duration
  m_frame = 0;
  m_palettes = jlist_new();
  m_stock = stock_new(imgtype);
  m_folder = new LayerFolder(m_self);
  m_layer = NULL;
  m_path = NULL;
  m_mask = mask_new();
  m_undo = undo_new(m_self);
  m_repository.paths = jlist_new();
  m_repository.masks = jlist_new();
  m_extraCel = NULL;
  m_extraImage = NULL;

  // Boundary stuff
  m_bound.nseg = 0;
  m_bound.seg = NULL;

  // Preferred edition options
  m_preferred.scroll_x = 0;
  m_preferred.scroll_y = 0;
  m_preferred.zoom = 0;

  // Generate palette
  Palette pal(0, ncolors);

  switch (imgtype) {

    // For colored images
    case IMAGE_RGB:
    case IMAGE_INDEXED:
      get_default_palette()->copyColorsTo(&pal);
      pal.resize(ncolors);
      break;

    // For black and white images
    case IMAGE_GRAYSCALE:
    case IMAGE_BITMAP:
      for (int c=0; c<ncolors; c++) {
	int g = 255 * c / (ncolors-1);
	g = MID(0, g, 255);
	pal.setEntry(c, _rgba(g, g, g, 255));
      }
      break;
  }

  // Multiple access
  m_write_lock = false;
  m_read_locks = 0;
  m_mutex = new Mutex();

  // File format options
  m_format_options = NULL;

  // Initial RGB map
  m_rgbMap = NULL;

  setPalette(&pal, true);
}

/**
 * Makes a copy "sprite" without the layers (only with the empty layer set)
 */
SpriteImpl* SpriteImpl::copyBase(Sprite* new_sprite, const SpriteImpl* src_sprite)
{
  JLink link;
  SpriteImpl* dst_sprite = new SpriteImpl(new_sprite,
					  src_sprite->m_imgtype,
					  src_sprite->m_width, src_sprite->m_height,
					  src_sprite->getPalette(0)->size());


  // Copy stock
  stock_free(dst_sprite->m_stock);
  dst_sprite->m_stock = stock_new_copy(src_sprite->m_stock);
  if (!dst_sprite->m_stock) {
    delete dst_sprite;
    return NULL;
  }

  // Copy general properties
  dst_sprite->m_filename = src_sprite->m_filename;
  dst_sprite->setTotalFrames(src_sprite->m_frames);
  std::copy(src_sprite->m_frlens.begin(),
	    src_sprite->m_frlens.end(),
	    dst_sprite->m_frlens.begin());

  // Copy color palettes
  JI_LIST_FOR_EACH(src_sprite->m_palettes, link) {
    Palette* pal = reinterpret_cast<Palette*>(link->data);
    dst_sprite->setPalette(pal, true);
  }

  // Copy path
  if (dst_sprite->m_path) {
    path_free(dst_sprite->m_path);
    dst_sprite->m_path = NULL;
  }

  if (src_sprite->m_path)
    dst_sprite->m_path = path_new_copy(src_sprite->m_path);

  // Copy mask
  if (dst_sprite->m_mask) {
    mask_free(dst_sprite->m_mask);
    dst_sprite->m_mask = NULL;
  }

  if (src_sprite->m_mask)
    dst_sprite->m_mask = mask_new_copy(src_sprite->m_mask);

  /* copy repositories */
  JI_LIST_FOR_EACH(src_sprite->m_repository.paths, link) {
    Path* path_copy = path_new_copy(reinterpret_cast<Path*>(link->data));
    if (path_copy)
      dst_sprite->addPath(path_copy);
  }

  JI_LIST_FOR_EACH(src_sprite->m_repository.masks, link) {
    Mask* mask_copy = mask_new_copy(reinterpret_cast<Mask*>(link->data));
    if (mask_copy)
      dst_sprite->addMask(mask_copy);
  }

  // Copy preferred edition options
  dst_sprite->m_preferred = src_sprite->m_preferred;

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

  undo_disable(dst_sprite->getUndo());
  dst_sprite->m_folder = src_sprite->getFolder()->duplicate_for(dst_sprite->m_self);
  undo_enable(dst_sprite->getUndo());

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

SpriteImpl::~SpriteImpl()
{
  JLink link;

  // Destroy layers
  delete m_folder;

  // Destroy images' stock
  if (m_stock)
    stock_free(m_stock);

  // Destroy paths
  if (m_repository.paths) {
    JI_LIST_FOR_EACH(m_repository.paths, link)
      path_free(reinterpret_cast<Path*>(link->data));

    jlist_free(m_repository.paths);
  }

  // Destroy masks
  if (m_repository.masks) {
    JI_LIST_FOR_EACH(m_repository.masks, link)
      mask_free(reinterpret_cast<Mask*>(link->data));

    jlist_free(m_repository.masks);
  }

  // Destroy palettes
  if (m_palettes) {
    JI_LIST_FOR_EACH(m_palettes, link)
      delete reinterpret_cast<Palette*>(link->data);

    jlist_free(m_palettes);
  }

  // Destroy undo, mask, etc.
  delete m_undo;
  delete m_mask;
  delete m_extraCel;
  delete m_extraImage;
  if (m_bound.seg) jfree(m_bound.seg);
  delete m_mutex;

  // Destroy file format options
  if (m_format_options)
    format_options_free(m_format_options);

  delete m_rgbMap;
}

/**
 * Lock the sprite to read or write it.
 *
 * @return true if the sprite can be accessed in the desired mode.
 */
bool SpriteImpl::lock(bool write)
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

/**
 * If you have locked the sprite to read, using this method
 * you can raise your access level to write it.
 */
bool SpriteImpl::lockToWrite()
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

/**
 * If you have locked the sprite to write, using this method
 * you can your access level to only read it.
 */
void SpriteImpl::unlockToRead()
{
  ScopedLock lock(*m_mutex);

  ASSERT(m_read_locks == 0);
  ASSERT(m_write_lock);

  m_write_lock = false;
  m_read_locks = 1;
}

void SpriteImpl::unlock()
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

int SpriteImpl::getMemSize() const
{
  Image *image;
  int i, size = 0;

  for (i=0; i<m_stock->nimage; i++) {
    image = m_stock->image[i];

    if (image != NULL)
      size += image_line_size(image, image->w) * image->h;
  }

  return size;
}

const LayerImage* SpriteImpl::getBackgroundLayer() const
{
  if (getFolder()->get_layers_count() > 0) {
    const Layer* bglayer = *getFolder()->get_layer_begin();

    if (bglayer->is_background())
      return static_cast<const LayerImage*>(bglayer);
  }
  return NULL;
}

LayerImage* SpriteImpl::getBackgroundLayer()
{
  if (getFolder()->get_layers_count() > 0) {
    Layer* bglayer = *getFolder()->get_layer_begin();

    if (bglayer->is_background())
      return static_cast<LayerImage*>(bglayer);
  }
  return NULL;
}

void SpriteImpl::setTotalFrames(int frames)
{
  frames = MAX(1, frames);
  m_frlens.resize(frames);

  if (frames > m_frames) {
    int c;
    for (c=m_frames; c<frames; c++)
      m_frlens[c] = m_frlens[m_frames-1];
  }

  m_frames = frames;
}

Mask *SpriteImpl::requestMask(const char *name) const
{
  Mask *mask;
  JLink link;

  JI_LIST_FOR_EACH(m_repository.masks, link) {
    mask = reinterpret_cast<Mask*>(link->data);
    if (strcmp(mask->name, name) == 0)
      return mask;
  }

  return NULL;
}


void SpriteImpl::generateMaskBoundaries(Mask* mask)
{
  // No mask specified? Use the current one in the sprite
  if (!mask)
    mask = m_mask;

  if (m_bound.seg) {
    jfree(m_bound.seg);
    m_bound.seg = NULL;
    m_bound.nseg = 0;
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

int SpriteImpl::getPixel(int x, int y) const
{
  int color = 0;

  if ((x >= 0) && (y >= 0) && (x < m_width) && (y < m_height)) {
    Image* image = image_new(m_imgtype, 1, 1);
    image_clear(image, 0);
    this->render(image, -x, -y);
    color = image_getpixel(image, 0, 0);
    image_free(image);
  }

  return color;
}

const Palette* SpriteImpl::getPalette(int frame) const
{
  const Palette* found = NULL;
  const Palette* pal;
  JLink link;

  ASSERT(frame >= 0);

  JI_LIST_FOR_EACH(m_palettes, link) {
    pal = reinterpret_cast<const Palette*>(link->data);
    if (frame < pal->getFrame())
      break;

    found = pal;
    if (frame == pal->getFrame())
      break;
  }

  ASSERT(found != NULL);
  return found;
}

Palette* SpriteImpl::getPalette(int frame)
{
  Palette* found = NULL;
  Palette* pal;
  JLink link;

  ASSERT(frame >= 0);

  JI_LIST_FOR_EACH(m_palettes, link) {
    pal = reinterpret_cast<Palette*>(link->data);
    if (frame < pal->getFrame())
      break;

    found = pal;
    if (frame == pal->getFrame())
      break;
  }

  ASSERT(found != NULL);
  return found;
}

void SpriteImpl::setPalette(Palette* pal, bool truncate)
{
  ASSERT(pal != NULL);

  if (!truncate) {
    Palette* sprite_pal = getPalette(pal->getFrame());
    pal->copyColorsTo(sprite_pal);
  }
  else {
    JLink link = NULL;
    Palette* other;

    JI_LIST_FOR_EACH(m_palettes, link) {
      other = reinterpret_cast<Palette*>(link->data);

      if (pal->getFrame() == other->getFrame()) {
	pal->copyColorsTo(other);
	return;
      }
      else if (pal->getFrame() < other->getFrame())
	break;
    }

    jlist_insert_before(m_palettes, link, new Palette(*pal));
  }
}

void SpriteImpl::resetPalettes()
{
  JLink link, next;

  JI_LIST_FOR_EACH_SAFE(m_palettes, link, next) {
    if (jlist_first(m_palettes) != link) {
      delete reinterpret_cast<Palette*>(link->data);
      jlist_delete_link(m_palettes, link);
    }
  }
}

void SpriteImpl::deletePalette(Palette* pal)
{
  ASSERT(pal != NULL);

  JLink link = jlist_find(m_palettes, pal);
  ASSERT(link != NULL);

  delete pal;
  jlist_delete_link(m_palettes, link);
}

void SpriteImpl::destroyExtraCel()
{
  delete m_extraCel;
  delete m_extraImage;

  m_extraCel = NULL;
  m_extraImage = NULL;
}

void SpriteImpl::prepareExtraCel(int x, int y, int w, int h, int opacity)
{
  if (!m_extraCel)
    m_extraCel = new Cel(0, 0);	// Ignored fields for this cell (frame, and image index)
  m_extraCel->x = x;
  m_extraCel->y = y;
  m_extraCel->opacity = opacity; 

  if (!m_extraImage ||
      m_extraImage->imgtype != m_imgtype ||
      m_extraImage->w != w ||
      m_extraImage->h != h) {
    delete m_extraImage;		// image
    m_extraImage = image_new(m_imgtype, w, h);
    image_clear(m_extraImage,
		m_extraImage->mask_color = 0);
  }
}

Cel* SpriteImpl::getExtraCel() const
{
  return m_extraCel;
}

Image* SpriteImpl::getExtraCelImage() const
{
  return m_extraImage;
}

//////////////////////////////////////////////////////////////////////
// Constructors/Destructor

Sprite::Sprite()
  : GfxObj(GFXOBJ_SPRITE)
  , m_impl(NULL)
{
}

Sprite::Sprite(int imgtype, int width, int height, int ncolors)
  : GfxObj(GFXOBJ_SPRITE)
  , m_impl(new SpriteImpl(this, imgtype, width, height, ncolors))
{
}

Sprite::~Sprite()
{
  delete m_impl;
}

Sprite::Sprite(const Sprite& original)
  : GfxObj(GFXOBJ_SPRITE)
  , m_impl(SpriteImpl::copyBase(this, original.m_impl))
{
  SpriteImpl::copyLayers(m_impl, original.m_impl);
}

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

Sprite* Sprite::createWithLayer(int imgtype, int width, int height, int ncolors)
{
  Sprite* sprite = NULL;
  LayerImage *layer = NULL;
  Image *image = NULL;
  Cel *cel = NULL;

  try {
    sprite = new Sprite(imgtype, width, height, ncolors);
    image = image_new(imgtype, width, height);
    layer = new LayerImage(sprite);

    /* clear with mask color */
    image_clear(image, 0);

    /* configure the first transparent layer */
    layer->set_name("Layer 1");
    layer->set_blend_mode(BLEND_MODE_NORMAL);

    /* add image in the layer stock */
    int index = stock_add_image(sprite->getStock(), image);

    /* create the cel */
    cel = cel_new(0, index);
    cel_set_position(cel, 0, 0);

    /* add the cel in the layer */
    layer->add_cel(cel);

    /* add the layer in the sprite */
    sprite->getFolder()->add_layer(layer);

    sprite->setTotalFrames(1);
    sprite->setFilename("Sprite");
    sprite->setCurrentLayer(layer);
  }
  catch (...) {
    delete sprite;
    delete image;
    throw;
  }

  return sprite;
}

//////////////////////////////////////////////////////////////////////
// Multi-threading ("sprite wrappers" use this)

/**
 * Lock the sprite to read or write it.
 *
 * @return true if the sprite can be accessed in the desired mode.
 */
bool Sprite::lock(bool write)
{
  return m_impl->lock(write);
}

/**
 * If you have locked the sprite to read, using this method
 * you can raise your access level to write it.
 */
bool Sprite::lockToWrite()
{
  return m_impl->lockToWrite();
}

/**
 * If you have locked the sprite to write, using this method
 * you can your access level to only read it.
 */
void Sprite::unlockToRead()
{
  m_impl->unlockToRead();
}

void Sprite::unlock()
{
  m_impl->unlock();
}

//////////////////////////////////////////////////////////////////////
// Main properties

int Sprite::getImgType() const
{
  return m_impl->getImgType();
}

void Sprite::setImgType(int imgtype)
{
  m_impl->setImgType(imgtype);
}

int Sprite::getWidth() const
{
  return m_impl->getWidth();
}

int Sprite::getHeight() const
{
  return m_impl->getHeight();
}

void Sprite::setSize(int width, int height)
{
  m_impl->setSize(width, height);
}

const char* Sprite::getFilename() const
{
  return m_impl->getFilename();
}

/**
 * Changes the sprite's filename
 */
void Sprite::setFilename(const char* filename)
{
  m_impl->setFilename(filename);
}

bool Sprite::isModified() const
{
  return m_impl->isModified();
}

bool Sprite::isAssociatedToFile() const
{
  return m_impl->isAssociatedToFile();
}

void Sprite::markAsSaved()
{
  m_impl->markAsSaved();
}

/**
   Returns true if the rendered images will contain alpha values less than 255.

   @note Only RGB and Grayscale images without background needs alpha channel in the render.
 */
bool Sprite::needAlpha() const
{
  return m_impl->needAlpha();
}

int Sprite::getMemSize() const
{
  return m_impl->getMemSize();
}

//////////////////////////////////////////////////////////////////////
// Layers

const LayerFolder* Sprite::getFolder() const
{
  return m_impl->getFolder();
}

LayerFolder* Sprite::getFolder()
{
  return m_impl->getFolder();
}

const LayerImage* Sprite::getBackgroundLayer() const
{
  return m_impl->getBackgroundLayer();
}

LayerImage* Sprite::getBackgroundLayer()
{
  return m_impl->getBackgroundLayer();
}

const Layer* Sprite::getCurrentLayer() const
{
  return m_impl->getCurrentLayer();
}

Layer* Sprite::getCurrentLayer()
{
  return m_impl->getCurrentLayer();
}

/**
 * Changes the current layer
 */
void Sprite::setCurrentLayer(Layer* layer)
{
  m_impl->setCurrentLayer(layer);
}

int Sprite::countLayers() const
{
  return m_impl->countLayers();
}

const Layer* Sprite::indexToLayer(int index) const
{
  return m_impl->indexToLayer(index);
}

Layer* Sprite::indexToLayer(int index)
{
  return m_impl->indexToLayer(index);
}

int Sprite::layerToIndex(const Layer* layer) const
{
  return m_impl->layerToIndex(layer);
}

//////////////////////////////////////////////////////////////////////
// Palettes

const Palette* Sprite::getPalette(int frame) const
{
  return m_impl->getPalette(frame);
}

Palette* Sprite::getPalette(int frame)
{
  return m_impl->getPalette(frame);
}

JList Sprite::getPalettes()
{
  return m_impl->getPalettes();
}

void Sprite::setPalette(Palette* pal, bool truncate)
{
  m_impl->setPalette(pal, truncate);
}

/**
 * Removes all palettes from the sprites except the first one.
 */
void Sprite::resetPalettes()
{
  m_impl->resetPalettes();
}

void Sprite::deletePalette(Palette* pal)
{
  m_impl->deletePalette(pal);
}

const Palette* Sprite::getCurrentPalette() const
{
  return m_impl->getCurrentPalette();
}

Palette* Sprite::getCurrentPalette()
{
  return m_impl->getCurrentPalette();
}

RgbMap* Sprite::getRgbMap()
{
  return m_impl->getRgbMap(getCurrentFrame());
}

RgbMap* Sprite::getRgbMap(int frame)
{
  return m_impl->getRgbMap(frame);
}

//////////////////////////////////////////////////////////////////////
// Frames

int Sprite::getTotalFrames() const
{
  return m_impl->getTotalFrames();
}

/**
 * Changes the quantity of frames
 */
void Sprite::setTotalFrames(int frames)
{
  m_impl->setTotalFrames(frames);
}

int Sprite::getFrameDuration(int frame) const
{
  return m_impl->getFrameDuration(frame);
}

void Sprite::setFrameDuration(int frame, int msecs)
{
  m_impl->setFrameDuration(frame, msecs);
}

/**
 * Sets a constant frame-rate.
 */
void Sprite::setDurationForAllFrames(int msecs)
{
  m_impl->setDurationForAllFrames(msecs);
}

int Sprite::getCurrentFrame() const
{
  return m_impl->getCurrentFrame();
}

void Sprite::setCurrentFrame(int frame)
{
  m_impl->setCurrentFrame(frame);
}

//////////////////////////////////////////////////////////////////////
// Images

const Stock* Sprite::getStock() const
{
  return m_impl->getStock();
}

Stock* Sprite::getStock()
{
  return m_impl->getStock();
}

const Image* Sprite::getCurrentImage(int* x, int* y, int* opacity) const
{
  return m_impl->getCurrentImage(x, y, opacity);
}

Image* Sprite::getCurrentImage(int* x, int* y, int* opacity)
{
  return m_impl->getCurrentImage(x, y, opacity);
}

void Sprite::getCels(CelList& cels)
{
  m_impl->getCels(cels);
}

void Sprite::remapImages(int frame_from, int frame_to, const std::vector<int>& mapping)
{
  m_impl->remapImages(frame_from, frame_to, mapping);
}

//////////////////////////////////////////////////////////////////////
// Undo

const Undo* Sprite::getUndo() const
{
  return m_impl->getUndo();
}

Undo* Sprite::getUndo()
{
  return m_impl->getUndo();
}

//////////////////////////////////////////////////////////////////////
// Mask

const Mask* Sprite::getMask() const
{
  return m_impl->getMask();
}

Mask* Sprite::getMask()
{
  return m_impl->getMask();
}

/**
 * Changes the current mask (makes a copy of "mask")
 */
void Sprite::setMask(const Mask* mask)
{
  m_impl->setMask(mask);
}

/**
 * Adds a mask to the sprites's repository
 */
void Sprite::addMask(Mask* mask)
{
  m_impl->addMask(mask);
}

/**
 * Removes a mask from the sprites's repository
 */
void Sprite::removeMask(Mask* mask)
{
  m_impl->removeMask(mask);
}

/**
 * Returns a mask from the sprite's repository searching it by its name
 */
Mask* Sprite::requestMask(const char* name) const
{
  return m_impl->requestMask(name);
}

void Sprite::generateMaskBoundaries(Mask* mask)
{
  m_impl->generateMaskBoundaries(mask);
}

JList Sprite::getMasksRepository()
{
  return m_impl->getMasksRepository();
}

//////////////////////////////////////////////////////////////////////
// Path

/**
 * Adds a path to the sprites's repository
 */
void Sprite::addPath(Path* path)
{
  m_impl->addPath(path);
}

/**
 * Removes a path from the sprites's repository
 */
void Sprite::removePath(Path* path)
{
  m_impl->removePath(path);
}

/**
 * Changes the current path (makes a copy of "path")
 */
void Sprite::setPath(const Path* path)
{
  m_impl->setPath(path);
}

JList Sprite::getPathsRepository()
{
  return m_impl->getPathsRepository();
}

//////////////////////////////////////////////////////////////////////
// Loaded options from file

void Sprite::setFormatOptions(FormatOptions* format_options)
{
  m_impl->setFormatOptions(format_options);
}

//////////////////////////////////////////////////////////////////////
// Drawing

void Sprite::render(Image* image, int x, int y) const
{
  m_impl->render(image, x, y);
}

/**
 * Gets a pixel from the sprite in the specified position. If in the
 * specified coordinates there're background this routine will return
 * the 0 color (the mask-color).
 */
int Sprite::getPixel(int x, int y) const
{
  return m_impl->getPixel(x, y);
}

//////////////////////////////////////////////////////////////////////
// Preferred editor settings

PreferredEditorSettings Sprite::getPreferredEditorSettings() const
{
  return m_impl->getPreferredEditorSettings();
}

void Sprite::setPreferredEditorSettings(const PreferredEditorSettings& settings)
{
  m_impl->setPreferredEditorSettings(settings);
}

//////////////////////////////////////////////////////////////////////
// Boundaries

int Sprite::getBoundariesSegmentsCount() const
{
  return m_impl->getBoundariesSegmentsCount();
}

const _BoundSeg* Sprite::getBoundariesSegments() const
{
  return m_impl->getBoundariesSegments();
}

//////////////////////////////////////////////////////////////////////
// Extras

void Sprite::prepareExtraCel(int x, int y, int w, int h, int opacity)
{
  m_impl->prepareExtraCel(x, y, w, h, opacity);
}

void Sprite::destroyExtraCel()
{
  m_impl->destroyExtraCel();
}

Cel* Sprite::getExtraCel() const
{
  return m_impl->getExtraCel();
}

Image* Sprite::getExtraCelImage() const
{
  return m_impl->getExtraCelImage();
}

static Layer* index2layer(const Layer* layer, int index, int* index_count)
{
  if (index == *index_count)
    return (Layer*)layer;
  else {
    (*index_count)++;

    if (layer->is_folder()) {
      Layer *found;

      LayerConstIterator it = static_cast<const LayerFolder*>(layer)->get_layer_begin();
      LayerConstIterator end = static_cast<const LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it) {
	if ((found = index2layer(*it, index, index_count)))
	  return found;
      }
    }

    return NULL;
  }
}

static int layer2index(const Layer* layer, const Layer* find_layer, int* index_count)
{
  if (layer == find_layer)
    return *index_count;
  else {
    (*index_count)++;

    if (layer->is_folder()) {
      int found;

      LayerConstIterator it = static_cast<const LayerFolder*>(layer)->get_layer_begin();
      LayerConstIterator end = static_cast<const LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it) {
	if ((found = layer2index(*it, find_layer, index_count)) >= 0)
	  return found;
      }
    }

    return -1;
  }
}
