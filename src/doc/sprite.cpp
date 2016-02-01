// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/sprite.h"

#include "base/memory.h"
#include "base/remove_from_container.h"
#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/frame_tag.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/layers_range.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/remap.h"
#include "doc/rgbmap.h"

#include <cstring>
#include <vector>

namespace doc {

static Layer* index2layer(const Layer* layer, const LayerIndex& index, int* index_count);
static LayerIndex layer2index(const Layer* layer, const Layer* find_layer, int* index_count);

//////////////////////////////////////////////////////////////////////
// Constructors/Destructor

Sprite::Sprite(PixelFormat format, int width, int height, int ncolors)
  : Object(ObjectType::Sprite)
  , m_document(NULL)
  , m_format(format)
  , m_width(width)
  , m_height(height)
  , m_frames(1)
  , m_frameTags(this)
{
  ASSERT(width > 0 && height > 0);

  m_frlens.push_back(100);      // First frame with 100 msecs of duration
  m_folder = new LayerFolder(this);

  // Generate palette
  switch (format) {
    case IMAGE_GRAYSCALE: ncolors = 256; break;
    case IMAGE_BITMAP: ncolors = 2; break;
  }

  Palette pal(frame_t(0), ncolors);

  switch (format) {

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

// static
Sprite* Sprite::createBasicSprite(doc::PixelFormat format, int width, int height, int ncolors)
{
  // Create the sprite.
  base::UniquePtr<doc::Sprite> sprite(new doc::Sprite(format, width, height, ncolors));
  sprite->setTotalFrames(doc::frame_t(1));

  // Create the main image.
  doc::ImageRef image(doc::Image::create(format, width, height));
  doc::clear_image(image.get(), 0);

  // Create the first transparent layer.
  {
    base::UniquePtr<doc::LayerImage> layer(new doc::LayerImage(sprite));
    layer->setName("Layer 1");

    // Create the cel.
    {
      base::UniquePtr<doc::Cel> cel(new doc::Cel(doc::frame_t(0), image));
      cel->setPosition(0, 0);

      // Add the cel in the layer.
      layer->addCel(cel);
      cel.release();            // Release the cel because it is in the layer
    }

    // Add the layer in the sprite.
    sprite->folder()->addLayer(layer.release()); // Release the layer because it's owned by the sprite
  }

  return sprite.release();
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
    case IMAGE_GRAYSCALE: {
      Layer* bg = backgroundLayer();
      return (!bg || !bg->isVisible());
    }
  }
  return false;
}

bool Sprite::supportAlpha() const
{
  switch (m_format) {
    case IMAGE_RGB:
    case IMAGE_GRAYSCALE:
      return true;
  }
  return false;
}

void Sprite::setTransparentColor(color_t color)
{
  m_transparentColor = color;

  // Change the mask color of all images.
  std::vector<Image*> images;
  getImages(images);
  for (Image* image : images)
    image->setMaskColor(color);
}

int Sprite::getMemSize() const
{
  int size = 0;

  std::vector<Image*> images;
  getImages(images);
  for (Image* image : images)
    size += image->getRowStrideSize() * image->height();

  return size;
}

//////////////////////////////////////////////////////////////////////
// Layers

LayerFolder* Sprite::folder() const
{
  return m_folder;
}

LayerImage* Sprite::backgroundLayer() const
{
  if (folder()->getLayersCount() > 0) {
    Layer* bglayer = *folder()->getLayerBegin();

    if (bglayer->isBackground()) {
      ASSERT(bglayer->isImage());
      return static_cast<LayerImage*>(bglayer);
    }
  }
  return NULL;
}

LayerIndex Sprite::countLayers() const
{
  return LayerIndex(folder()->getLayersCount());
}

LayerIndex Sprite::firstLayer() const
{
  return LayerIndex(0);
}

LayerIndex Sprite::lastLayer() const
{
  return LayerIndex(folder()->getLayersCount()-1);
}

Layer* Sprite::layer(int layerIndex) const
{
  return indexToLayer(LayerIndex(layerIndex));
}

Layer* Sprite::indexToLayer(LayerIndex index) const
{
  if (index < LayerIndex(0))
    return NULL;

  int index_count = -1;
  return index2layer(folder(), index, &index_count);
}

LayerIndex Sprite::layerToIndex(const Layer* layer) const
{
  int index_count = -1;
  return layer2index(folder(), layer, &index_count);
}

void Sprite::getLayersList(std::vector<Layer*>& layers) const
{
  // TODO support subfolders
  LayerConstIterator it = m_folder->getLayerBegin();
  LayerConstIterator end = m_folder->getLayerEnd();

  for (; it != end; ++it) {
    layers.push_back(*it);
  }
}

//////////////////////////////////////////////////////////////////////
// Palettes

Palette* Sprite::palette(frame_t frame) const
{
  ASSERT(frame >= 0);

  Palette* found = NULL;

  PalettesList::const_iterator end = m_palettes.end();
  PalettesList::const_iterator it = m_palettes.begin();
  for (; it != end; ++it) {
    Palette* pal = *it;
    if (frame < pal->frame())
      break;

    found = pal;
    if (frame == pal->frame())
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
    Palette* sprite_pal = palette(pal->frame());
    pal->copyColorsTo(sprite_pal);
  }
  else {
    Palette* other;

    PalettesList::iterator end = m_palettes.end();
    PalettesList::iterator it = m_palettes.begin();
    for (; it != end; ++it) {
      other = *it;

      if (pal->frame() == other->frame()) {
        pal->copyColorsTo(other);
        return;
      }
      else if (pal->frame() < other->frame())
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

void Sprite::deletePalette(frame_t frame)
{
  auto it = m_palettes.begin(), end = m_palettes.end();
  for (; it != end; ++it) {
    Palette* pal = *it;

    if (pal->frame() == frame) {
      delete pal;                   // delete palette
      m_palettes.erase(it);
      break;
    }
  }
}

RgbMap* Sprite::rgbMap(frame_t frame) const
{
  return rgbMap(frame, backgroundLayer() ? RgbMapFor::OpaqueLayer:
                                           RgbMapFor::TransparentLayer);
}

RgbMap* Sprite::rgbMap(frame_t frame, RgbMapFor forLayer) const
{
  int maskIndex = (forLayer == RgbMapFor::OpaqueLayer ?
                   -1: transparentColor());

  if (m_rgbMap == NULL) {
    m_rgbMap = new RgbMap();
    m_rgbMap->regenerate(palette(frame), maskIndex);
  }
  else if (!m_rgbMap->match(palette(frame)) ||
           m_rgbMap->maskIndex() != maskIndex) {
    m_rgbMap->regenerate(palette(frame), maskIndex);
  }

  return m_rgbMap;
}

//////////////////////////////////////////////////////////////////////
// Frames

void Sprite::addFrame(frame_t newFrame)
{
  setTotalFrames(m_frames+1);
  for (frame_t i=m_frames-1; i>=newFrame; --i)
    setFrameDuration(i, frameDuration(i-1));

  folder()->displaceFrames(newFrame, +1);
}

void Sprite::removeFrame(frame_t frame)
{
  folder()->displaceFrames(frame, -1);

  frame_t newTotal = m_frames-1;
  for (frame_t i=frame; i<newTotal; ++i)
    setFrameDuration(i, frameDuration(i+1));
  setTotalFrames(newTotal);
}

void Sprite::setTotalFrames(frame_t frames)
{
  frames = MAX(frame_t(1), frames);
  m_frlens.resize(frames);

  if (frames > m_frames) {
    for (frame_t c=m_frames; c<frames; ++c)
      m_frlens[c] = m_frlens[m_frames-1];
  }

  m_frames = frames;
}

int Sprite::frameDuration(frame_t frame) const
{
  if (frame >= 0 && frame < m_frames)
    return m_frlens[frame];
  else
    return 0;
}

void Sprite::setFrameDuration(frame_t frame, int msecs)
{
  if (frame >= 0 && frame < m_frames)
    m_frlens[frame] = MID(1, msecs, 65535);
}

void Sprite::setFrameRangeDuration(frame_t from, frame_t to, int msecs)
{
  std::fill(
    m_frlens.begin()+(std::size_t)from,
    m_frlens.begin()+(std::size_t)to+1, MID(1, msecs, 65535));
}

void Sprite::setDurationForAllFrames(int msecs)
{
  std::fill(m_frlens.begin(), m_frlens.end(), MID(1, msecs, 65535));
}

//////////////////////////////////////////////////////////////////////
// Shared Images and CelData (for linked Cels)

ImageRef Sprite::getImageRef(ObjectId imageId)
{
  for (Cel* cel : cels()) {
    if (cel->image()->id() == imageId)
      return cel->imageRef();
  }
  return ImageRef(nullptr);
}

CelDataRef Sprite::getCelDataRef(ObjectId celDataId)
{
  for (Cel* cel : cels()) {
    if (cel->dataRef()->id() == celDataId)
      return cel->dataRef();
  }
  return CelDataRef(nullptr);
}

//////////////////////////////////////////////////////////////////////
// Images

void Sprite::replaceImage(ObjectId curImageId, const ImageRef& newImage)
{
  for (Cel* cel : cels()) {
    if (cel->image()->id() == curImageId)
      cel->data()->setImage(newImage);
  }
}

// TODO replace it with a images iterator
void Sprite::getImages(std::vector<Image*>& images) const
{
  for (const auto& cel : uniqueCels())
    images.push_back(cel->image());
}

void Sprite::remapImages(frame_t frameFrom, frame_t frameTo, const Remap& remap)
{
  ASSERT(m_format == IMAGE_INDEXED);
  //ASSERT(remap.size() == 256);

  for (const Cel* cel : uniqueCels()) {
    // Remap this Cel because is inside the specified range
    if (cel->frame() >= frameFrom &&
        cel->frame() <= frameTo) {
      remap_image(cel->image(), remap);
    }
  }
}

//////////////////////////////////////////////////////////////////////
// Drawing

void Sprite::pickCels(int x, int y, frame_t frame, int opacityThreshold, CelList& cels) const
{
  std::vector<Layer*> layers;
  getLayersList(layers);

  for (int i=(int)layers.size()-1; i>=0; --i) {
    Layer* layer = layers[i];
    if (!layer->isImage() || !layer->isVisible())
      continue;

    Cel* cel = layer->cel(frame);
    if (!cel)
      continue;

    Image* image = cel->image();
    if (!image)
      continue;

    if (!cel->bounds().contains(gfx::Point(x, y)))
      continue;

    color_t color = get_pixel(image,
      x - cel->x(),
      y - cel->y());

    bool isOpaque = true;

    switch (image->pixelFormat()) {
      case IMAGE_RGB:
        isOpaque = (rgba_geta(color) >= opacityThreshold);
        break;
      case IMAGE_INDEXED:
        isOpaque = (color != image->maskColor());
        break;
      case IMAGE_GRAYSCALE:
        isOpaque = (graya_geta(color) >= opacityThreshold);
        break;
    }

    if (!isOpaque)
      continue;

    cels.push_back(cel);
  }
  fflush(stdout);
}

//////////////////////////////////////////////////////////////////////
// Iterators

LayersRange Sprite::layers() const
{
  return LayersRange(this, LayerIndex(0), LayerIndex(countLayers()-1));
}

CelsRange Sprite::cels() const
{
  return CelsRange(this, frame_t(0), lastFrame());
}

CelsRange Sprite::cels(frame_t frame) const
{
  return CelsRange(this, frame, frame);
}

CelsRange Sprite::uniqueCels() const
{
  return CelsRange(this, frame_t(0), lastFrame(), CelsRange::UNIQUE);
}

CelsRange Sprite::uniqueCels(frame_t from, frame_t to) const
{
  return CelsRange(this, from, to, CelsRange::UNIQUE);
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

} // namespace doc
