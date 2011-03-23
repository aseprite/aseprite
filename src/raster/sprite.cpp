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

#include <cstring>
#include <vector>

#include "base/memory.h"
#include "base/remove_from_container.h"
#include "file/format_options.h"
#include "raster/raster.h"

#ifdef _MSC_VER
  #pragma warning(disable: 4355)
#endif

static Layer* index2layer(const Layer* layer, int index, int* index_count);
static int layer2index(const Layer* layer, const Layer* find_layer, int* index_count);

//////////////////////////////////////////////////////////////////////
// SpriteImpl

class SpriteImpl
{
public:
  SpriteImpl(Sprite* sprite, int imgtype, int width, int height, int ncolors);
  ~SpriteImpl();

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

  bool needAlpha() const {
    switch (m_imgtype) {
      case IMAGE_RGB:
      case IMAGE_GRAYSCALE:
	return (getBackgroundLayer() == NULL);
    }
    return false;
  }

  ase_uint32 getTransparentColor() const {
    return m_transparentColor;
  }

  void setTransparentColor(ase_uint32 color) {
    m_transparentColor = color;
  }

  int getMemSize() const;

  LayerFolder* getFolder() const {
    return m_folder;
  }

  LayerImage* getBackgroundLayer() const;

  Layer* getCurrentLayer() const {
    return m_layer;
  }

  void setCurrentLayer(Layer* layer) {
    m_layer = layer;
  }

  int countLayers() const {
    return getFolder()->get_layers_count();
  }

  Layer* indexToLayer(int index) const {
    int index_count = -1;
    return index2layer(getFolder(), index, &index_count);
  }

  int layerToIndex(const Layer* layer) const {
    int index_count = -1;
    return layer2index(getFolder(), layer, &index_count);
  }

  Palette* getPalette(int frame) const;

  PalettesList getPalettes() const {
    return m_palettes;
  }

  void setPalette(Palette* pal, bool truncate);
  void resetPalettes();
  void deletePalette(Palette* pal);

  Palette* getCurrentPalette() const {
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

  Stock* getStock() const {
    return m_stock;
  }

  Image* getCurrentImage(int* x, int* y, int* opacity) const {
    Image* image = NULL;

    if (getCurrentLayer() != NULL &&
	getCurrentLayer()->is_image()) {
      const Cel* cel = static_cast<const LayerImage*>(getCurrentLayer())->getCel(getCurrentFrame());
      if (cel) {
	ASSERT((cel->image >= 0) &&
	       (cel->image < getStock()->size()));

	image = getStock()->getImage(cel->image);

	if (x) *x = cel->x;
	if (y) *y = cel->y;
	if (opacity) *opacity = MID(0, cel->opacity, 255);
      }
    }

    return image;
  }

  void getCels(CelList& cels) {
    getFolder()->getCels(cels);
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
	Image* image = getStock()->getImage(cel->image);

	for (int y=0; y<image->h; ++y) {
	  IndexedTraits::address_t ptr = image_address_fast<IndexedTraits>(image, 0, y);
	  for (int x=0; x<image->w; ++x, ++ptr)
	    *ptr = mapping[*ptr];
	}
      }
    }
  }

  Mask* getMask() const {
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
    m_repository.masks.push_back(mask);
  }

  void removeMask(Mask* mask) {
    // Remove the mask from the repository
    base::remove_from_container(m_repository.masks, mask);
  }

  Mask* requestMask(const char* name) const;

  MasksList getMasksRepository() {
    return m_repository.masks;
  }

  void addPath(Path* path) {
    m_repository.paths.push_back(path);
  }

  void removePath(Path* path) {
    base::remove_from_container(m_repository.paths, path);
  }

  void setPath(const Path* path) {
    delete m_path;
    m_path = new Path(*path);
  }

  PathsList getPathsRepository() {
    return m_repository.paths;
  }

  void render(Image* image, int x, int y) const {
    image_rectfill(image, x, y, x+m_width-1, y+m_height-1, 0);
    layer_render(getFolder(), image, x, y, getCurrentFrame());
  }

  int getPixel(int x, int y) const;

private:
  Sprite* m_self;			 // pointer to the Sprite
  int m_imgtype;			 // image type
  int m_width;				 // image width (in pixels)
  int m_height;				 // image height (in pixels)
  int m_frames;				 // how many frames has this sprite
  std::vector<int> m_frlens;		 // duration per frame
  int m_frame;				 // current frame, range [0,frames)
  PalettesList m_palettes;		 // list of palettes
  Stock* m_stock;			 // stock to get images
  LayerFolder* m_folder;		 // main folder of layers
  Layer* m_layer;			 // current layer
  Path* m_path;				 // working path
  Mask* m_mask;				 // selected mask region
  struct {
    PathsList paths;			// paths
    MasksList masks;			// masks
  } m_repository;

  // Current rgb map
  RgbMap* m_rgbMap;

  // Transparent color used in indexed images
  ase_uint32 m_transparentColor;
};

