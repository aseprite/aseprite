// Aseprite Document Library
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/sprite.h"

#include "base/base.h"
#include "base/memory.h"
#include "base/remove_from_container.h"
#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/frame_tag.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/remap.h"
#include "doc/rgbmap.h"

#include <cstring>
#include <vector>

namespace doc {

//////////////////////////////////////////////////////////////////////
// Constructors/Destructor

Sprite::Sprite(PixelFormat format, int width, int height, int ncolors)
  : Object(ObjectType::Sprite)
  , m_document(NULL)
  , m_spec((ColorMode)format, width, height, 0)
  , m_pixelRatio(1, 1)
  , m_frames(1)
  , m_frameTags(this)
  , m_slices(this)
{
  ASSERT(width > 0 && height > 0);

  m_frlens.push_back(100);      // First frame with 100 msecs of duration
  m_root = new LayerGroup(this);

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

  setPalette(&pal, true);
}

Sprite::Sprite(const ImageSpec& spec, int ncolors)
  : Sprite((PixelFormat)spec.colorMode(), spec.width(), spec.height(), ncolors)
{
}

Sprite::~Sprite()
{
  // Destroy layers
  delete m_root;

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
    sprite->root()->addLayer(layer.release()); // Release the layer because it's owned by the sprite
  }

  return sprite.release();
}

//////////////////////////////////////////////////////////////////////
// Main properties

void Sprite::setPixelFormat(PixelFormat format)
{
  m_spec.setColorMode((ColorMode)format);
}

void Sprite::setPixelRatio(const PixelRatio& pixelRatio)
{
  m_pixelRatio = pixelRatio;
}

void Sprite::setSize(int width, int height)
{
  ASSERT(width > 0);
  ASSERT(height > 0);

  m_spec.setSize(width, height);
}

bool Sprite::needAlpha() const
{
  switch (pixelFormat()) {
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
  switch (pixelFormat()) {
    case IMAGE_RGB:
    case IMAGE_GRAYSCALE:
      return true;
  }
  return false;
}

void Sprite::setTransparentColor(color_t color)
{
  m_spec.setMaskColor(color);

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

LayerImage* Sprite::backgroundLayer() const
{
  if (root()->layersCount() > 0) {
    Layer* bglayer = root()->layers().front();

    if (bglayer->isBackground()) {
      ASSERT(bglayer->isImage());
      return static_cast<LayerImage*>(bglayer);
    }
  }
  return NULL;
}

Layer* Sprite::firstBrowsableLayer() const
{
  Layer* layer = root()->firstLayer();
  while (layer->isBrowsable())
    layer = static_cast<LayerGroup*>(layer)->firstLayer();
  return layer;
}

layer_t Sprite::allLayersCount() const
{
  return root()->allLayersCount();
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

  root()->displaceFrames(newFrame, +1);
}

void Sprite::removeFrame(frame_t frame)
{
  root()->displaceFrames(frame, -1);

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
  ASSERT(pixelFormat() == IMAGE_INDEXED);
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

void Sprite::pickCels(const double x,
                      const double y,
                      const frame_t frame,
                      const int opacityThreshold,
                      const LayerList& layers,
                      CelList& cels) const
{
  gfx::PointF pos(x, y);

  for (int i=(int)layers.size()-1; i>=0; --i) {
    const Layer* layer = layers[i];
    if (!layer->isImage())
      continue;

    Cel* cel = layer->cel(frame);
    if (!cel)
      continue;

    const Image* image = cel->image();
    if (!image)
      continue;

    gfx::RectF celBounds;
    if (cel->layer()->isReference())
      celBounds = cel->boundsF();
    else
      celBounds = cel->bounds();

    if (!celBounds.contains(pos))
      continue;

    const gfx::Point ipos(
      int((pos.x-celBounds.x)*image->width()/celBounds.w),
      int((pos.y-celBounds.y)*image->height()/celBounds.h));
    if (!image->bounds().contains(ipos))
      continue;

    const color_t color = get_pixel(image, ipos.x, ipos.y);
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
}

//////////////////////////////////////////////////////////////////////
// Iterators

LayerList Sprite::allLayers() const
{
  LayerList list;
  m_root->allLayers(list);
  return list;
}

LayerList Sprite::allVisibleLayers() const
{
  LayerList list;
  m_root->allVisibleLayers(list);
  return list;
}

LayerList Sprite::allVisibleReferenceLayers() const
{
  LayerList list;
  m_root->allVisibleReferenceLayers(list);
  return list;
}

LayerList Sprite::allBrowsableLayers() const
{
  LayerList list;
  m_root->allBrowsableLayers(list);
  return list;
}

CelsRange Sprite::cels() const
{
  SelectedFrames selFrames;
  selFrames.insert(0, lastFrame());
  return CelsRange(this, selFrames);
}

CelsRange Sprite::cels(frame_t frame) const
{
  SelectedFrames selFrames;
  selFrames.insert(frame);
  return CelsRange(this, selFrames);
}

CelsRange Sprite::uniqueCels() const
{
  SelectedFrames selFrames;
  selFrames.insert(0, lastFrame());
  return CelsRange(this, selFrames, CelsRange::UNIQUE);
}

CelsRange Sprite::uniqueCels(const SelectedFrames& selFrames) const
{
  return CelsRange(this, selFrames, CelsRange::UNIQUE);
}

} // namespace doc
