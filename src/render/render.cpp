// Aseprite Render Library
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "render/render.h"

#include "base/clamp.h"
#include "doc/blend_internals.h"
#include "doc/blend_mode.h"
#include "doc/doc.h"
#include "doc/handle_anidir.h"
#include "doc/image_impl.h"
#include "gfx/clip.h"
#include "gfx/region.h"

#include <cmath>

namespace render {

namespace {

//////////////////////////////////////////////////////////////////////
// Scaled composite

template<class DstTraits, class SrcTraits>
class BlenderHelper {
  BlendFunc m_blendFunc;
  color_t m_mask_color;
public:
  BlenderHelper(const Image* src, const Palette* pal, BlendMode blendMode, const bool newBlend)
  {
    m_blendFunc = SrcTraits::get_blender(blendMode, newBlend);
    m_mask_color = src->maskColor();
  }
  inline typename DstTraits::pixel_t
  operator()(const typename DstTraits::pixel_t& dst,
             const typename SrcTraits::pixel_t& src,
             const int opacity)
  {
    if (src != m_mask_color)
      return (*m_blendFunc)(dst, src, opacity);
    else
      return dst;
  }
};

template<>
class BlenderHelper<RgbTraits, GrayscaleTraits> {
  BlendFunc m_blendFunc;
  color_t m_mask_color;
public:
  BlenderHelper(const Image* src, const Palette* pal, BlendMode blendMode, const bool newBlend)
  {
    m_blendFunc = RgbTraits::get_blender(blendMode, newBlend);
    m_mask_color = src->maskColor();
  }
  inline RgbTraits::pixel_t
  operator()(const RgbTraits::pixel_t& dst,
             const GrayscaleTraits::pixel_t& src,
             const int opacity)
  {
    if (src != m_mask_color) {
      int v = graya_getv(src);
      return (*m_blendFunc)(dst, rgba(v, v, v, graya_geta(src)), opacity);
    }
    else
      return dst;
  }
};

template<>
class BlenderHelper<RgbTraits, IndexedTraits> {
  const Palette* m_pal;
  BlendMode m_blendMode;
  BlendFunc m_blendFunc;
  color_t m_mask_color;
public:
  BlenderHelper(const Image* src, const Palette* pal, BlendMode blendMode, const bool newBlend)
  {
    m_blendMode = blendMode;
    m_blendFunc = RgbTraits::get_blender(blendMode, newBlend);
    m_mask_color = src->maskColor();
    m_pal = pal;
  }
  inline RgbTraits::pixel_t
  operator()(const RgbTraits::pixel_t& dst,
             const IndexedTraits::pixel_t& src,
             const int opacity)
  {
    if (m_blendMode == BlendMode::SRC) {
      return m_pal->getEntry(src);
    }
    else {
      if (src != m_mask_color) {
        return (*m_blendFunc)(dst, m_pal->getEntry(src), opacity);
      }
      else
        return dst;
    }
  }
};

template<>
class BlenderHelper<IndexedTraits, IndexedTraits> {
  BlendMode m_blendMode;
  color_t m_mask_color;
public:
  BlenderHelper(const Image* src, const Palette* pal, BlendMode blendMode, const bool newBlend)
  {
    m_blendMode = blendMode;
    m_mask_color = src->maskColor();
  }
  inline IndexedTraits::pixel_t
  operator()(const IndexedTraits::pixel_t& dst,
             const IndexedTraits::pixel_t& src,
             const int opacity)
  {
    if (m_blendMode == BlendMode::SRC) {
      return src;
    }
    else if (m_blendMode == BlendMode::DST_OVER) {
      if (dst != m_mask_color)
        return dst;
      else
        return src;
    }
    else {
      if (src != m_mask_color)
        return src;
      else
        return dst;
    }
  }
};

template<class DstTraits, class SrcTraits>
void composite_image_without_scale(
  Image* dst, const Image* src, const Palette* pal,
  const gfx::ClipF& areaF,
  const int opacity,
  const BlendMode blendMode,
  const double sx,
  const double sy,
  const bool newBlend)
{
  ASSERT(dst);
  ASSERT(src);
  ASSERT(DstTraits::pixel_format == dst->pixelFormat());
  ASSERT(SrcTraits::pixel_format == src->pixelFormat());

  BlenderHelper<DstTraits, SrcTraits> blender(src, pal, blendMode, newBlend);

  gfx::Clip area(areaF);
  if (!area.clip(dst->width(), dst->height(),
                 src->width(), src->height()))
    return;

  gfx::Rect srcBounds = area.srcBounds();
  gfx::Rect dstBounds = area.dstBounds();
  int bottom = dstBounds.y2()-1;

  ASSERT(!srcBounds.isEmpty());

  // Lock all necessary bits
  const LockImageBits<SrcTraits> srcBits(src, srcBounds);
  LockImageBits<DstTraits> dstBits(dst, dstBounds);
  typename LockImageBits<SrcTraits>::const_iterator src_it = srcBits.begin();
#ifdef _DEBUG
  typename LockImageBits<SrcTraits>::const_iterator src_end = srcBits.end();
#endif
  typename LockImageBits<DstTraits>::iterator dst_it, dst_end;

  // For each line to draw of the source image...
  dstBounds.h = 1;
  for (int y=0; y<srcBounds.h; ++y) {
    dst_it = dstBits.begin_area(dstBounds);
    dst_end = dstBits.end_area(dstBounds);

    for (int x=0; x<srcBounds.w; ++x) {
      ASSERT(src_it >= srcBits.begin() && src_it < src_end);
      ASSERT(dst_it >= dstBits.begin() && dst_it < dst_end);
      *dst_it = blender(*dst_it, *src_it, opacity);
      ++src_it;
      ++dst_it;
    }

    if (++dstBounds.y > bottom)
      break;
  }
}

template<class DstTraits, class SrcTraits>
void composite_image_scale_up(
  Image* dst, const Image* src, const Palette* pal,
  const gfx::ClipF& areaF,
  const int opacity,
  const BlendMode blendMode,
  const double sx,
  const double sy,
  const bool newBlend)
{
  ASSERT(dst);
  ASSERT(src);
  ASSERT(DstTraits::pixel_format == dst->pixelFormat());
  ASSERT(SrcTraits::pixel_format == src->pixelFormat());

  gfx::Clip area(areaF);
  if (!area.clip(dst->width(), dst->height(),
                 int(sx*double(src->width())),
                 int(sy*double(src->height()))))
    return;

  BlenderHelper<DstTraits, SrcTraits> blender(src, pal, blendMode, newBlend);
  int px_x, px_y;
  int px_w = int(sx);
  int px_h = int(sy);
  int first_px_w = px_w - (area.src.x % px_w);
  int first_px_h = px_h - (area.src.y % px_h);

  gfx::Rect srcBounds = area.srcBounds();
  srcBounds.w = (srcBounds.x+srcBounds.w)/px_w - srcBounds.x/px_w;
  srcBounds.h = (srcBounds.y+srcBounds.h)/px_h - srcBounds.y/px_h;
  srcBounds.x /= px_w;
  srcBounds.y /= px_h;
  if ((area.src.x+area.size.w) % px_w > 0) ++srcBounds.w;
  if ((area.src.y+area.size.h) % px_h > 0) ++srcBounds.h;
  if (srcBounds.isEmpty())
    return;

  gfx::Rect dstBounds = area.dstBounds();
  int bottom = dstBounds.y2()-1;
  int line_h;

  // the scanline variable is used to blend src/dst pixels one time for each pixel
  typedef std::vector<typename DstTraits::pixel_t> Scanline;
  Scanline scanline(srcBounds.w);
  typename Scanline::iterator scanline_it;
#ifdef _DEBUG
  typename Scanline::iterator scanline_end = scanline.end();
#endif

  // Lock all necessary bits
  const LockImageBits<SrcTraits> srcBits(src, srcBounds);
  LockImageBits<DstTraits> dstBits(dst, dstBounds);
  typename LockImageBits<SrcTraits>::const_iterator src_it = srcBits.begin();
#ifdef _DEBUG
  typename LockImageBits<SrcTraits>::const_iterator src_end = srcBits.end();
#endif
  typename LockImageBits<DstTraits>::iterator dst_it, dst_end;

  // For each line to draw of the source image...
  dstBounds.h = 1;
  for (int y=0; y<srcBounds.h; ++y) {
    dst_it = dstBits.begin_area(dstBounds);
    dst_end = dstBits.end_area(dstBounds);

    // Read 'src' and 'dst' and blend them, put the result in `scanline'
    scanline_it = scanline.begin();
    for (int x=0; x<srcBounds.w; ++x) {
      ASSERT(src_it >= srcBits.begin() && src_it < src_end);
      ASSERT(dst_it >= dstBits.begin() && dst_it < dst_end);
      ASSERT(scanline_it >= scanline.begin() && scanline_it < scanline_end);

      *scanline_it = blender(*dst_it, *src_it, opacity);
      ++src_it;

      int delta;
      if (x == 0)
        delta = first_px_w;
      else
        delta = px_w;

      while (dst_it != dst_end && delta-- > 0)
        ++dst_it;

      ++scanline_it;
    }

    // Get the 'height' of the line to be painted in 'dst'
    if ((y == 0) && (first_px_h > 0))
      line_h = first_px_h;
    else
      line_h = px_h;

    // Draw the line in 'dst'
    for (px_y=0; px_y<line_h; ++px_y) {
      dst_it = dstBits.begin_area(dstBounds);
      dst_end = dstBits.end_area(dstBounds);
      scanline_it = scanline.begin();

      int x = 0;

      // first pixel
      for (px_x=0; px_x<first_px_w; ++px_x) {
        ASSERT(scanline_it != scanline_end);
        ASSERT(dst_it != dst_end);

        *dst_it = *scanline_it;

        ++dst_it;
        if (dst_it == dst_end)
          goto done_with_line;
      }

      ++scanline_it;
      ++x;

      // the rest of the line
      for (; x<srcBounds.w; ++x) {
        for (px_x=0; px_x<px_w; ++px_x) {
          ASSERT(dst_it !=  dst_end);

          *dst_it = *scanline_it;

          ++dst_it;
          if (dst_it == dst_end)
            goto done_with_line;
        }

        ++scanline_it;
      }

done_with_line:;
      if (++dstBounds.y > bottom)
        goto done_with_blit;
    }
  }

done_with_blit:;
}

template<class DstTraits, class SrcTraits>
void composite_image_scale_down(
  Image* dst, const Image* src, const Palette* pal,
  const gfx::ClipF& areaF,
  const int opacity,
  const BlendMode blendMode,
  const double sx,
  const double sy,
  const bool newBlend)
{
  ASSERT(dst);
  ASSERT(src);
  ASSERT(DstTraits::pixel_format == dst->pixelFormat());
  ASSERT(SrcTraits::pixel_format == src->pixelFormat());

  gfx::Clip area(areaF);
  if (!area.clip(dst->width(), dst->height(),
                 int(sx*double(src->width())),
                 int(sy*double(src->height()))))
    return;

  BlenderHelper<DstTraits, SrcTraits> blender(src, pal, blendMode, newBlend);
  int step_w = int(1.0 / sx);
  int step_h = int(1.0 / sy);
  if (step_w < 1 || step_h < 1)
    return;

  gfx::Rect srcBounds = area.srcBounds();
  srcBounds.w = (srcBounds.x+srcBounds.w)*step_w - srcBounds.x*step_w;
  srcBounds.h = (srcBounds.y+srcBounds.h)*step_h - srcBounds.y*step_h;
  srcBounds.x *= step_w;
  srcBounds.y *= step_h;
  if (srcBounds.isEmpty())
    return;

  gfx::Rect dstBounds = area.dstBounds();

  // Lock all necessary bits
  const LockImageBits<SrcTraits> srcBits(src, srcBounds);
  LockImageBits<DstTraits> dstBits(dst, dstBounds);
  auto src_it = srcBits.begin();
  auto dst_it = dstBits.begin();
#ifdef _DEBUG
  auto src_end = srcBits.end();
  auto dst_end = dstBits.end();
#endif

  // Adjust to src_it for each line
  int adjust_per_line = (dstBounds.w*step_w)*(step_h-1);

  // For each line to draw of the source image...
  for (int y=0; y<dstBounds.h; ++y) {
    for (int x=0; x<dstBounds.w; ++x) {
      ASSERT(src_it >= srcBits.begin() && src_it < src_end);
      ASSERT(dst_it >= dstBits.begin() && dst_it < dst_end);

      *dst_it = blender(*dst_it, *src_it, opacity);

      // Skip columns
      src_it += step_w;
      ++dst_it;
    }

    // Skip rows
    src_it += adjust_per_line;
  }
}

template<class DstTraits, class SrcTraits>
void composite_image_general(
  Image* dst, const Image* src, const Palette* pal,
  const gfx::ClipF& areaF,
  const int opacity,
  const BlendMode blendMode,
  const double sx,
  const double sy,
  const bool newBlend)
{
  ASSERT(dst);
  ASSERT(src);
  ASSERT(DstTraits::pixel_format == dst->pixelFormat());
  ASSERT(SrcTraits::pixel_format == src->pixelFormat());

  gfx::ClipF area(areaF);
  if (!area.clip(dst->width(), dst->height(),
                 sx*src->width(), sy*src->height()))
    return;

  BlenderHelper<DstTraits, SrcTraits> blender(src, pal, blendMode, newBlend);

  gfx::Rect dstBounds(
    area.dstBounds().x, area.dstBounds().y,
    int(std::ceil(area.dstBounds().w)),
    int(std::ceil(area.dstBounds().h)));
  gfx::RectF srcBounds = area.srcBounds();

  dstBounds &= dst->bounds();

  int dstY = dstBounds.y;
  double srcXStart = srcBounds.x / sx;
  double srcXDelta = 1.0 / sx;
  int srcWidth = src->width();
  for (int y=0; y<dstBounds.h; ++y, ++dstY) {
    int srcY = int((srcBounds.y+double(y)) / sy);
    double srcX = srcXStart;
    int oldSrcX;

    // Out of bounds
    if (srcY >= src->height())
      break;

    ASSERT(srcY >= 0 && srcY < src->height());
    ASSERT(dstY >= 0 && dstY < dst->height());

    auto dstPtr = get_pixel_address_fast<DstTraits>(dst, dstBounds.x, dstY);
    auto srcPtr = get_pixel_address_fast<SrcTraits>(src, int(srcX), srcY);

#if _DEBUG
    int dstX = dstBounds.x;
#endif

    for (int x=0; x<dstBounds.w; ++dstPtr) {
      ASSERT(dstX >= 0 && dstX < dst->width());
      ASSERT(srcX >= 0 && srcX < src->width());

      *dstPtr = blender(*dstPtr, *srcPtr, opacity);
      ++x;

      oldSrcX = int(srcX);
      srcX = srcXStart + srcXDelta*x;
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

template<class DstTraits, class SrcTraits>
CompositeImageFunc get_fastest_composition_path(const Projection& proj,
                                                const bool finegrain)
{
  if (finegrain || !proj.zoom().isSimpleZoomLevel()) {
    return composite_image_general<DstTraits, SrcTraits>;
  }
  else if (proj.applyX(1) == 1 && proj.applyY(1) == 1) {
    return composite_image_without_scale<DstTraits, SrcTraits>;
  }
  else if (proj.scaleX() >= 1.0 && proj.scaleY() >= 1.0) {
    return composite_image_scale_up<DstTraits, SrcTraits>;
  }
  // Slower composite function for special cases with odd zoom and non-square pixel ratio
  else if (((proj.removeX(1) > 1) && (proj.removeX(1) & 1)) ||
           ((proj.removeY(1) > 1) && (proj.removeY(1) & 1))) {
    return composite_image_general<DstTraits, SrcTraits>;
  }
  else {
    return composite_image_scale_down<DstTraits, SrcTraits>;
  }
}

bool has_visible_reference_layers(const LayerGroup* group)
{
  for (const Layer* child : group->layers()) {
    if (!child->isVisible())
      continue;

    if (child->isReference())
      return true;

    if (child->isGroup() &&
        has_visible_reference_layers(static_cast<const LayerGroup*>(child)))
      return true;
  }
  return false;
}

} // anonymous namespace

Render::Render()
  : m_flags(0)
  , m_nonactiveLayersOpacity(255)
  , m_sprite(nullptr)
  , m_currentLayer(NULL)
  , m_currentFrame(0)
  , m_extraType(ExtraType::NONE)
  , m_extraCel(NULL)
  , m_extraImage(NULL)
  , m_newBlendMethod(true)
  , m_bgType(BgType::TRANSPARENT)
  , m_bgCheckedSize(16, 16)
  , m_globalOpacity(255)
  , m_selectedLayerForOpacity(nullptr)
  , m_selectedLayer(nullptr)
  , m_selectedFrame(-1)
  , m_previewImage(nullptr)
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

void Render::setNewBlend(const bool newBlend)
{
  m_newBlendMethod = newBlend;
}

void Render::setProjection(const Projection& projection)
{
  m_proj = projection;
}

void Render::setBgType(BgType type)
{
  m_bgType = type;
}

void Render::setBgZoom(bool state)
{
  m_bgZoom = state;
}

void Render::setBgColor1(color_t color)
{
  m_bgColor1 = color;
}

void Render::setBgColor2(color_t color)
{
  m_bgColor2 = color;
}

void Render::setBgCheckedSize(const gfx::Size& size)
{
  m_bgCheckedSize = size;
}

void Render::setSelectedLayer(const Layer* layer)
{
  m_selectedLayerForOpacity = layer;
}

void Render::setPreviewImage(const Layer* layer,
                             const frame_t frame,
                             const Image* image,
                             const gfx::Point& pos,
                             const BlendMode blendMode)
{
  m_selectedLayer = layer;
  m_selectedFrame = frame;
  m_previewImage = image;
  m_previewPos = pos;
  m_previewBlendMode = blendMode;
}

void Render::setExtraImage(
  ExtraType type,
  const Cel* cel, const Image* image, BlendMode blendMode,
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
}

void Render::removeExtraImage()
{
  m_extraType = ExtraType::NONE;
  m_extraCel = NULL;
}

void Render::setOnionskin(const OnionskinOptions& options)
{
  m_onionskin = options;
}

void Render::disableOnionskin()
{
  m_onionskin.type(OnionskinType::NONE);
}

void Render::renderSprite(
  Image* dstImage,
  const Sprite* sprite,
  frame_t frame)
{
  renderSprite(
    dstImage, sprite, frame,
    gfx::ClipF(sprite->bounds()));
}

void Render::renderLayer(
  Image* dstImage,
  const Layer* layer,
  frame_t frame)
{
  renderLayer(dstImage, layer, frame,
    gfx::Clip(layer->sprite()->bounds()));
}

void Render::renderLayer(
  Image* dstImage,
  const Layer* layer,
  frame_t frame,
  const gfx::Clip& area,
  BlendMode blendMode)
{
  m_sprite = layer->sprite();

  CompositeImageFunc compositeImage =
    getImageComposition(
      dstImage->pixelFormat(),
      m_sprite->pixelFormat(), layer);
  if (!compositeImage)
    return;

  m_globalOpacity = 255;
  renderLayer(
    layer, dstImage, area,
    frame, compositeImage,
    true, true, blendMode, false);
}

void Render::renderSprite(
  Image* dstImage,
  const Sprite* sprite,
  frame_t frame,
  const gfx::ClipF& area)
{
  m_sprite = sprite;

  CompositeImageFunc compositeImage =
    getImageComposition(
      dstImage->pixelFormat(),
      m_sprite->pixelFormat(), sprite->root());
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
      case IMAGE_INDEXED:
        bg_color = m_sprite->transparentColor();
        break;
    }
  }

  // New Blending Method:
  if (m_newBlendMethod) {
    // Clear dstImage with the bg_color (if the background is not a
    // special background pattern like the checked background, this is
    // enough as a base color).
    fill_rect(dstImage, area.dstBounds(), bg_color);

    // Draw the Background layer - Onion skin behind the sprite - Transparent Layers
    renderSpriteLayers(dstImage, area, frame, compositeImage);

    // In case that we need a special background (e.g. like the
    // checked pattern), we can draw the background in a temporal
    // image and then merge this temporal image with the dstImage.
    if (!isSolidBackground(bgLayer, bg_color)) {
      if (!m_tmpBuf)
        m_tmpBuf.reset(new doc::ImageBuffer);
      ImageRef tmpBackground(Image::create(dstImage->spec(), m_tmpBuf));
      renderBackground(tmpBackground.get(), bgLayer, bg_color, area);

      // Draws dstImage over the background on each pixel of dstImage
      // with opacity is < 255 (the result is left on dstImage itself)
      composite_image(dstImage, tmpBackground.get(), sprite->palette(frame),
                      0, 0, 255, BlendMode::DST_OVER);
    }
  }
  // Old Blending Method:
  else {
    renderBackground(dstImage, bgLayer, bg_color, area);
    renderSpriteLayers(dstImage, area, frame, compositeImage);
  }

  // Draw onion skin in front of the sprite.
  if (m_onionskin.position() == OnionskinPosition::INFRONT)
    renderOnionskin(dstImage, area, frame, compositeImage);

  // Overlay preview image
  if (m_previewImage &&
      m_selectedLayer == nullptr &&
      m_selectedFrame == frame) {
    renderImage(
      dstImage,
      m_previewImage,
      m_sprite->palette(frame),
      gfx::Rect(m_previewPos.x, m_previewPos.y,
                m_previewImage->width(),
                m_previewImage->height()),
      area,
      getImageComposition(
        dstImage->pixelFormat(),
        m_previewImage->pixelFormat(), sprite->root()),
      255,
      m_previewBlendMode);
  }
}

void Render::renderSpriteLayers(Image* dstImage,
                              const gfx::ClipF& area,
                              frame_t frame,
                              CompositeImageFunc compositeImage)
{
  // Draw the background layer.
  m_globalOpacity = 255;
  renderLayer(m_sprite->root(), dstImage,
              area, frame, compositeImage,
              true,
              false,
              BlendMode::UNSPECIFIED,
              false);

  // Draw onion skin behind the sprite.
  if (m_onionskin.position() == OnionskinPosition::BEHIND)
    renderOnionskin(dstImage, area, frame, compositeImage);

  // Draw the transparent layers.
  m_globalOpacity = 255;
  renderLayer(m_sprite->root(), dstImage,
              area, frame, compositeImage,
              false,
              true,
              BlendMode::UNSPECIFIED, false);
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
    switch (m_bgType) {
      case BgType::CHECKED:
        renderCheckedBackground(image, area);
        if (bgLayer && bgLayer->isVisible() &&
            // TODO Review this: bg_color can be an index (not an rgba())
            //      when sprite and dstImage are indexed
            rgba_geta(bg_color) > 0) {
          blend_rect(image,
                     int(area.dst.x),
                     int(area.dst.y),
                     int(area.dst.x+area.size.w-1),
                     int(area.dst.y+area.size.h-1),
                     bg_color, 255);
        }
        break;
      default:
        ASSERT(false); // Invalid case, needsBackground() should
                       // return false in this case
        break;
    }
  }
}

bool Render::isSolidBackground(
  const Layer* bgLayer,
  const color_t bg_color) const
{
  return
    ((m_bgType != BgType::CHECKED) ||
     (bgLayer && bgLayer->isVisible() &&
      // TODO Review this: bg_color can be an index (not an rgba())
      //      when sprite and dstImage are indexed
      rgba_geta(bg_color) == 255));
}

void Render::renderOnionskin(
  Image* dstImage,
  const gfx::Clip& area,
  const frame_t frame,
  const CompositeImageFunc compositeImage)
{
  // Onion-skin feature: Draw previous/next frames with different
  // opacity (<255)
  if (m_onionskin.type() != OnionskinType::NONE) {
    Tag* loop = m_onionskin.loopTag();
    Layer* onionLayer = (m_onionskin.layer() ? m_onionskin.layer():
                                               m_sprite->root());
    frame_t frameIn;

    for (frame_t frameOut = frame - m_onionskin.prevFrames();
         frameOut <= frame + m_onionskin.nextFrames();
         ++frameOut) {
      if (loop) {
        bool pingPongForward = true;
        frameIn =
          calculate_next_frame(m_sprite,
                               frame, frameOut - frame,
                               loop, pingPongForward);
      }
      else {
        frameIn = frameOut;
      }

      if (frameIn == frame ||
          frameIn < 0 ||
          frameIn > m_sprite->lastFrame()) {
        continue;
      }

      if (frameOut < frame) {
        m_globalOpacity = m_onionskin.opacityBase() - m_onionskin.opacityStep() * ((frame - frameOut)-1);
      }
      else {
        m_globalOpacity = m_onionskin.opacityBase() - m_onionskin.opacityStep() * ((frameOut - frame)-1);
      }

      m_globalOpacity = base::clamp(m_globalOpacity, 0, 255);
      if (m_globalOpacity > 0) {
        BlendMode blendMode = BlendMode::UNSPECIFIED;
        if (m_onionskin.type() == OnionskinType::MERGE)
          blendMode = BlendMode::NORMAL;
        else if (m_onionskin.type() == OnionskinType::RED_BLUE_TINT)
          blendMode = (frameOut < frame ? BlendMode::RED_TINT: BlendMode::BLUE_TINT);

        renderLayer(
          onionLayer, dstImage,
          area, frameIn, compositeImage,
          // Render background only for "in-front" onion skinning and
          // when opacity is < 255
          (m_globalOpacity < 255 &&
           m_onionskin.position() == OnionskinPosition::INFRONT),
          true, blendMode, false);
      }
    }
  }
}

void Render::renderCheckedBackground(
  Image* image,
  const gfx::Clip& area)
{
  int x, y, u, v;
  int tile_w = m_bgCheckedSize.w;
  int tile_h = m_bgCheckedSize.h;

  if (m_bgZoom) {
    tile_w = m_proj.zoom().apply(tile_w);
    tile_h = m_proj.zoom().apply(tile_h);
  }

  // Tile size
  if (tile_w < 1) tile_w = 1;
  if (tile_h < 1) tile_h = 1;

  // Tile position (u,v) is the number of tile we start in "area.src" coordinate
  u = (area.src.x / tile_w);
  v = (area.src.y / tile_h);

  // Position where we start drawing the first tile in "image"
  int x_start = -(area.src.x % tile_w);
  int y_start = -(area.src.y % tile_h);

  gfx::Rect dstBounds = area.dstBounds();

  // Fix background color (make them opaque)
  switch (image->pixelFormat()) {
    case IMAGE_RGB:
      m_bgColor1 |= doc::rgba_a_mask;
      m_bgColor2 |= doc::rgba_a_mask;
      break;
    case IMAGE_GRAYSCALE:
      m_bgColor1 |= doc::graya_a_mask;
      m_bgColor2 |= doc::graya_a_mask;
      break;
  }

  // Draw checked background (tile by tile)
  int u_start = u;
  for (y=y_start-tile_h; y<image->height()+tile_h; y+=tile_h) {
    for (x=x_start-tile_w; x<image->width()+tile_w; x+=tile_w) {
      gfx::Rect fillRc = dstBounds.createIntersection(gfx::Rect(x, y, tile_w, tile_h));
      if (!fillRc.isEmpty())
        fill_rect(
          image, fillRc.x, fillRc.y, fillRc.x+fillRc.w-1, fillRc.y+fillRc.h-1,
          (((u+v))&1)? m_bgColor2: m_bgColor1);
      ++u;
    }
    u = u_start;
    ++v;
  }
}

void Render::renderImage(
  Image* dst_image,
  const Image* src_image,
  const Palette* pal,
  const int x,
  const int y,
  const int opacity,
  const BlendMode blendMode)
{
  CompositeImageFunc compositeImage =
    getImageComposition(
      dst_image->pixelFormat(),
      src_image->pixelFormat(), nullptr);
  if (!compositeImage)
    return;

  compositeImage(
    dst_image, src_image, pal,
    gfx::ClipF(x, y, 0, 0,
               m_proj.applyX(src_image->width()),
               m_proj.applyY(src_image->height())),
    opacity, blendMode,
    m_proj.scaleX(),
    m_proj.scaleY(),
    m_newBlendMethod);
}

void Render::renderLayer(
  const Layer* layer,
  Image* image,
  const gfx::Clip& area,
  const frame_t frame,
  const CompositeImageFunc compositeImage,
  const bool render_background,
  const bool render_transparent,
  const BlendMode blendMode,
  bool isSelected)
{
  // we can't read from this layer
  if (!layer->isVisible())
    return;

  if (m_selectedLayerForOpacity == layer)
    isSelected = true;

  const Cel* cel = nullptr;
  gfx::Rect extraArea;
  bool drawExtra = false;

  if (m_extraCel &&
      m_extraImage &&
      layer == m_currentLayer &&
      ((layer->isBackground() && render_background) ||
       (!layer->isBackground() && render_transparent))) {
    if (frame == m_extraCel->frame() &&
        frame == m_currentFrame) { // TODO this double check is not necessary
      drawExtra = true;
    }
    else {
      // Check if we can draw the extra cel when we render a linked
      // frame.
      cel = layer->cel(frame);
      Cel* cel2 = layer->cel(m_extraCel->frame());
      if (cel && cel2 &&
          cel->data() == cel2->data()) {
        drawExtra = true;
      }
    }
  }

  if (drawExtra) {
    extraArea = gfx::Rect(
      m_extraCel->x(),
      m_extraCel->y(),
      m_extraImage->width(),
      m_extraImage->height());
    extraArea = m_proj.apply(extraArea);
    if (m_proj.scaleX() < 1.0) extraArea.w--;
    if (m_proj.scaleY() < 1.0) extraArea.h--;
    if (extraArea.w < 1) extraArea.w = 1;
    if (extraArea.h < 1) extraArea.h = 1;
  }

  switch (layer->type()) {

    case ObjectType::LayerImage: {
      if ((!render_background  &&  layer->isBackground()) ||
          (!render_transparent && !layer->isBackground()))
        break;

      // Ignore reference layers
      if (!(m_flags & Flags::ShowRefLayers) &&
          layer->isReference())
        break;

      if (!cel)
        cel = layer->cel(frame);

      if (cel) {
        Palette* pal = m_sprite->palette(frame);
        const Image* celImage = nullptr;
        gfx::RectF celBounds;

        // Is the 'm_previewImage' set to be used with this layer?
        bool usePreview = false;
        if ((m_previewImage) &&
            (m_selectedLayer == layer)) {
          if (m_selectedFrame == frame) {
            usePreview = true;
          }
          else {
            // This preview might be useful if we are rendering a
            // linked frame to the preview.
            Cel* cel2 = layer->cel(m_selectedFrame);
            if (cel2->data() == cel->data()) {
              usePreview = true;
            }
          }
        }

        // If not, we use the original cel-image from the images' stock
        if (usePreview) {
          celImage = m_previewImage;
          celBounds = gfx::RectF(m_previewPos.x,
                                 m_previewPos.y,
                                 m_previewImage->width(),
                                 m_previewImage->height());

          ASSERT(celImage->pixelFormat() == cel->image()->pixelFormat());
        }
        else {
          celImage = cel->image();
          if (cel->layer()->isReference())
            celBounds = cel->boundsF();
          else
            celBounds = cel->bounds();
        }

        if (celImage) {
          const LayerImage* imgLayer = static_cast<const LayerImage*>(layer);
          const BlendMode layerBlendMode =
            (blendMode == BlendMode::UNSPECIFIED ?
             imgLayer->blendMode():
             blendMode);

          ASSERT(cel->opacity() >= 0);
          ASSERT(cel->opacity() <= 255);
          ASSERT(imgLayer->opacity() >= 0);
          ASSERT(imgLayer->opacity() <= 255);

          // Multiple three opacities: cel*layer*global (*nonactive-layer-opacity)
          int t;
          int opacity = cel->opacity();
          opacity = MUL_UN8(opacity, imgLayer->opacity(), t);
          opacity = MUL_UN8(opacity, m_globalOpacity, t);
          if (!isSelected && m_nonactiveLayersOpacity != 255)
            opacity = MUL_UN8(opacity, m_nonactiveLayersOpacity, t);

          ASSERT(celImage->maskColor() == m_sprite->transparentColor());

          // Draw parts outside the "m_extraCel" area
          if (drawExtra && m_extraType == ExtraType::PATCH) {
            gfx::Region originalAreas(area.srcBounds());
            originalAreas.createSubtraction(
              originalAreas, gfx::Region(extraArea));

            for (auto rc : originalAreas) {
              renderCel(
                image, celImage, pal, celBounds,
                gfx::Clip(area.dst.x+rc.x-area.src.x,
                          area.dst.y+rc.y-area.src.y, rc), compositeImage,
                opacity, layerBlendMode);
            }
          }
          // Draw the whole cel
          else {
            renderCel(
              image, celImage, pal,
              celBounds, area, compositeImage,
              opacity, layerBlendMode);
          }
        }
      }
      break;
    }

    case ObjectType::LayerGroup: {
      for (const Layer* child : static_cast<const LayerGroup*>(layer)->layers()) {
        renderLayer(
          child, image,
          area, frame,
          compositeImage,
          render_background,
          render_transparent,
          blendMode,
          isSelected);
      }
      break;
    }

  }

  // Draw extras
  if (drawExtra && m_extraType != ExtraType::NONE) {
    if (m_extraCel->opacity() > 0) {
      renderCel(
        image, m_extraImage,
        m_sprite->palette(frame),
        m_extraCel->bounds(),
        gfx::Clip(area.dst.x+extraArea.x-area.src.x,
                  area.dst.y+extraArea.y-area.src.y,
                  extraArea),
        compositeImage,
        m_extraCel->opacity(),
        m_extraBlendMode);
    }
  }
}

void Render::renderCel(
  Image* dst_image,
  const Sprite* sprite,
  const Image* cel_image,
  const Layer* cel_layer,
  const Palette* pal,
  const gfx::RectF& celBounds,
  const gfx::Clip& area,
  const int opacity,
  const BlendMode blendMode)
{
  m_sprite = sprite;

  CompositeImageFunc compositeImage =
    getImageComposition(
      dst_image->pixelFormat(),
      sprite->pixelFormat(), nullptr);
  if (!compositeImage)
    return;

  renderCel(
    dst_image,
    cel_image,
    pal,
    celBounds,
    area,
    compositeImage,
    opacity,
    blendMode);
}

void Render::renderCel(
  Image* dst_image,
  const Image* cel_image,
  const Palette* pal,
  const gfx::RectF& celBounds,
  const gfx::Clip& area,
  const CompositeImageFunc compositeImage,
  const int opacity,
  const BlendMode blendMode)
{
  renderImage(dst_image,
              cel_image,
              pal,
              celBounds,
              area,
              compositeImage,
              opacity,
              blendMode);
}

void Render::renderImage(
  Image* dst_image,
  const Image* cel_image,
  const Palette* pal,
  const gfx::RectF& celBounds,
  const gfx::Clip& area,
  const CompositeImageFunc compositeImage,
  const int opacity,
  const BlendMode blendMode)
{
  gfx::RectF scaledBounds = m_proj.apply(celBounds);
  gfx::RectF srcBounds = gfx::RectF(area.srcBounds()).createIntersection(scaledBounds);
  if (srcBounds.isEmpty())
    return;

  compositeImage(
    dst_image, cel_image, pal,
    gfx::ClipF(
      double(area.dst.x) + srcBounds.x - double(area.src.x),
      double(area.dst.y) + srcBounds.y - double(area.src.y),
      srcBounds.x - scaledBounds.x,
      srcBounds.y - scaledBounds.y,
      srcBounds.w,
      srcBounds.h),
    opacity,
    blendMode,
    m_proj.scaleX() * celBounds.w / double(cel_image->width()),
    m_proj.scaleY() * celBounds.h / double(cel_image->height()),
    m_newBlendMethod);
}

CompositeImageFunc Render::getImageComposition(
  const PixelFormat dstFormat,
  const PixelFormat srcFormat,
  const Layer* layer)
{
  // True if we need blending pixel by pixel. If this is false we can
  // blend src+dst one time and repeat the resulting color in dst
  // image n-times (where n is the zoom scale).
  double intpart;
  const bool finegrain =
    (!m_bgZoom && (m_bgCheckedSize.w < m_proj.applyX(1) ||
                   m_bgCheckedSize.h < m_proj.applyY(1) ||
                   std::modf(double(m_bgCheckedSize.w) / m_proj.applyX(1.0), &intpart) != 0.0 ||
                   std::modf(double(m_bgCheckedSize.h) / m_proj.applyY(1.0), &intpart) != 0.0)) ||
    (layer &&
     layer->isGroup() &&
     has_visible_reference_layers(static_cast<const LayerGroup*>(layer)));

  switch (srcFormat) {

    case IMAGE_RGB:
      switch (dstFormat) {
        case IMAGE_RGB:       return get_fastest_composition_path<RgbTraits, RgbTraits>(m_proj, finegrain);
        case IMAGE_GRAYSCALE: return get_fastest_composition_path<GrayscaleTraits, RgbTraits>(m_proj, finegrain);
        case IMAGE_INDEXED:   return get_fastest_composition_path<IndexedTraits, RgbTraits>(m_proj, finegrain);
      }
      break;

    case IMAGE_GRAYSCALE:
      switch (dstFormat) {
        case IMAGE_RGB:       return get_fastest_composition_path<RgbTraits, GrayscaleTraits>(m_proj, finegrain);
        case IMAGE_GRAYSCALE: return get_fastest_composition_path<GrayscaleTraits, GrayscaleTraits>(m_proj, finegrain);
        case IMAGE_INDEXED:   return get_fastest_composition_path<IndexedTraits, GrayscaleTraits>(m_proj, finegrain);
      }
      break;

    case IMAGE_INDEXED:
      switch (dstFormat) {
        case IMAGE_RGB:       return get_fastest_composition_path<RgbTraits, IndexedTraits>(m_proj, finegrain);
        case IMAGE_GRAYSCALE: return get_fastest_composition_path<GrayscaleTraits, IndexedTraits>(m_proj, finegrain);
        case IMAGE_INDEXED:   return get_fastest_composition_path<IndexedTraits, IndexedTraits>(m_proj, finegrain);
      }
      break;
  }

  ASSERT(false && "Invalid pixel formats");
  return nullptr;
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
  Render().renderImage(
    dst, src, pal, x, y,
    opacity, blendMode);
}

} // namespace render