SpriteImpl::SpriteImpl(Sprite* sprite, int imgtype, int width, int height, int ncolors)
  : m_self(sprite)
  , m_imgtype(imgtype)
  , m_width(width)
  , m_height(height)
{
  ASSERT(width > 0 && height > 0);

  m_frames = 1;
  m_frlens.push_back(100);	// First frame with 100 msecs of duration
  m_frame = 0;
  m_stock = new Stock(imgtype);
  m_folder = new LayerFolder(m_self);
  m_layer = NULL;
  m_path = NULL;
  m_mask = mask_new();

  // Generate palette
  Palette pal(0, ncolors);

  switch (imgtype) {

    // For colored images
    case IMAGE_RGB:
    case IMAGE_INDEXED:
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

  // Initial RGB map
  m_rgbMap = NULL;

  // The transparent color for indexed images is 0 by default
  m_transparentColor = 0;

  setPalette(&pal, true);
}

SpriteImpl::~SpriteImpl()
{
  // Destroy layers
  delete m_folder;

  // Destroy images' stock
  if (m_stock)
    delete m_stock;

  // Destroy paths
  {
    PathsList::iterator end = m_repository.paths.end();
    PathsList::iterator it = m_repository.paths.begin();
    for (; it != end; ++it)
      delete *it;		// path
  }

  // Destroy masks
  {
    MasksList::iterator end = m_repository.masks.end();
    MasksList::iterator it = m_repository.masks.begin();
    for (; it != end; ++it)
      delete *it;		// mask
  }

  // Destroy palettes
  {
    PalettesList::iterator end = m_palettes.end();
    PalettesList::iterator it = m_palettes.begin();
    for (; it != end; ++it)
      delete *it;		// palette
  }

  // Destroy undo, mask, etc.
  delete m_mask;
  delete m_rgbMap;
}

int SpriteImpl::getMemSize() const
{
  Image *image;
  int i, size = 0;

  for (i=0; i<m_stock->size(); i++) {
    image = m_stock->getImage(i);
    if (image != NULL)
      size += image_line_size(image, image->w) * image->h;
  }

  return size;
}

LayerImage* SpriteImpl::getBackgroundLayer() const
{
  if (getFolder()->get_layers_count() > 0) {
    Layer* bglayer = *getFolder()->get_layer_begin();

    if (bglayer->is_background()) {
      ASSERT(bglayer->is_image());
      return static_cast<LayerImage*>(bglayer);
    }
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
  MasksList::const_iterator end = m_repository.masks.end();
  MasksList::const_iterator it = m_repository.masks.begin();

  for (; it != end; ++it) {
    Mask* mask = *it;
    if (strcmp(mask->name, name) == 0)
      return mask;
  }

  return NULL;
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

Palette* SpriteImpl::getPalette(int frame) const
{
  ASSERT(frame >= 0);

  Palette* found = NULL;

  PalettesList::const_iterator end = m_palettes.end();
  PalettesList::const_iterator it = m_palettes.begin();
  for (; it != end; ++it) {
    Palette* pal = *it;
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
    Palette* other;

    PalettesList::iterator end = m_palettes.end();
    PalettesList::iterator it = m_palettes.begin();
    for (; it != end; ++it) {
      other = *it;

      if (pal->getFrame() == other->getFrame()) {
	pal->copyColorsTo(other);
	return;
      }
      else if (pal->getFrame() < other->getFrame())
	break;
    }

    m_palettes.insert(it, new Palette(*pal));
  }
}

void SpriteImpl::resetPalettes()
{
  PalettesList::iterator end = m_palettes.end();
  PalettesList::iterator it = m_palettes.begin();

  if (it != end) {
    ++it;			// Leave the first palette only.
    while (it != end) {
      delete *it;		// palette
      it = m_palettes.erase(it);
      end = m_palettes.end();
    }
  }
}

void SpriteImpl::deletePalette(Palette* pal)
{
  ASSERT(pal != NULL);

  base::remove_from_container(m_palettes, pal);
  delete pal;			// palette
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

/**
   Returns true if the rendered images will contain alpha values less than 255.

   @note Only RGB and Grayscale images without background needs alpha channel in the render.
 */
bool Sprite::needAlpha() const
{
  return m_impl->needAlpha();
}

ase_uint32 Sprite::getTransparentColor() const
{
  return m_impl->getTransparentColor();
}

void Sprite::setTransparentColor(ase_uint32 color)
{
  m_impl->setTransparentColor(color);
}

int Sprite::getMemSize() const
{
  return m_impl->getMemSize();
}

//////////////////////////////////////////////////////////////////////
// Layers

LayerFolder* Sprite::getFolder() const
{
  return m_impl->getFolder();
}

LayerImage* Sprite::getBackgroundLayer() const
{
  return m_impl->getBackgroundLayer();
}

Layer* Sprite::getCurrentLayer() const
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

Layer* Sprite::indexToLayer(int index) const
{
  return m_impl->indexToLayer(index);
}

int Sprite::layerToIndex(const Layer* layer) const
{
  return m_impl->layerToIndex(layer);
}

//////////////////////////////////////////////////////////////////////
// Palettes

Palette* Sprite::getPalette(int frame) const
{
  return m_impl->getPalette(frame);
}

PalettesList Sprite::getPalettes() const
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

Palette* Sprite::getCurrentPalette() const
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

Stock* Sprite::getStock() const
{
  return m_impl->getStock();
}

Image* Sprite::getCurrentImage(int* x, int* y, int* opacity) const
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
// Mask

Mask* Sprite::getMask() const
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

MasksList Sprite::getMasksRepository()
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

PathsList Sprite::getPathsRepository()
{
  return m_impl->getPathsRepository();
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
