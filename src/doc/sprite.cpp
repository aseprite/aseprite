// Aseprite Document Library
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/sprite.h"

#include "base/memory.h"
#include "base/remove_from_container.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/octree_map.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/remap.h"
#include "doc/render_plan.h"
#include "doc/rgbmap_rgb5a3.h"
#include "doc/tag.h"
#include "doc/tilesets.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>

namespace doc {

static RgbMapAlgorithm g_rgbMapAlgorithm = RgbMapAlgorithm::DEFAULT;
static gfx::Rect g_defaultGridBounds(0, 0, 16, 16);

// static
gfx::Rect Sprite::DefaultGridBounds()
{
  return g_defaultGridBounds;
}

// static
void Sprite::SetDefaultGridBounds(const gfx::Rect& defGridBounds)
{
  g_defaultGridBounds = defGridBounds;
}

// static
RgbMapAlgorithm Sprite::DefaultRgbMapAlgorithm()
{
  return g_rgbMapAlgorithm;
}

// static
void Sprite::SetDefaultRgbMapAlgorithm(const RgbMapAlgorithm mapAlgo)
{
  g_rgbMapAlgorithm = mapAlgo;
}

//////////////////////////////////////////////////////////////////////
// Constructors/Destructor

Sprite::Sprite(const ImageSpec& spec,
               int ncolors)
  : WithUserData(ObjectType::Sprite)
  , m_document(nullptr)
  , m_spec(spec)
  , m_pixelRatio(1, 1)
  , m_frames(1)
  , m_frlens(1, 100)            // First frame with 100 msecs of duration
  , m_root(new LayerGroup(this))
  , m_gridBounds(Sprite::DefaultGridBounds())
  , m_tags(this)
  , m_slices(this)
  , m_tilesets(nullptr)
{
  // Generate palette
  switch (spec.colorMode()) {
    case ColorMode::GRAYSCALE: ncolors = 256; break;
    case ColorMode::BITMAP: ncolors = 2; break;
  }

  Palette pal(frame_t(0), ncolors);

  switch (spec.colorMode()) {

    // For black and white images
    case ColorMode::GRAYSCALE:
    case ColorMode::BITMAP:
      for (int c=0; c<ncolors; c++) {
        int g = 255 * c / (ncolors-1);
        g = std::clamp(g, 0, 255);
        pal.setEntry(c, rgba(g, g, g, 255));
      }
      break;
  }

  setPalette(&pal, true);
}

Sprite::~Sprite()
{
  // Destroy layers
  delete m_root;

  // Destroy tilesets
  delete m_tilesets;

  // Destroy palettes
  {
    PalettesList::iterator end = m_palettes.end();
    PalettesList::iterator it = m_palettes.begin();
    for (; it != end; ++it)
      delete *it;               // palette
  }
}

// static
Sprite* Sprite::MakeStdSprite(const ImageSpec& spec,
                              const int ncolors,
                              const ImageBufferPtr& imageBuf)
{
  // Create the sprite.
  std::unique_ptr<Sprite> sprite(new Sprite(spec, ncolors));
  sprite->setTotalFrames(frame_t(1));

  // Create the main image.
  ImageRef image(Image::create(spec, imageBuf));
  clear_image(image.get(), 0);

  // Create the first transparent layer.
  {
    std::unique_ptr<LayerImage> layer(new LayerImage(sprite.get()));
    layer->setName("Layer 1");

    // Create the cel.
    {
      std::unique_ptr<Cel> cel(new Cel(frame_t(0), image));
      cel->setPosition(0, 0);

      // Add the cel in the layer.
      layer->addCel(cel.get());
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

void Sprite::setColorSpace(const gfx::ColorSpaceRef& colorSpace)
{
  m_spec.setColorSpace(colorSpace);
  for (auto cel : uniqueCels())
    cel->image()->setColorSpace(colorSpace);
}

bool Sprite::isOpaque() const
{
  Layer* bg = backgroundLayer();
  return (bg && bg->isVisible());
}

bool Sprite::needAlpha() const
{
  switch (pixelFormat()) {
    case IMAGE_RGB:
    case IMAGE_GRAYSCALE:
      return !isOpaque();
    default:
      return false;
  }
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
#if _DEBUG
  if (colorMode() != ColorMode::INDEXED) {
    ASSERT(color == 0);
  }
#endif // _DEBUG

  m_spec.setMaskColor(color);

  // Change the mask color of all images.
  std::vector<ImageRef> images;
  getImages(images);
  for (ImageRef& image : images)
    image->setMaskColor(color);

  // Transform the empty tile of all tilemaps
  if (hasTilesets()) {
    for (Tileset* tileset : *tilesets()) {
      if (tileset)
        tileset->notifyRegenerateEmptyTile();
    }
  }
}

int Sprite::getMemSize() const
{
  int size = 0;

  std::vector<ImageRef> images;
  getImages(images);
  for (const ImageRef& image : images)
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

Layer* Sprite::firstLayer() const
{
  Layer* layer = root()->firstLayer();
  while (layer->isGroup())
    layer = static_cast<LayerGroup*>(layer)->firstLayer();
  return layer;
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

bool Sprite::hasVisibleReferenceLayers() const
{
  return root()->hasVisibleReferenceLayers();
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

Sprite::RgbMapFor Sprite::rgbMapForSprite() const
{
  return backgroundLayer() ? RgbMapFor::OpaqueLayer:
                             RgbMapFor::TransparentLayer;
}

RgbMap* Sprite::rgbMap(const frame_t frame) const
{
  return rgbMap(frame, rgbMapForSprite());
}

RgbMap* Sprite::rgbMap(const frame_t frame,
                       const RgbMapFor forLayer) const
{
  return rgbMap(frame,
                forLayer,
                g_rgbMapAlgorithm);
}

RgbMap* Sprite::rgbMap(const frame_t frame,
                       const RgbMapFor forLayer,
                       RgbMapAlgorithm mapAlgo) const
{
  if (!m_rgbMap || m_rgbMapAlgorithm != mapAlgo) {
    m_rgbMapAlgorithm = mapAlgo;
    switch (m_rgbMapAlgorithm) {
      case RgbMapAlgorithm::RGB5A3: m_rgbMap.reset(new RgbMapRGB5A3); break;
      case RgbMapAlgorithm::DEFAULT:
      case RgbMapAlgorithm::OCTREE: m_rgbMap.reset(new OctreeMap); break;
      default:
        m_rgbMap.reset(nullptr);
        ASSERT(false);
        return nullptr;
    }
  }
  int maskIndex;
  if (forLayer == RgbMapFor::OpaqueLayer)
    maskIndex = -1;
  else {
    maskIndex = palette(frame)->findMaskColor();
    if (maskIndex == -1)
      maskIndex = 0;
  }
  m_rgbMap->regenerateMap(palette(frame), maskIndex);
  return m_rgbMap.get();
}

//////////////////////////////////////////////////////////////////////
// Frames

void Sprite::addFrame(frame_t newFrame)
{
  setTotalFrames(m_frames+1);

  frame_t to = std::max(1, newFrame);
  for (frame_t i=m_frames-1; i>=to; --i)
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
  frames = std::max(frame_t(1), frames);
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

int Sprite::totalAnimationDuration() const
{
  int duration = 0;
  for (frame_t frame=0; frame<m_frames; ++frame)
    duration += frameDuration(frame);
  return duration; // TODO cache this value
}

void Sprite::setFrameDuration(frame_t frame, int msecs)
{
  if (frame >= 0 && frame < m_frames)
    m_frlens[frame] = std::clamp(msecs, 1, 65535);
}

void Sprite::setFrameRangeDuration(frame_t from, frame_t to, int msecs)
{
  std::fill(
    m_frlens.begin()+(std::size_t)from,
    m_frlens.begin()+(std::size_t)to+1, std::clamp(msecs, 1, 65535));
}

void Sprite::setDurationForAllFrames(int msecs)
{
  std::fill(m_frlens.begin(), m_frlens.end(), std::clamp(msecs, 1, 65535));
}

//////////////////////////////////////////////////////////////////////
// Shared Images and CelData (for linked cels and tilesets)

ImageRef Sprite::getImageRef(ObjectId imageId)
{
  for (Cel* cel : cels()) {
    if (cel->image()->id() == imageId)
      return cel->imageRef();
  }
  if (hasTilesets()) {
    for (Tileset* tileset : *tilesets()) {
      if (!tileset)
        continue;

      for (tile_index i=0; i<tileset->size(); ++i) {
        ImageRef image = tileset->get(i);
        if (image && image->id() == imageId)
          return image;
      }
    }
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
      cel->data()->setImage(newImage, cel->layer());
  }

  if (hasTilesets()) {
    for (Tileset* tileset : *tilesets()) {
      if (!tileset)
        continue;

      for (tile_index i=0; i<tileset->size(); ++i) {
        ImageRef image = tileset->get(i);
        if (image && image->id() == curImageId) {
          // Change only the tile image (not its user data)
          tileset->set(i, newImage);
        }
      }
    }
  }
}

void Sprite::replaceTileset(tileset_index tsi, Tileset* newTileset)
{
  ASSERT(hasTilesets());

  tilesets()->set(tsi, newTileset);

  for (Layer* layer : allLayers()) {
    if (layer->isTilemap() &&
        static_cast<LayerTilemap*>(layer)->tilesetIndex() == tsi) {
      // Set same index just to update the internal tileset pointer in
      // LayerTilemap
      static_cast<LayerTilemap*>(layer)->setTilesetIndex(tsi);
    }
  }
}

// TODO replace it with a images iterator
void Sprite::getImages(std::vector<ImageRef>& images) const
{
  for (Cel* cel : uniqueCels())
    if (cel->image()->pixelFormat() != IMAGE_TILEMAP)
      images.push_back(cel->imageRef());

  if (hasTilesets()) {
    for (Tileset* tileset : *tilesets()) {
      if (!tileset)
        continue;

      for (tile_index i=0; i<tileset->size(); ++i) {
        ImageRef image = tileset->get(i);
        if (image)
          images.push_back(image);
      }
    }
  }
}

void Sprite::getTilemapsByTileset(const Tileset* tileset,
                                  std::vector<ImageRef>& images) const
{
  for (const Cel* cel : uniqueCels()) {
    if (cel->layer()->isTilemap() &&
        static_cast<LayerTilemap*>(cel->layer())->tileset() == tileset) {
      images.push_back(cel->imageRef());
    }
  }
}

void Sprite::remapImages(const Remap& remap)
{
  ASSERT(pixelFormat() == IMAGE_INDEXED);
  //ASSERT(remap.size() == 256);

  std::vector<ImageRef> images;
  getImages(images);
  for (ImageRef& image : images)
    remap_image(image.get(), remap);
}

void Sprite::remapTilemaps(const Tileset* tileset,
                           const Remap& remap)
{
  for (Cel* cel : uniqueCels()) {
    if (cel->layer()->isTilemap() &&
        static_cast<LayerTilemap*>(cel->layer())->tileset() == tileset) {
      remap_image(cel->image(), remap);
    }
  }
}

//////////////////////////////////////////////////////////////////////
// Drawing

void Sprite::pickCels(const gfx::PointF& pos,
                      const int opacityThreshold,
                      const RenderPlan& plan,
                      CelList& cels) const
{
  // Iterate cels in reversed order (from the front-most to the
  // bottom-most) so we pick first visible cel in the given position.
  const auto& planItems = plan.items();
  for (auto it=planItems.rbegin(), end=planItems.rend(); it!=end; ++it) {
    const Cel* cel = it->cel;
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

    gfx::Point ipos;
    if (image->isTilemap()) {
      Tileset* tileset = static_cast<LayerTilemap*>(cel->layer())->tileset();
      if (!tileset)
        continue;

      const Grid grid = cel->grid();

      tile_t tile = notile;
      gfx::Point tilePos = grid.canvasToTile(gfx::Point(pos));
      if (image->bounds().contains(tilePos.x, tilePos.y))
        tile = image->getPixel(tilePos.x, tilePos.y);
      if (tile == notile)
        continue;

      image = tileset->get(tile).get();
      if (!image)
        continue;

      gfx::Point tileStart = grid.tileToCanvas(tilePos);
      ipos = gfx::Point(pos.x - tileStart.x,
                        pos.y - tileStart.y);
    }
    else {
      ipos = gfx::Point(
        int((pos.x-celBounds.x)*image->width()/celBounds.w),
        int((pos.y-celBounds.y)*image->height()/celBounds.h));
    }

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

    cels.push_back(const_cast<Cel*>(cel));
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

LayerList Sprite::allTilemaps() const
{
  LayerList list;
  m_root->allTilemaps(list);
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

CelsRange Sprite::cels(const SelectedFrames& selFrames) const
{
  return CelsRange(this, selFrames, CelsRange::ALL);
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

////////////////////////////////////////
// Tilesets

Tilesets* Sprite::tilesets() const
{
  if (!m_tilesets)
    m_tilesets = new Tilesets;
  return m_tilesets;
}

} // namespace doc
