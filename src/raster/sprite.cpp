/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#include "raster/sprite.h"

#include "base/memory.h"
#include "base/remove_from_container.h"
#include "base/unique_ptr.h"
#include "raster/image_bits.h"
#include "raster/primitives.h"
#include "raster/raster.h"

#include <cstring>
#include <vector>

namespace raster {

static Layer* index2layer(const Layer* layer, const LayerIndex& index, int* index_count);
static LayerIndex layer2index(const Layer* layer, const Layer* find_layer, int* index_count);

//////////////////////////////////////////////////////////////////////
// Constructors/Destructor

Sprite::Sprite(PixelFormat format, int width, int height, int ncolors)
  : Object(OBJECT_SPRITE)
  , m_format(format)
  , m_width(width)
  , m_height(height)
  , m_frames(1)
{
  ASSERT(width > 0 && height > 0);

  m_frlens.push_back(100);      // First frame with 100 msecs of duration
  m_stock = new Stock(format);
  m_folder = new LayerFolder(this);

  // Generate palette
  Palette pal(FrameNumber(0), ncolors);

  switch (format) {

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
        pal.setEntry(c, rgba(g, g, g, 255));
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
      delete *it;               // palette
  }

  // Destroy RGB map
  delete m_rgbMap;
}

//////////////////////////////////////////////////////////////////////
// Main properties

void Sprite::setPixelFormat(PixelFormat format)
{
  m_format = format;
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
  switch (m_format) {
    case IMAGE_RGB:
    case IMAGE_GRAYSCALE:
      return (getBackgroundLayer() == NULL);
  }
  return false;
}

void Sprite::setTransparentColor(color_t color)
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
      size += image->getRowStrideSize() * image->getHeight();
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
  if (getFolder()->getLayersCount() > 0) {
    Layer* bglayer = *getFolder()->getLayerBegin();

    if (bglayer->isBackground()) {
      ASSERT(bglayer->isImage());
      return static_cast<LayerImage*>(bglayer);
    }
  }
  return NULL;
}

LayerIndex Sprite::countLayers() const
{
  return LayerIndex(getFolder()->getLayersCount());
}

Layer* Sprite::indexToLayer(LayerIndex index) const
{
  int index_count = -1;
  return index2layer(getFolder(), index, &index_count);
}

LayerIndex Sprite::layerToIndex(const Layer* layer) const
{
  int index_count = -1;
  return layer2index(getFolder(), layer, &index_count);
}

//////////////////////////////////////////////////////////////////////
// Palettes

Palette* Sprite::getPalette(FrameNumber frame) const
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

const PalettesList& Sprite::getPalettes() const
{
  return m_palettes;
}

void Sprite::setPalette(const Palette* pal, bool truncate)
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
    ++it;                       // Leave the first palette only.
    while (it != end) {
      delete *it;               // palette
      it = m_palettes.erase(it);
      end = m_palettes.end();
    }
  }
}

void Sprite::deletePalette(Palette* pal)
{
  ASSERT(pal != NULL);

  base::remove_from_container(m_palettes, pal);
  delete pal;                   // palette
}

RgbMap* Sprite::getRgbMap(FrameNumber frame)
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

void Sprite::addFrame(FrameNumber newFrame)
{
  setTotalFrames(m_frames.next());
  for (FrameNumber i=m_frames.previous(); i>=newFrame; i=i.previous())
    setFrameDuration(i, getFrameDuration(i.previous()));
}

void Sprite::removeFrame(FrameNumber newFrame)
{
  FrameNumber newTotal = m_frames.previous();
  for (FrameNumber i=newFrame; i<newTotal; i=i.next())
    setFrameDuration(i, getFrameDuration(i.next()));
  setTotalFrames(newTotal);
}

void Sprite::setTotalFrames(FrameNumber frames)
{
  frames = MAX(FrameNumber(1), frames);
  m_frlens.resize(frames);

  if (frames > m_frames) {
    for (FrameNumber c=m_frames; c<frames; ++c)
      m_frlens[c] = m_frlens[m_frames.previous()];
  }

  m_frames = frames;
}

