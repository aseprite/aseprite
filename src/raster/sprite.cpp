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

#include "raster/sprite.h"

#include "base/memory.h"
#include "base/remove_from_container.h"
#include "raster/raster.h"

#include <cstring>
#include <vector>

static Layer* index2layer(const Layer* layer, int index, int* index_count);
static int layer2index(const Layer* layer, const Layer* find_layer, int* index_count);

//////////////////////////////////////////////////////////////////////
// Constructors/Destructor

Sprite::Sprite(int imgtype, int width, int height, int ncolors)
  : GfxObj(GFXOBJ_SPRITE)
  , m_imgtype(imgtype)
  , m_width(width)
  , m_height(height)
{
  ASSERT(width > 0 && height > 0);

  m_frames = 1;
  m_frlens.push_back(100);	// First frame with 100 msecs of duration
  m_frame = 0;
  m_stock = new Stock(imgtype);
  m_folder = new LayerFolder(this);
  m_layer = NULL;

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

Sprite::~Sprite()
{
  // Destroy layers
  delete m_folder;

  // Destroy images' stock
  if (m_stock)
    delete m_stock;

  // Destroy palettes
  {
    PalettesList::iterator end = m_palettes.end();
    PalettesList::iterator it = m_palettes.begin();
    for (; it != end; ++it)
      delete *it;		// palette
  }

  // Destroy RGB map
  delete m_rgbMap;
}

//////////////////////////////////////////////////////////////////////
// Main properties

void Sprite::setImgType(int imgtype)
{
  m_imgtype = imgtype;
}

void Sprite::setSize(int width, int height)
{
  ASSERT(width > 0);
  ASSERT(height > 0);

  m_width = width;
  m_height = height;
}

bool Sprite::needAlpha() const
{
  switch (m_imgtype) {
    case IMAGE_RGB:
    case IMAGE_GRAYSCALE:
      return (getBackgroundLayer() == NULL);
  }
  return false;
}

void Sprite::setTransparentColor(uint32_t color)
{
  m_transparentColor = color;
}

int Sprite::getMemSize() const
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

//////////////////////////////////////////////////////////////////////
// Layers

LayerFolder* Sprite::getFolder() const
{
  return m_folder;
}

LayerImage* Sprite::getBackgroundLayer() const
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

Layer* Sprite::getCurrentLayer() const
{
  return m_layer;
}

void Sprite::setCurrentLayer(Layer* layer)
{
  m_layer = layer;
}

int Sprite::countLayers() const
{
  return getFolder()->get_layers_count();
}

Layer* Sprite::indexToLayer(int index) const
{
  int index_count = -1;
  return index2layer(getFolder(), index, &index_count);
}

int Sprite::layerToIndex(const Layer* layer) const
{
  int index_count = -1;
  return layer2index(getFolder(), layer, &index_count);
}

//////////////////////////////////////////////////////////////////////
// Palettes

Palette* Sprite::getPalette(int frame) const
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

PalettesList Sprite::getPalettes() const
{
  return m_palettes;
}

void Sprite::setPalette(Palette* pal, bool truncate)
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

void Sprite::resetPalettes()
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

void Sprite::deletePalette(Palette* pal)
{
  ASSERT(pal != NULL);

  base::remove_from_container(m_palettes, pal);
  delete pal;			// palette
}

Palette* Sprite::getCurrentPalette() const
{
  return getPalette(getCurrentFrame());
}

RgbMap* Sprite::getRgbMap()
{
  return getRgbMap(getCurrentFrame());
}

RgbMap* Sprite::getRgbMap(int frame)
{
  if (m_rgbMap == NULL) {
    m_rgbMap = new RgbMap();
    m_rgbMap->regenerate(getPalette(frame));
  }
  else if (!m_rgbMap->match(getPalette(frame))) {
    m_rgbMap->regenerate(getPalette(frame));
  }
  return m_rgbMap;
}

//////////////////////////////////////////////////////////////////////
// Frames

void Sprite::setTotalFrames(int frames)
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

int Sprite::getFrameDuration(int frame) const
{
  if (frame >= 0 && frame < m_frames)
    return m_frlens[frame];
  else
    return 0;
}

void Sprite::setFrameDuration(int frame, int msecs)
{
  if (frame >= 0 && frame < m_frames)
    m_frlens[frame] = MID(1, msecs, 65535);
}

void Sprite::setDurationForAllFrames(int msecs)
{
  std::fill(m_frlens.begin(), m_frlens.end(), MID(1, msecs, 65535));
}

void Sprite::setCurrentFrame(int frame)
{
  m_frame = frame;
}

//////////////////////////////////////////////////////////////////////
// Images

Stock* Sprite::getStock() const
{
  return m_stock;
}

Image* Sprite::getCurrentImage(int* x, int* y, int* opacity) const
{
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

void Sprite::getCels(CelList& cels)
{
  getFolder()->getCels(cels);
}

void Sprite::remapImages(int frame_from, int frame_to, const std::vector<uint8_t>& mapping)
{
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

//////////////////////////////////////////////////////////////////////
// Drawing

void Sprite::render(Image* image, int x, int y) const
{
  image_rectfill(image, x, y, x+m_width-1, y+m_height-1, 0);
  layer_render(getFolder(), image, x, y, getCurrentFrame());
}

int Sprite::getPixel(int x, int y) const
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
