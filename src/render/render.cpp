// Aseprite Render Library
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "render/render.h"

#include "base/gcd.h"
#include "doc/blend_internals.h"
#include "doc/blend_mode.h"
#include "doc/doc.h"
#include "doc/image.h"
#include "doc/layer_tilemap.h"
#include "doc/playback.h"
#include "doc/render_plan.h"
#include "doc/tileset.h"
#include "doc/tilesets.h"
#include "gfx/clip.h"
#include "gfx/region.h"

#include <cmath>

#define TRACE_RENDER_CEL(...) // TRACE

namespace render {

namespace {

//////////////////////////////////////////////////////////////////////
// Scaled composite

template<class DstTraits, class SrcTraits>
struct composite_image_without_scale {
  static void call(const CompositeParams& p)
  {
    ASSERT(p.dst);
    ASSERT(p.src);
    ASSERT(DstTraits::pixel_format == p.dst->pixelFormat());
    ASSERT(SrcTraits::pixel_format == p.src->pixelFormat());

    BlenderHelper<DstTraits, SrcTraits> blender(p.dst, p.src, p.pal, p.blendMode);

    gfx::Clip area(p.areaF);
    if (!area.clip(p.dst->width(), p.dst->height(), p.src->width(), p.src->height()))
      return;

    gfx::Rect srcBounds = area.srcBounds();
    ASSERT(!srcBounds.isEmpty());
    if (srcBounds.isEmpty())
      return;

    gfx::Rect dstBounds = area.dstBounds();
    ASSERT(!dstBounds.isEmpty());
    if (dstBounds.isEmpty())
      return;

    const int opacity = p.opacity;

    // Iterators
    auto src_it = p.src->readArea(srcBounds);
    auto dst_it = p.dst->writeArea(dstBounds);

    ASSERT(srcBounds.w == dstBounds.w);
    const int min_w = std::min(srcBounds.w, dstBounds.w);

    // For each line to draw of the source image...
    while (src_it.nextLine() && dst_it.nextLine()) {
      const auto* src_p = src_it.addr<SrcTraits>();
      auto* dst_p = dst_it.addr<DstTraits>();

      for (int x = 0; x < min_w; ++x) {
        p.src->assertInside(src_p);
        p.dst->assertInside(dst_p);

        *dst_p = blender(*dst_p, *src_p, opacity);
        ++src_p;
        ++dst_p;
      }
    }
  }
};

template<class DstTraits, class SrcTraits>
struct composite_image_scale_up {
  static void call(const CompositeParams& p)
  {
    ASSERT(p.dst);
    ASSERT(p.src);
    ASSERT(DstTraits::pixel_format == p.dst->pixelFormat());
    ASSERT(SrcTraits::pixel_format == p.src->pixelFormat());

    gfx::Clip area(p.areaF);
    if (!area.clip(p.dst->width(),
                   p.dst->height(),
                   int(p.sx * double(p.src->width())),
                   int(p.sy * double(p.src->height()))))
      return;

    BlenderHelper<DstTraits, SrcTraits> blender(p.dst, p.src, p.pal, p.blendMode);
    int px_w = int(p.sx);
    int px_h = int(p.sy);

    // We've received crash reports about these values being 0 when it's
    // called from Render::renderImage() when the projection is scaled
    // to the cel bounds (this can happen only when a reference layer is
    // scaled, but when a reference layer is visible we shouldn't be
    // here, we should be using the composite_image_general(), see the
    // "finegrain" var in Render::getImageComposition()).
    ASSERT(px_w > 0);
    ASSERT(px_h > 0);
    if (px_w <= 0 || px_h <= 0)
      return;

    int first_px_w = px_w - (area.src.x % px_w);
    int first_px_h = px_h - (area.src.y % px_h);

    gfx::Rect srcBounds = area.srcBounds();
    srcBounds.w = srcBounds.x2() / px_w - srcBounds.x / px_w;
    srcBounds.h = srcBounds.y2() / px_h - srcBounds.y / px_h;
    srcBounds.x /= px_w;
    srcBounds.y /= px_h;
    if ((area.src.x + area.size.w) % px_w > 0)
      ++srcBounds.w;
    if ((area.src.y + area.size.h) % px_h > 0)
      ++srcBounds.h;
    if (srcBounds.isEmpty())
      return;

    gfx::Rect dstBounds = area.dstBounds();
    const int opacity = p.opacity;

    // The scanline variable is used to blend src/dst pixels one time for each pixel
    using Scanline = std::vector<typename DstTraits::pixel_t>;
    Scanline scanline(srcBounds.w);
    typename Scanline::iterator scanline_it;
#ifdef _DEBUG
    typename Scanline::iterator scanline_end = scanline.end();
#endif

    // Iterators
    auto src_it = p.src->readArea(srcBounds);
    auto dst_it = p.dst->writeArea(dstBounds);
    if (!dst_it.nextLine())
      return;

    // For each line to draw of the source image...
    for (int y = 0; src_it.nextLine(); ++y) {
      // Read 'src' and 'dst' and blend them, put the result in 'scanline'
      auto* dst_p = dst_it.addr<DstTraits>();
      auto* dst_end = dst_p + dstBounds.w;
      {
        const auto* src_p = src_it.addr<SrcTraits>();

        scanline_it = scanline.begin();
        for (int x = 0; x < srcBounds.w && dst_p < dst_end; ++x) {
          auto* dst_px_end = std::min(dst_p + (x == 0 ? first_px_w : px_w), dst_end);
          p.src->assertInside(src_p);
          p.dst->assertInside(dst_px_end - 1);
          ASSERT(scanline_it < scanline_end);

          *scanline_it = blender(*dst_p, *src_p, opacity);
          dst_p = dst_px_end;
          ++src_p;
          ++scanline_it;
        }
      }

      // Draw the merged 'scanline' on 'dst', 'row_h' times
      int row_h = (y == 0 ? first_px_h : px_h);
      for (int px_y = 0; px_y < row_h; ++px_y) {
        dst_p = dst_it.addr<DstTraits>();
        dst_end = dst_p + dstBounds.w;
        scanline_it = scanline.begin();

        // First pixel
        int x = 0;
        ASSERT(first_px_w > 0);
        {
          auto* dst_px_end = std::min(dst_p + first_px_w, dst_end);
          p.dst->assertInside(dst_p);
          p.dst->assertInside(dst_px_end - 1);
          ASSERT(scanline_it < scanline_end);

          std::fill(dst_p, dst_px_end, *scanline_it);
          dst_p = dst_px_end;
          ++scanline_it;
          ++x;
        }

        // Other pixels
        for (; x < srcBounds.w; ++x) {
          auto* dst_px_end = std::min(dst_p + px_w, dst_end);
          p.dst->assertInside(dst_p);
          p.dst->assertInside(dst_px_end - 1);
          ASSERT(scanline_it < scanline_end);

          std::fill(dst_p, dst_px_end, *scanline_it);
          dst_p = dst_px_end;
          ++scanline_it;
        }

        if (!dst_it.nextLine())
          return;
      }
    }
  }
};

template<class DstTraits, class SrcTraits>
struct composite_image_scale_down {
  static void call(const CompositeParams& p)
  {
    ASSERT(p.dst);
    ASSERT(p.src);
    ASSERT(DstTraits::pixel_format == p.dst->pixelFormat());
    ASSERT(SrcTraits::pixel_format == p.src->pixelFormat());

    gfx::Clip area(p.areaF);
    if (!area.clip(p.dst->width(),
                   p.dst->height(),
                   int(p.sx * double(p.src->width())),
                   int(p.sy * double(p.src->height()))))
      return;

    BlenderHelper<DstTraits, SrcTraits> blender(p.dst, p.src, p.pal, p.blendMode);
    int step_w = int(std::round(1.0 / p.sx));
    int step_h = int(std::round(1.0 / p.sy));
    if (step_w < 1 || step_h < 1)
      return;

    gfx::Rect srcBounds = area.srcBounds();
    srcBounds.w = srcBounds.x2() * step_w - srcBounds.x * step_w;
    srcBounds.h = srcBounds.y2() * step_h - srcBounds.y * step_h;
    srcBounds.x *= step_w;
    srcBounds.y *= step_h;
    if (srcBounds.isEmpty())
      return;

    gfx::Rect dstBounds = area.dstBounds();
    const int opacity = p.opacity;

    // Iterators
    auto src_it = p.src->readArea(srcBounds);
    auto dst_it = p.dst->writeArea(dstBounds);
    if (!src_it.nextLine())
      return;

    // For each line to draw of the source image...
    for (int y = 0; y < dstBounds.h && dst_it.nextLine(); ++y) {
      const auto* src_p = src_it.addr<SrcTraits>();
      auto* dst_p = dst_it.addr<DstTraits>();

      for (int x = 0; x < dstBounds.w; ++x) {
        p.src->assertInside(src_p);
        p.dst->assertInside(dst_p);

        *dst_p = blender(*dst_p, *src_p, opacity);

        src_p += step_w; // Skip columns
        ++dst_p;
      }

      // Skip rows
      if (!src_it.nextLine(step_h))
        break;
    }
  }
};

template<class DstTraits, class SrcTraits>
struct composite_image_general {
  static void call(const CompositeParams& p)
  {
    ASSERT(p.dst);
    ASSERT(p.src);
    ASSERT(DstTraits::pixel_format == p.dst->pixelFormat());
    ASSERT(SrcTraits::pixel_format == p.src->pixelFormat());

    gfx::ClipF area(p.areaF);
    if (!area.clip(p.dst->width(), p.dst->height(), p.sx * p.src->width(), p.sy * p.src->height()))
      return;

    BlenderHelper<DstTraits, SrcTraits> blender(p.dst, p.src, p.pal, p.blendMode);

    gfx::Rect dstBounds(area.dstBounds().x,
                        area.dstBounds().y,
                        int(std::ceil(area.dstBounds().w)),
                        int(std::ceil(area.dstBounds().h)));
    gfx::RectF srcBounds = area.srcBounds();
    const int opacity = p.opacity;

    dstBounds &= p.dst->bounds();

    int dstY = dstBounds.y;
    double srcXStart = srcBounds.x / p.sx;
    double srcXDelta = 1.0 / p.sx;
    int srcWidth = p.src->width();

    for (int y = 0; y < dstBounds.h; ++y, ++dstY) {
      int srcY = int((srcBounds.y + double(y)) / p.sy);
      double srcX = srcXStart;
      int oldSrcX;

      // Out of bounds
      if (srcY >= p.src->height())
        break;

      ASSERT(srcY >= 0 && srcY < p.src->height());
      ASSERT(dstY >= 0 && dstY < p.dst->height());

      auto dstPtr = get_pixel_address_fast<DstTraits>(p.dst, dstBounds.x, dstY);
      auto srcPtr = get_pixel_address_fast<SrcTraits>(p.src, int(srcX), srcY);

#if _DEBUG
      int dstX = dstBounds.x;
#endif

      for (int x = 0; x < dstBounds.w; ++dstPtr) {
        ASSERT(dstX >= 0 && dstX < p.dst->width());
        ASSERT(srcX >= 0 && srcX < p.src->width());

        *dstPtr = blender(*dstPtr, *srcPtr, opacity);
        ++x;

        oldSrcX = int(srcX);
        srcX = srcXStart + srcXDelta * x;
        // Out of bounds
        if (srcX >= srcWidth)
          break;
        srcPtr += int(srcX - oldSrcX);

#if _DEBUG
        ++dstX;
#endif
      }
    }
  }
};

template<class DstTraits, class SrcTraits>
struct composite_image_general_with_tile_flags {
  static void call(const CompositeParams& p)
  {
    ASSERT(p.dst);
    ASSERT(p.src);
    ASSERT(DstTraits::pixel_format == p.dst->pixelFormat());
    ASSERT(SrcTraits::pixel_format == p.src->pixelFormat());

    gfx::ClipF area(p.areaF);
    if (!area.clip(p.dst->width(), p.dst->height(), p.sx * p.src->width(), p.sy * p.src->height()))
      return;

    BlenderHelper<DstTraits, SrcTraits> blender(p.dst, p.src, p.pal, p.blendMode);

    gfx::Rect dstBounds(area.dstBounds().x,
                        area.dstBounds().y,
                        int(std::ceil(area.dstBounds().w)),
                        int(std::ceil(area.dstBounds().h)));
    gfx::Rect srcBounds = area.srcBounds();
    const gfx::Rect srcImgBounds = p.src->bounds();
    const gfx::Size srcMinSize = p.src->size();
    const int opacity = p.opacity;

    dstBounds &= p.dst->bounds();

    if (p.tileFlags & tile_f_xflip) {
      srcBounds.x = p.sx * srcImgBounds.x2() - srcBounds.x2();
    }
    if (p.tileFlags & tile_f_yflip) {
      srcBounds.y = p.sy * srcImgBounds.y2() - srcBounds.y2();
    }

    int dstY = dstBounds.y;

    for (int y = 0; y < dstBounds.h; ++y, ++dstY) {
      ASSERT(dstY >= 0 && dstY < p.dst->height());

      auto dstPtr = get_pixel_address_fast<DstTraits>(p.dst, dstBounds.x, dstY);

#if _DEBUG
      int dstX = dstBounds.x;
#endif

      for (int x = 0; x < dstBounds.w; ++x, ++dstPtr) {
        int srcX;
        int srcY;
        if (p.tileFlags & tile_f_xflip) {
          srcX = (srcBounds.x2() - 1 - x) / p.sx;
        }
        else {
          srcX = (srcBounds.x + x) / p.sx;
        }
        if (p.tileFlags & tile_f_yflip) {
          srcY = (srcBounds.y2() - 1 - y) / p.sy;
        }
        else {
          srcY = (srcBounds.y + y) / p.sy;
        }

        gfx::Size minSize;
        if (p.tileFlags & tile_f_dflip) {
          std::swap(srcX, srcY);
          minSize.w = minSize.h = std::min(srcMinSize.w, srcMinSize.h);
        }
        else {
          minSize = srcMinSize;
        }

        ASSERT(dstX >= 0 && dstX < p.dst->width());

        if (srcX >= 0 && srcX < minSize.w && srcY >= 0 && srcY < minSize.h) {
          auto srcPtr = get_pixel_address_fast<SrcTraits>(p.src, srcX, srcY);
          *dstPtr = blender(*dstPtr, *srcPtr, opacity);
        }
        else {
          *dstPtr = 0;
        }
#if _DEBUG
        ++dstX;
#endif
      }
    }
  }
};

template<template<class, class> class Func>
CompositeImageFunc composite_formats(const PixelFormat dstFormat, const PixelFormat srcFormat)
{
  switch (srcFormat) {
    case IMAGE_RGB:
      switch (dstFormat) {
        case IMAGE_RGB:       return Func<RgbTraits, RgbTraits>::call;
        case IMAGE_GRAYSCALE: return Func<GrayscaleTraits, RgbTraits>::call;
        case IMAGE_INDEXED:   return Func<IndexedTraits, RgbTraits>::call;
      }
      break;

    case IMAGE_GRAYSCALE:
      switch (dstFormat) {
        case IMAGE_RGB:       return Func<RgbTraits, GrayscaleTraits>::call;
        case IMAGE_GRAYSCALE: return Func<GrayscaleTraits, GrayscaleTraits>::call;
        case IMAGE_INDEXED:   return Func<IndexedTraits, GrayscaleTraits>::call;
      }
      break;

    case IMAGE_INDEXED:
      switch (dstFormat) {
        case IMAGE_RGB:       return Func<RgbTraits, IndexedTraits>::call;
        case IMAGE_GRAYSCALE: return Func<GrayscaleTraits, IndexedTraits>::call;
        case IMAGE_INDEXED:   return Func<IndexedTraits, IndexedTraits>::call;
      }
      break;

    case IMAGE_TILEMAP:
      switch (dstFormat) {
        case IMAGE_TILEMAP: return Func<TilemapTraits, TilemapTraits>::call;
      }
      break;
  }
  ASSERT(false && "Invalid pixel formats");
  return nullptr;
}

bool has_visible_reference_layers(const LayerGroup* group)
{
  for (const Layer* child : group->layers()) {
    if (!child->isVisible())
      continue;

    if (child->isReference())
      return true;

    if (child->isGroup() && has_visible_reference_layers(static_cast<const LayerGroup*>(child)))
      return true;
  }
  return false;
}

} // anonymous namespace

Render::Render()
  : m_flags(0)
  , m_nonactiveLayersOpacity(255)
  , m_sprite(nullptr)
  , m_currentLayer(nullptr)
  , m_currentFrame(0)
  , m_extraType(ExtraType::NONE)
  , m_extraCel(nullptr)
  , m_extraImage(nullptr)
  , m_globalOpacity(255)
  , m_selectedLayerForOpacity(nullptr)
  , m_selectedLayer(nullptr)
  , m_selectedFrame(-1)
  , m_previewImage(nullptr)
  , m_previewTileset(nullptr)
  , m_previewBlendMode(BlendMode::NORMAL)
  , m_onionskin(OnionskinType::NONE)
{
}

void Render::setRefLayersVisiblity(const bool visible)
{
  if (visible)
    m_flags |= Flags::ShowRefLayers;
  else
    m_flags &= ~Flags::ShowRefLayers;
}

void Render::setNonactiveLayersOpacity(const int opacity)
{
  m_nonactiveLayersOpacity = opacity;
}

void Render::setComposeGroups(const bool composeGroup)
{
  m_composeGroups = composeGroup;
}

void Render::setProjection(const Projection& projection)
{
  m_proj = projection;
}

void Render::setBgOptions(const BgOptions& bg)
{
  m_bg = bg;
}

void Render::setSelectedLayer(const Layer* layer)
{
  m_selectedLayerForOpacity = layer;
}

void Render::setPreviewImage(const Layer* layer,
                             const frame_t frame,
                             const Image* image,
                             const Tileset* tileset,
                             const gfx::Point& pos,
                             const BlendMode blendMode)
{
  m_selectedLayer = layer;
  m_selectedFrame = frame;
  m_previewImage = image;
  m_previewTileset = tileset;
  m_previewPos = pos;
  m_previewBlendMode = blendMode;
}

void Render::setExtraImage(ExtraType type,
                           const Cel* cel,
                           const Image* image,
                           BlendMode blendMode,
                           const Layer* currentLayer,
                           frame_t currentFrame)
{
  m_extraType = type;
  m_extraCel = cel;
  m_extraImage = image;
  m_extraBlendMode = blendMode;
  m_currentLayer = currentLayer;
  m_currentFrame = currentFrame;
}

void Render::removePreviewImage()
{
  m_previewImage = nullptr;
  m_previewTileset = nullptr;
}

void Render::removeExtraImage()
{
  m_extraType = ExtraType::NONE;
  m_extraCel = nullptr;
}

void Render::setExtraCelInfoMap(const ExtraCelInfoMap* map)
{
  m_extraCelInfoMap = map;
}

void Render::removeExtraCelInfoMap()
{
  m_extraCelInfoMap = nullptr;
}

void Render::setOnionskin(const OnionskinOptions& options)
{
  m_onionskin = options;
}

void Render::disableOnionskin()
{
  m_onionskin.type(OnionskinType::NONE);
}

void Render::renderSprite(Image* dstImage, const Sprite* sprite, frame_t frame)
{
  renderSprite(dstImage, sprite, frame, gfx::ClipF(sprite->bounds()));
}

void Render::renderLayer(Image* dstImage, const Layer* layer, frame_t frame)
{
  renderLayer(dstImage, layer, frame, gfx::Clip(layer->sprite()->bounds()));
}

void Render::renderLayer(Image* dstImage,
                         const Layer* layer,
                         frame_t frame,
                         const gfx::Clip& area,
                         BlendMode blendMode)
{
  m_sprite = layer->sprite();

  CompositeImageFunc compositeImage = getImageComposition(
    (dstImage->pixelFormat() != IMAGE_TILEMAP ? dstImage->pixelFormat() : m_sprite->pixelFormat()),
    m_sprite->pixelFormat(),
    layer);
  if (!compositeImage)
    return;

  m_globalOpacity = 255;

  doc::RenderPlan plan(m_composeGroups);
  plan.addLayer(layer, frame);
  renderPlan(plan, dstImage, area, frame, compositeImage, true, true, blendMode);
}

void Render::renderSprite(Image* dstImage,
                          const Sprite* sprite,
                          frame_t frame,
                          const gfx::ClipF& area)
{
  m_sprite = sprite;

  CompositeImageFunc compositeImage =
    getImageComposition(dstImage->pixelFormat(), m_sprite->pixelFormat(), sprite->root());
  if (!compositeImage)
    return;

  const LayerImage* bgLayer = m_sprite->backgroundLayer();
  color_t bg_color = 0;
  if (m_sprite->pixelFormat() == IMAGE_INDEXED) {
    switch (dstImage->pixelFormat()) {
      case IMAGE_RGB:
      case IMAGE_GRAYSCALE:
        if (bgLayer && bgLayer->isVisible())
          bg_color = m_sprite->palette(frame)->getEntry(m_sprite->transparentColor());
        break;
      case IMAGE_INDEXED: bg_color = m_sprite->transparentColor(); break;
    }
  }

  {
    // Clear dstImage with the bg_color (if the background is not a
    // special background pattern like the checkered background, this
    // is enough as a base color).
    fill_rect(dstImage, area.dstBounds(), bg_color);

    // Draw the Background layer - Onion skin behind the sprite - Transparent Layers
    renderSpriteLayers(dstImage, area, frame, compositeImage);

    // In case that we need a special background (e.g. like the
    // checkered pattern), we can draw the background in a temporal
    // image and then merge this temporal image with the dstImage.
    if (!isSolidBackground(bgLayer, bg_color)) {
      if (!m_tmpBuf)
        m_tmpBuf.reset(new doc::ImageBuffer);
      ImageRef tmpBackground(Image::create(dstImage->spec(), m_tmpBuf));
      renderBackground(tmpBackground.get(), bgLayer, bg_color, area);

      // Draws dstImage over the background on each pixel of dstImage
      // with opacity is < 255 (the result is left on dstImage itself)
      composite_image(dstImage,
                      tmpBackground.get(),
                      sprite->palette(frame),
                      0,
                      0,
                      255,
                      BlendMode::DST_OVER);
    }
  }

  // Draw onion skin in front of the sprite.
  if (m_onionskin.position() == OnionskinPosition::INFRONT)
    renderOnionskin(dstImage, area, frame, compositeImage);

  // Overlay preview image
  if (m_previewImage && m_selectedLayer == nullptr && m_selectedFrame == frame) {
    renderImage(
      dstImage,
      m_previewImage,
      m_sprite->palette(frame),
      gfx::Rect(m_previewPos.x, m_previewPos.y, m_previewImage->width(), m_previewImage->height()),
      area,
      getImageComposition(dstImage->pixelFormat(), m_previewImage->pixelFormat(), sprite->root()),
      255,
      m_previewBlendMode);
  }
}

void Render::renderSpriteLayers(Image* dstImage,
                                const gfx::ClipF& area,
                                frame_t frame,
                                CompositeImageFunc compositeImage)
{
  doc::RenderPlan plan(m_composeGroups);
  plan.addLayer(m_sprite->root(), frame);

  // Draw the background layer.
  m_globalOpacity = 255;
  renderPlan(plan, dstImage, area, frame, compositeImage, true, false, BlendMode::UNSPECIFIED);

  // Draw onion skin behind the sprite.
  if (m_onionskin.position() == OnionskinPosition::BEHIND)
    renderOnionskin(dstImage, area, frame, compositeImage);

  // Draw the transparent layers.
  m_globalOpacity = 255;
  renderPlan(plan, dstImage, area, frame, compositeImage, false, true, BlendMode::UNSPECIFIED);
}

void Render::renderBackground(Image* image,
                              const Layer* bgLayer,
                              const color_t bg_color,
                              const gfx::ClipF& area)
{
  if (isSolidBackground(bgLayer, bg_color)) {
    fill_rect(image, area.dstBounds(), bg_color);
  }
  else {
    switch (m_bg.type) {
      case BgType::CHECKERED:
        renderCheckeredBackground(image, area);
        if (bgLayer && bgLayer->isVisible() &&
            // TODO Review this: bg_color can be an index (not an rgba())
            //      when sprite and dstImage are indexed
            rgba_geta(bg_color) > 0) {
          blend_rect(image,
                     int(area.dst.x),
                     int(area.dst.y),
                     int(area.dst.x + area.size.w - 1),
                     int(area.dst.y + area.size.h - 1),
                     bg_color,
                     255);
        }
        break;
      default:
        ASSERT(false); // Invalid case, needsBackground() should
                       // return false in this case
        break;
    }
  }
}

bool Render::isSolidBackground(const Layer* bgLayer, const color_t bg_color) const
{
  return ((m_bg.type != BgType::CHECKERED) || (bgLayer && bgLayer->isVisible() &&
                                               // TODO Review this: bg_color can be an index (not an
                                               // rgba())
                                               //      when sprite and dstImage are indexed
                                               rgba_geta(bg_color) == 255));
}

void Render::renderOnionskin(Image* dstImage,
                             const gfx::Clip& area,
                             const frame_t frame,
                             const CompositeImageFunc compositeImage)
{
  // Onion-skin feature: Draw previous/next frames with different
  // opacity (<255)
  if (m_onionskin.type() != OnionskinType::NONE) {
    Tag* loop = m_onionskin.loopTag();
    Layer* onionLayer = (m_onionskin.layer() ? m_onionskin.layer() : m_sprite->root());
    Playback play(m_sprite,
                  TagsList(), // TODO add an onionskin option to iterate subtags
                  frame,
                  loop ? Playback::PlayInLoop : Playback::PlayAll,
                  loop);
    frame_t prevFrames = (loop ? m_onionskin.prevFrames() :
                                 std::min(frame, m_onionskin.prevFrames()));
    play.nextFrame(-prevFrames);

    for (frame_t frameOut = frame - prevFrames; frameOut <= frame + m_onionskin.nextFrames();
         ++frameOut, play.nextFrame()) {
      const frame_t frameIn = play.frame();

      if (frameIn == frame || frameIn < 0 || frameIn > m_sprite->lastFrame()) {
        continue;
      }

      if (frameOut < frame) {
        m_globalOpacity = m_onionskin.opacityBase() -
                          m_onionskin.opacityStep() * ((frame - frameOut) - 1);
      }
      else {
        m_globalOpacity = m_onionskin.opacityBase() -
                          m_onionskin.opacityStep() * ((frameOut - frame) - 1);
      }

      m_globalOpacity = std::clamp(m_globalOpacity, 0, 255);
      if (m_globalOpacity > 0) {
        BlendMode blendMode = BlendMode::UNSPECIFIED;
        if (m_onionskin.type() == OnionskinType::MERGE)
          blendMode = BlendMode::NORMAL;
        else if (m_onionskin.type() == OnionskinType::RED_BLUE_TINT)
          blendMode = (frameOut < frame ? BlendMode::RED_TINT : BlendMode::BLUE_TINT);

        doc::RenderPlan plan(m_composeGroups);
        plan.addLayer(onionLayer, frameIn);
        renderPlan(plan,
                   dstImage,
                   area,
                   frameIn,
                   compositeImage,
                   // Render background only for "in-front" onion skinning and
                   // when opacity is < 255
                   (m_globalOpacity < 255 && m_onionskin.position() == OnionskinPosition::INFRONT),
                   true,
                   blendMode);
      }
    }
  }
}

void Render::renderCheckeredBackground(Image* image, const gfx::Clip& area)
{
  int x, y, u, v;
  int tile_w = m_bg.stripeSize.w;
  int tile_h = m_bg.stripeSize.h;

  if (m_bg.zoom) {
    tile_w = m_proj.zoom().apply(tile_w);
    tile_h = m_proj.zoom().apply(tile_h);
  }

  // Tile size
  if (tile_w < 1)
    tile_w = 1;
  if (tile_h < 1)
    tile_h = 1;

  // Tile position (u,v) is the number of tile we start in "area.src" coordinate
  u = (area.src.x / tile_w);
  v = (area.src.y / tile_h);

  // Position where we start drawing the first tile in "image"
  int x_start = -(area.src.x % tile_w);
  int y_start = -(area.src.y % tile_h);

  gfx::Rect dstBounds = area.dstBounds();

  // Fix background colors (make them opaque)
  ASSERT(m_bg.colorPixelFormat == image->pixelFormat());
  switch (m_bg.colorPixelFormat) {
    case IMAGE_RGB:
      m_bg.color1 |= doc::rgba_a_mask;
      m_bg.color2 |= doc::rgba_a_mask;
      break;
    case IMAGE_GRAYSCALE:
      m_bg.color1 |= doc::graya_a_mask;
      m_bg.color2 |= doc::graya_a_mask;
      break;
  }

  // Draw checkered background (tile by tile)
  int u_start = u;
  for (y = y_start - tile_h; y < image->height() + tile_h; y += tile_h) {
    for (x = x_start - tile_w; x < image->width() + tile_w; x += tile_w) {
      gfx::Rect fillRc = dstBounds.createIntersection(gfx::Rect(x, y, tile_w, tile_h));
      if (!fillRc.isEmpty())
        fill_rect(image,
                  fillRc.x,
                  fillRc.y,
                  fillRc.x + fillRc.w - 1,
                  fillRc.y + fillRc.h - 1,
                  (((u + v)) & 1) ? m_bg.color2 : m_bg.color1);
      ++u;
    }
    u = u_start;
    ++v;
  }
}

void Render::renderImage(Image* dst_image,
                         const Image* src_image,
                         const Palette* pal,
                         const int x,
                         const int y,
                         const int opacity,
                         const BlendMode blendMode)
{
  const tile_flags tileFlags = 0;

  CompositeImageFunc compositeImage =
    getImageComposition(dst_image->pixelFormat(), src_image->pixelFormat(), nullptr, tileFlags);
  if (!compositeImage)
    return;

  CompositeParams params;
  params.dst = dst_image;
  params.src = src_image;
  params.pal = pal;
  params.areaF =
    gfx::ClipF(x, y, 0, 0, m_proj.applyX(src_image->width()), m_proj.applyY(src_image->height()));
  params.opacity = opacity;
  params.blendMode = blendMode;
  params.sx = m_proj.scaleX();
  params.sy = m_proj.scaleY();
  params.tileFlags = tileFlags;

  compositeImage(params);
}

void Render::renderPlan(RenderPlan& plan,
                        Image* image,
                        const gfx::Clip& area,
                        const frame_t frame,
                        const CompositeImageFunc compositeImage,
                        const bool render_background,
                        const bool render_transparent,
                        const BlendMode blendMode)
{
  for (const auto& item : plan.items()) {
    const Cel* cel = item.cel;
    const Layer* layer = item.layer;

    ASSERT(layer->isVisible()); // Hidden layers shouldn't be in the plan

    const bool isSelected = (m_selectedLayerForOpacity == layer);
    gfx::Rect extraArea;
    bool drawExtra = false;

    if (m_extraCel && m_extraImage && layer == m_currentLayer &&
        ((layer->isBackground() && render_background) ||
         (!layer->isBackground() && render_transparent)) &&
        // Don't use a tilemap extra cel (IMAGE_TILEMAP) in a
        // non-tilemap layer (in the other hand tilemap layers allow
        // extra cels of any kind). This fixes a crash on renderCel()
        // when we were painting the Preview window using a tilemap
        // extra image to patch a regular layer, when switching from a
        // tilemap layer to a regular layer.
        ((layer->isTilemap()) ||
         (!layer->isTilemap() && m_extraImage->pixelFormat() != IMAGE_TILEMAP))) {
      if (frame == m_extraCel->frame() && frame == m_currentFrame) { // TODO this double check is
                                                                     // not necessary
        drawExtra = true;
      }
      else {
        // Check if we can draw the extra cel when we render a linked
        // frame.
        const Cel* cel2 = layer->cel(m_extraCel->frame());
        if (cel && cel2 && cel->data() == cel2->data()) {
          drawExtra = true;
        }
      }
    }

    if (drawExtra) {
      extraArea = m_extraCel->bounds();
      extraArea = m_proj.apply(extraArea);
      if (m_proj.scaleX() < 1.0)
        extraArea.w--;
      if (m_proj.scaleY() < 1.0)
        extraArea.h--;
      if (extraArea.w < 1)
        extraArea.w = 1;
      if (extraArea.h < 1)
        extraArea.h = 1;
    }

    // Extra cel info from map (for multi-cel transformations)
    const ExtraCelInfo* extraCelInfo = nullptr;

    switch (layer->type()) {
      case ObjectType::LayerImage:
      case ObjectType::LayerTilemap: {
        if ((!render_background && layer->isBackground()) ||
            (!render_transparent && !layer->isBackground()))
          break;

        // Ignore reference layers
        if (!(m_flags & Flags::ShowRefLayers) && layer->isReference())
          break;

        if (!cel)
          cel = layer->cel(frame);

        if (cel) {
          Palette* pal = m_sprite->palette(frame);
          const Image* celImage = nullptr;
          gfx::RectF celBounds;

          // Is the 'm_previewImage' set to be used with this layer?
          if (m_previewImage && checkIfWeShouldUsePreview(cel)) {
            celImage = m_previewImage;
            celBounds = gfx::RectF(m_previewPos.x,
                                   m_previewPos.y,
                                   m_previewImage->width(),
                                   m_previewImage->height());
          }
          // If not, we use the original cel-image from the images' stock
          else {
            celImage = cel->image();
            if (layer->isReference())
              celBounds = cel->boundsF();
            else
              celBounds = cel->bounds();
          }

          if (celImage) {
            BlendMode layerBlendMode = (blendMode == BlendMode::UNSPECIFIED ? layer->blendMode() :
                                                                              blendMode);

            ASSERT(cel->opacity() >= 0);
            ASSERT(cel->opacity() <= 255);
            ASSERT(layer->opacity() >= 0);
            ASSERT(layer->opacity() <= 255);

            // Multiple three opacities: cel*layer*global (*nonactive-layer-opacity)
            int t;
            int opacity = cel->opacity();
            opacity = MUL_UN8(opacity, layer->opacity(), t);
            opacity = MUL_UN8(opacity, m_globalOpacity, t);
            if (!isSelected && m_nonactiveLayersOpacity != 255)
              opacity = MUL_UN8(opacity, m_nonactiveLayersOpacity, t);

            // Check if there's extra cel info for this cel (multi-cel transformations)
            // We use cel->data() as key to support linked cels
            if (m_extraCelInfoMap) {
              auto it = m_extraCelInfoMap->find(cel->data());
              if (it != m_extraCelInfoMap->end())
                extraCelInfo = &it->second;
            }

            // Generally this is just one pass, but if we are using
            // OVER_COMPOSITE extra cel, this will be two passes.
            for (int pass = 0; pass < 2; ++pass) {
              // Draw parts outside the "m_extraCel" area and/or extra cel info area
              const bool hasExtraCelPatch = (drawExtra && m_extraType == ExtraType::PATCH);
              const bool hasExtraCelInfo = (extraCelInfo && extraCelInfo->image);

              if (hasExtraCelPatch || hasExtraCelInfo) {
                gfx::Region originalAreas(area.srcBounds());

                // Exclude the main extra cel area
                if (hasExtraCelPatch)
                  originalAreas.createSubtraction(originalAreas, gfx::Region(extraArea));

                // Exclude the extra cel info area
                if (hasExtraCelInfo)
                  originalAreas.createSubtraction(originalAreas,
                                                  gfx::Region(m_proj.apply(extraCelInfo->bounds)));

                for (auto rc : originalAreas) {
                  renderCel(
                    image,
                    cel,
                    celImage,
                    layer->tileset(),
                    pal,
                    celBounds,
                    gfx::Clip(area.dst.x + rc.x - area.src.x, area.dst.y + rc.y - area.src.y, rc),
                    compositeImage,
                    opacity,
                    layerBlendMode);
                }
              }
              // Draw the whole cel
              else {
                renderCel(image,
                          cel,
                          celImage,
                          layer->tileset(),
                          pal,
                          celBounds,
                          area,
                          compositeImage,
                          opacity,
                          layerBlendMode);
              }

              if (m_extraType == ExtraType::OVER_COMPOSITE && layer == m_currentLayer &&
                  pass == 0) {
                // Go for second pass with the extra blend mode...
                layerBlendMode = m_extraBlendMode;
              }
              else
                break;
            }
          }
        }
        break;
      }

      case ObjectType::LayerGroup: {
        if (!m_composeGroups) {
          ASSERT(false);
          break;
        }

        RenderPlan subPlan(m_composeGroups);

        for (const Layer* child : layer->layers())
          subPlan.addLayer(child, frame);

        // We treat the group layer as a separate image so we can apply modifiers
        // in the whole group while not affecting the layers behind it.
        ImageRef groupImage(Image::createCopy(image));
        groupImage.get()->clear(0);

        // Render the group sublayers
        // We don't apply any blend mode here, as the group layer is a separate image
        // and we want to first calculate the layer blendmodes separately and then merge the images
        renderPlan(subPlan,
                   groupImage.get(),
                   area,
                   frame,
                   compositeImage,
                   render_background,
                   render_transparent,
                   BlendMode::UNSPECIFIED);

        // Get the pallete of the sprite in the current frame
        Palette* pal = m_sprite->palette(frame);

        // Render the group image in the main image, applying the group modifiers
        // The global opacity is not applied here, as it is applied in the LayerImage case.
        composite_image(image, groupImage.get(), pal, 0, 0, layer->opacity(), layer->blendMode());

        break;
      }
    }

    // Draw extras
    if (drawExtra && m_extraType != ExtraType::NONE) {
      if (m_extraCel->opacity() > 0) {
        renderCel(image,
                  m_extraCel,
                  m_sprite,
                  m_extraImage,
                  m_currentLayer ? m_currentLayer->tileset() : nullptr,
                  m_sprite->palette(frame),
                  m_extraCel->bounds(),
                  gfx::Clip(area.dst.x + extraArea.x - area.src.x,
                            area.dst.y + extraArea.y - area.src.y,
                            extraArea),
                  m_extraCel->opacity(),
                  m_extraBlendMode);
      }
    }

    // Draw per-cel extra images (for multi-cel transformations)
    if (extraCelInfo && extraCelInfo->image && extraCelInfo->opacity > 0) {
      const gfx::Rect extraBounds = m_proj.apply(extraCelInfo->bounds);
      renderCel(image,
                cel,
                m_sprite,
                extraCelInfo->image,
                layer ? layer->tileset() : nullptr,
                m_sprite->palette(frame),
                extraCelInfo->bounds,
                gfx::Clip(area.dst.x + extraBounds.x - area.src.x,
                          area.dst.y + extraBounds.y - area.src.y,
                          extraBounds),
                extraCelInfo->opacity,
                extraCelInfo->blendMode);
    }
  }
}

void Render::renderCel(Image* dst_image,
                       const Cel* cel,
                       const Sprite* sprite,
                       const Image* cel_image,
                       const Tileset* tileset,
                       const Palette* pal,
                       const gfx::RectF& celBounds,
                       const gfx::Clip& area,
                       const int opacity,
                       const BlendMode blendMode)
{
  m_sprite = sprite;

  CompositeImageFunc compositeImage =
    getImageComposition(dst_image->pixelFormat(), sprite->pixelFormat(), nullptr);
  if (!compositeImage)
    return;

  renderCel(dst_image,
            cel,
            cel_image,
            tileset,
            pal,
            celBounds,
            area,
            compositeImage,
            opacity,
            blendMode);
}

void Render::renderCel(Image* dst_image,
                       const Cel* cel,
                       const Image* cel_image,
                       const Tileset* tileset,
                       const Palette* pal,
                       const gfx::RectF& celBounds,
                       const gfx::Clip& area,
                       const CompositeImageFunc compositeImage,
                       const int opacity,
                       const BlendMode blendMode)
{
  TRACE_RENDER_CEL(
    "dstImage=(%d %d) celImage=(%d %d) celBounds=(%d %d %d %d) clipArea=(src=%d %d dst=%d %d %d %d)\n",
    dst_image->width(),
    dst_image->height(),
    cel_image->width(),
    cel_image->height(),
    int(celBounds.x),
    int(celBounds.y),
    int(celBounds.w),
    int(celBounds.h),
    area.src.x,
    area.src.y,
    area.dst.x,
    area.dst.y,
    area.size.w,
    area.size.h);

  if (cel_image->pixelFormat() == IMAGE_TILEMAP) {
    ASSERT(tileset);
    if (!tileset || area.size.w < 1 || area.size.h < 1)
      return;

    doc::Grid grid = tileset->grid();
    grid.origin(grid.origin() + gfx::Point(celBounds.origin()));

    // Is the 'm_previewTileset' set to be used with this layer?
    if (m_previewTileset && cel && checkIfWeShouldUsePreview(cel)) {
      tileset = m_previewTileset;
      ASSERT(tileset);
    }
    if (!tileset)
      return;

    gfx::Rect tilesToDraw = grid.canvasToTile(m_proj.remove(gfx::Rect(area.src, area.size)));

    int yPixelsPerTile = m_proj.applyY(grid.tileSize().h);
    if (yPixelsPerTile > 0 && (area.size.h + area.src.y) % yPixelsPerTile > 0)
      tilesToDraw.h += 1;
    int xPixelsPerTile = m_proj.applyX(grid.tileSize().w);
    if (xPixelsPerTile > 0 && (area.size.w + area.src.x) % xPixelsPerTile > 0)
      tilesToDraw.w += 1;

    // As area.size is not empty at this point, we have to draw at
    // least one tile (and the clipping will be performed for the
    // tile pixels later).
    if (tilesToDraw.w < 1)
      tilesToDraw.w = 1;
    if (tilesToDraw.h < 1)
      tilesToDraw.h = 1;

    tilesToDraw &= cel_image->bounds();

    TRACE_RENDER_CEL("Drawing tilemap (%d %d %d %d)\n",
                     tilesToDraw.x,
                     tilesToDraw.y,
                     tilesToDraw.w,
                     tilesToDraw.h);

    for (int v = tilesToDraw.y; v < tilesToDraw.y2(); ++v) {
      for (int u = tilesToDraw.x; u < tilesToDraw.x2(); ++u) {
        auto tileBoundsOnCanvas = grid.tileToCanvas(gfx::Rect(u, v, 1, 1));
        TRACE_RENDER_CEL(" - tile (%d %d) -> (%d %d %d %d)\n",
                         u,
                         v,
                         tileBoundsOnCanvas.x,
                         tileBoundsOnCanvas.y,
                         tileBoundsOnCanvas.w,
                         tileBoundsOnCanvas.h);
        if (!cel_image->bounds().contains(u, v))
          continue;

        const tile_t t = cel_image->getPixel(u, v);
        if (t != doc::notile) {
          const tile_index i = tile_geti(t);

          if (dst_image->pixelFormat() == IMAGE_TILEMAP) {
            put_pixel(dst_image, u - area.dst.x, v - area.dst.y, t);
          }
          else {
            const ImageRef tile_image = tileset->get(i);
            if (!tile_image)
              continue;

            renderImage(dst_image,
                        tile_image.get(),
                        pal,
                        tileBoundsOnCanvas,
                        area,
                        compositeImage,
                        opacity,
                        blendMode,
                        tile_getf(t));
          }
        }
      }
    }
  }
  else {
    renderImage(dst_image, cel_image, pal, celBounds, area, compositeImage, opacity, blendMode);
  }
}

void Render::renderImage(Image* dst_image,
                         const Image* cel_image,
                         const Palette* pal,
                         const gfx::RectF& celBounds,
                         const gfx::Clip& area,
                         CompositeImageFunc compositeImage,
                         const int opacity,
                         const BlendMode blendMode,
                         const tile_flags tileFlags)
{
  gfx::RectF scaledBounds = m_proj.apply(celBounds);
  gfx::RectF srcBounds = gfx::RectF(area.srcBounds()).createIntersection(scaledBounds);
  if (srcBounds.isEmpty())
    return;

  // Get the function to composite the tile with the given flip flags
  if (tileFlags) {
    compositeImage =
      getImageComposition(dst_image->pixelFormat(), cel_image->pixelFormat(), nullptr, tileFlags);
  }

  CompositeParams params;
  params.dst = dst_image;
  params.src = cel_image;
  params.pal = pal;
  params.areaF = gfx::ClipF(double(area.dst.x) + srcBounds.x - double(area.src.x),
                            double(area.dst.y) + srcBounds.y - double(area.src.y),
                            srcBounds.x - scaledBounds.x,
                            srcBounds.y - scaledBounds.y,
                            srcBounds.w,
                            srcBounds.h);
  params.opacity = opacity;
  params.blendMode = blendMode;
  params.sx = m_proj.scaleX() * celBounds.w / double(cel_image->width());
  params.sy = m_proj.scaleY() * celBounds.h / double(cel_image->height());
  params.tileFlags = tileFlags;

  compositeImage(params);
}

CompositeImageFunc Render::getImageComposition(const PixelFormat dstFormat,
                                               const PixelFormat srcFormat,
                                               const Layer* layer,
                                               const tile_flags tileFlags)
{
  // True if we need blending pixel by pixel. If this is false we can
  // blend src+dst one time and repeat the resulting color in dst
  // image n-times (where n is the zoom scale).
  double intpart;
  const bool finegrain =
    (!m_bg.zoom && (m_bg.stripeSize.w < m_proj.applyX(1) || m_bg.stripeSize.h < m_proj.applyY(1) ||
                    std::modf(double(m_bg.stripeSize.w) / m_proj.applyX(1.0), &intpart) != 0.0 ||
                    std::modf(double(m_bg.stripeSize.h) / m_proj.applyY(1.0), &intpart) != 0.0)) ||
    (layer && layer->isGroup() &&
     has_visible_reference_layers(static_cast<const LayerGroup*>(layer)));

  if (tileFlags)
    return composite_formats<composite_image_general_with_tile_flags>(dstFormat, srcFormat);

  if (finegrain || !m_proj.zoom().isSimpleZoomLevel())
    return composite_formats<composite_image_general>(dstFormat, srcFormat);

  if (m_proj.scaleX() == 1.0 && m_proj.scaleY() == 1.0)
    return composite_formats<composite_image_without_scale>(dstFormat, srcFormat);

  if (m_proj.isSimpleScaleUpCase())
    return composite_formats<composite_image_scale_up>(dstFormat, srcFormat);

  if (m_proj.isSimpleScaleDownCase())
    return composite_formats<composite_image_scale_down>(dstFormat, srcFormat);

  // Slower composite function for remaining cases not considered
  return composite_formats<composite_image_general>(dstFormat, srcFormat);
}

bool Render::checkIfWeShouldUsePreview(const Cel* cel) const
{
  if ((m_selectedLayer == cel->layer())) {
    if (m_selectedFrame == cel->frame()) {
      return true;
    }
    else if (cel->layer()) {
      // This preview might be useful if we are rendering a linked
      // frame to preview.
      Cel* cel2 = cel->layer()->cel(m_selectedFrame);
      if (cel2 && cel2->data() == cel->data())
        return true;
    }
  }
  return false;
}

void composite_image(Image* dst,
                     const Image* src,
                     const Palette* pal,
                     const int x,
                     const int y,
                     const int opacity,
                     const BlendMode blendMode)
{
  // As the background is not rendered in renderImage(), we don't need
  // to configure the Render instance's BgType.
  Render().renderImage(dst, src, pal, x, y, opacity, blendMode);
}

} // namespace render