int Sprite::getFrameDuration(FrameNumber frame) const
{
  if (frame >= 0 && frame < m_frames)
    return m_frlens[frame];
  else
    return 0;
}

void Sprite::setFrameDuration(FrameNumber frame, int msecs)
{
  if (frame >= 0 && frame < m_frames)
    m_frlens[frame] = MID(1, msecs, 65535);
}

void Sprite::setFrameRangeDuration(FrameNumber from, FrameNumber to, int msecs)
{
  std::fill(
    m_frlens.begin()+(size_t)from,
    m_frlens.begin()+(size_t)to+1, MID(1, msecs, 65535));
}

void Sprite::setDurationForAllFrames(int msecs)
{
  std::fill(m_frlens.begin(), m_frlens.end(), MID(1, msecs, 65535));
}

//////////////////////////////////////////////////////////////////////
// Images

Stock* Sprite::getStock() const
{
  return m_stock;
}

void Sprite::getCels(CelList& cels) const
{
  getFolder()->getCels(cels);
}

size_t Sprite::getImageRefs(int imageIndex) const
{
  CelList cels;
  getCels(cels);

  size_t refs = 0;
  for (CelList::iterator it=cels.begin(), end=cels.end(); it != end; ++it)
    if ((*it)->getImage() == imageIndex)
      ++refs;

  return refs;
}

void Sprite::remapImages(FrameNumber frameFrom, FrameNumber frameTo, const std::vector<uint8_t>& mapping)
{
  ASSERT(m_format == IMAGE_INDEXED);
  ASSERT(mapping.size() == 256);

  CelList cels;
  getCels(cels);

  for (CelIterator it = cels.begin(); it != cels.end(); ++it) {
    Cel* cel = *it;

    // Remap this Cel because is inside the specified range
    if (cel->getFrame() >= frameFrom &&
        cel->getFrame() <= frameTo) {
      Image* image = getStock()->getImage(cel->getImage());
      LockImageBits<IndexedTraits> bits(image);
      LockImageBits<IndexedTraits>::iterator
        it = bits.begin(),
        end = bits.end();

      for (; it != end; ++it)
        *it = mapping[*it];
    }
  }
}

//////////////////////////////////////////////////////////////////////
// Drawing

void Sprite::render(Image* image, int x, int y, FrameNumber frame) const
{
  fill_rect(image, x, y, x+m_width-1, y+m_height-1,
            (m_format == IMAGE_INDEXED ? getTransparentColor(): 0));

  layer_render(getFolder(), image, x, y, frame);
}

int Sprite::getPixel(int x, int y, FrameNumber frame) const
{
  int color = 0;

  if ((x >= 0) && (y >= 0) && (x < m_width) && (y < m_height)) {
    base::UniquePtr<Image> image(Image::create(m_format, 1, 1));
    clear_image(image, (m_format == IMAGE_INDEXED ? getTransparentColor(): 0));
    render(image, -x, -y, frame);
    color = get_pixel(image, 0, 0);
  }

  return color;
}

//////////////////////////////////////////////////////////////////////

static Layer* index2layer(const Layer* layer, const LayerIndex& index, int* index_count)
{
  if (index == *index_count)
    return (Layer*)layer;
  else {
    (*index_count)++;

    if (layer->isFolder()) {
      Layer *found;

      LayerConstIterator it = static_cast<const LayerFolder*>(layer)->getLayerBegin();
      LayerConstIterator end = static_cast<const LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it) {
        if ((found = index2layer(*it, index, index_count)))
          return found;
      }
    }

    return NULL;
  }
}

static LayerIndex layer2index(const Layer* layer, const Layer* find_layer, int* index_count)
{
  if (layer == find_layer)
    return LayerIndex(*index_count);
  else {
    (*index_count)++;

    if (layer->isFolder()) {
      int found;

      LayerConstIterator it = static_cast<const LayerFolder*>(layer)->getLayerBegin();
      LayerConstIterator end = static_cast<const LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it) {
        if ((found = layer2index(*it, find_layer, index_count)) >= 0)
          return LayerIndex(found);
      }
    }

    return LayerIndex(-1);
  }
}

} // namespace raster
