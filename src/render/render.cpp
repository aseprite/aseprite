// Aseprite Render Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "render/render.h"

#include "base/base.h"
#include "doc/blend_internals.h"
#include "doc/blend_mode.h"
#include "doc/doc.h"
#include "doc/handle_anidir.h"
#include "doc/image_impl.h"
#include "gfx/clip.h"
#include "gfx/region.h"

namespace render {

namespace {

//////////////////////////////////////////////////////////////////////
// Scaled composite

template<class DstTraits, class SrcTraits>
class BlenderHelper {
  BlendFunc m_blendFunc;
  color_t m_mask_color;
public:
  BlenderHelper(const Image* src, const Palette* pal, BlendMode blendMode)
  {
    m_blendFunc = SrcTraits::get_blender(blendMode);
    m_mask_color = src->maskColor();
  }
  inline typename DstTraits::pixel_t
  operator()(const typename DstTraits::pixel_t& dst,
             const typename SrcTraits::pixel_t& src,
             int opacity)
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
  BlenderHelper(const Image* src, const Palette* pal, BlendMode blendMode)
  {
    m_blendFunc = RgbTraits::get_blender(blendMode);
    m_mask_color = src->maskColor();
  }
  inline RgbTraits::pixel_t
  operator()(const RgbTraits::pixel_t& dst,
             const GrayscaleTraits::pixel_t& src,
             int opacity)
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
  BlenderHelper(const Image* src, const Palette* pal, BlendMode blendMode)
  {
    m_blendMode = blendMode;
    m_blendFunc = RgbTraits::get_blender(blendMode);
    m_mask_color = src->maskColor();
    m_pal = pal;
  }
  inline RgbTraits::pixel_t
  operator()(const RgbTraits::pixel_t& dst,
             const IndexedTraits::pixel_t& src,
                         int opacity)
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
  BlenderHelper(const Image* src, const Palette* pal, BlendMode blendMode)
  {
    m_blendMode = blendMode;
    m_mask_color = src->maskColor();
  }
  inline IndexedTraits::pixel_t
  operator()(const IndexedTraits::pixel_t& dst,
             const IndexedTraits::pixel_t& src,
             int opacity)
  {
    if (m_blendMode == BlendMode::SRC) {
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
  Image* dst,
  const Image* src,
  const Palette* pal,
  const gfx::Clip& _area,
  const int opacity,
  const BlendMode blendMode,
  const Zoom& zoom)
{
  ASSERT(dst);
  ASSERT(src);
  ASSERT(DstTraits::pixel_format == dst->pixelFormat());
  ASSERT(SrcTraits::pixel_format == src->pixelFormat());

  BlenderHelper<DstTraits, SrcTraits> blender(src, pal, blendMode);

  gfx::Clip area = _area;
  if (!area.clip(dst->width(), dst->height(),
                 src->width(), src->height()))
    return;

  gfx::Rect srcBounds = area.srcBounds();
  gfx::Rect dstBounds = area.dstBounds();
  int bottom = area.dst.y+area.size.h-1;

  ASSERT(!srcBounds.isEmpty());

  // Lock all necessary bits
  const LockImageBits<SrcTraits> srcBits(src, srcBounds);
  LockImageBits<DstTraits> dstBits(dst, dstBounds);
  typename LockImageBits<SrcTraits>::const_iterator src_it = srcBits.begin();
  typename LockImageBits<SrcTraits>::const_iterator src_end = srcBits.end();
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
  Image* dst,
  const Image* src,
  const Palette* pal,
  const gfx::Clip& _area,
  const int opacity,
  const BlendMode blendMode,
  const Zoom& zoom)
{
  ASSERT(dst);
  ASSERT(src);
  ASSERT(DstTraits::pixel_format == dst->pixelFormat());
  ASSERT(SrcTraits::pixel_format == src->pixelFormat());

  BlenderHelper<DstTraits, SrcTraits> blender(src, pal, blendMode);
  int px_x, px_y;

  gfx::Clip area = _area;
  if (!area.clip(dst->width(), dst->height(),
      zoom.apply(src->width()),
      zoom.apply(src->height())))
    return;

  int px_w = zoom.apply(1);
  int px_h = zoom.apply(1);
  int first_px_w = px_w - (area.src.x % px_w);
  int first_px_h = px_h - (area.src.y % px_h);
  gfx::Rect srcBounds = zoom.remove(area.srcBounds());
  gfx::Rect dstBounds = area.dstBounds();
  int bottom = area.dst.y+area.size.h-1;
  int line_h;

  if ((area.src.x+area.size.w) % px_w > 0) ++srcBounds.w;
  if ((area.src.y+area.size.h) % px_h > 0) ++srcBounds.h;

  if (srcBounds.isEmpty())
    return;

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
  const gfx::Clip& _area,
  const int opacity,
  const BlendMode blendMode,
  const Zoom& zoom)
{
  ASSERT(dst);
  ASSERT(src);
  ASSERT(DstTraits::pixel_format == dst->pixelFormat());
  ASSERT(SrcTraits::pixel_format == src->pixelFormat());

  BlenderHelper<DstTraits, SrcTraits> blender(src, pal, blendMode);
  int unbox_w = zoom.remove(1);
  int unbox_h = zoom.remove(1);

  gfx::Clip area = _area;
  if (!area.clip(dst->width(), dst->height(),
      zoom.apply(src->width()),
      zoom.apply(src->height())))
    return;

  gfx::Rect srcBounds = zoom.remove(area.srcBounds());
  gfx::Rect dstBounds = area.dstBounds();
  int bottom = area.dst.y+area.size.h-1;

  if (srcBounds.isEmpty())
    return;

  // Lock all necessary bits
  const LockImageBits<SrcTraits> srcBits(src, srcBounds);
  LockImageBits<DstTraits> dstBits(dst, dstBounds);
  typename LockImageBits<SrcTraits>::const_iterator src_it = srcBits.begin();
  typename LockImageBits<SrcTraits>::const_iterator src_end = srcBits.end();
  typename LockImageBits<DstTraits>::iterator dst_it, dst_end;

  // For each line to draw of the source image...
  dstBounds.h = 1;
  for (int y=0; y<srcBounds.h; y+=unbox_h) {
    dst_it = dstBits.begin_area(dstBounds);
    dst_end = dstBits.end_area(dstBounds);

    for (int x=0; x<srcBounds.w; x+=unbox_w) {
      ASSERT(src_it >= srcBits.begin() && src_it < src_end);
      ASSERT(dst_it >= dstBits.begin() && dst_it < dst_end);

      *dst_it = blender(*dst_it, *src_it, opacity);

      // Skip source pixels
      for (int delta=0; delta < unbox_w && src_it != src_end; ++delta)
        ++src_it;

      ++dst_it;
    }

    if (++dstBounds.y > bottom)
      break;

    // Skip lines
    for (int delta=0; delta < srcBounds.w * (unbox_h-1) && src_it != src_end; ++delta)
      ++src_it;
  }
}

template<class DstTraits, class SrcTraits>
CompositeImageFunc get_image_composition_impl(Zoom zoom)
{
  if (zoom.scale() == 1.0)
    return composite_image_without_scale<DstTraits, SrcTraits>;
  else if (zoom.scale() > 1.0)
    return composite_image_scale_up<DstTraits, SrcTraits>;
  else
    return composite_image_scale_down<DstTraits, SrcTraits>;
}

CompositeImageFunc get_image_composition(PixelFormat dstFormat,
                                         PixelFormat srcFormat,
                                         const Zoom& zoom)
{
  switch (srcFormat) {

    case IMAGE_RGB:
      switch (dstFormat) {
        case IMAGE_RGB:       return get_image_composition_impl<RgbTraits, RgbTraits>(zoom);
        case IMAGE_GRAYSCALE: return get_image_composition_impl<GrayscaleTraits, RgbTraits>(zoom);
        case IMAGE_INDEXED:   return get_image_composition_impl<IndexedTraits, RgbTraits>(zoom);
      }
      break;

    case IMAGE_GRAYSCALE:
      switch (dstFormat) {
        case IMAGE_RGB:       return get_image_composition_impl<RgbTraits, GrayscaleTraits>(zoom);
        case IMAGE_GRAYSCALE: return get_image_composition_impl<GrayscaleTraits, GrayscaleTraits>(zoom);
        case IMAGE_INDEXED:   return get_image_composition_impl<IndexedTraits, GrayscaleTraits>(zoom);
      }
      break;

    case IMAGE_INDEXED:
      switch (dstFormat) {
        case IMAGE_RGB:       return get_image_composition_impl<RgbTraits, IndexedTraits>(zoom);
        case IMAGE_GRAYSCALE: return get_image_composition_impl<GrayscaleTraits, IndexedTraits>(zoom);
        case IMAGE_INDEXED:   return get_image_composition_impl<IndexedTraits, IndexedTraits>(zoom);
      }
      break;
  }

  ASSERT(false && "Invalid pixel formats");
  return NULL;
}

} // anonymous namespace

Render::Render()
  : m_sprite(NULL)
  , m_currentLayer(NULL)
  , m_currentFrame(0)
  , m_extraType(ExtraType::NONE)
  , m_extraCel(NULL)
  , m_extraImage(NULL)
  , m_bgType(BgType::TRANSPARENT)
  , m_bgCheckedSize(16, 16)
  , m_globalOpacity(255)
  , m_selectedLayer(nullptr)
  , m_selectedFrame(-1)
  , m_previewImage(nullptr)
  , m_previewBlendMode(BlendMode::NORMAL)
  , m_onionskin(OnionskinType::NONE)
{
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
  renderSprite(dstImage, sprite, frame,
    gfx::Clip(sprite->bounds()), Zoom(1, 1));
}

void Render::renderSprite(
  Image* dstImage,
  const Sprite* sprite,
  frame_t frame,
  const gfx::Clip& area)
{
  renderSprite(dstImage, sprite, frame, area, Zoom(1, 1));
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
    get_image_composition(
      dstImage->pixelFormat(),
      m_sprite->pixelFormat(), Zoom(1, 1));
  if (!compositeImage)
    return;

  m_globalOpacity = 255;
  renderLayer(
    layer, dstImage, area,
    frame, Zoom(1, 1), compositeImage,
    true, true, blendMode);
}

void Render::renderSprite(
  Image* dstImage,
  const Sprite* sprite,
  frame_t frame,
  const gfx::Clip& area,
  Zoom zoom)
{
  m_sprite = sprite;

  CompositeImageFunc compositeImage =
    get_image_composition(
      dstImage->pixelFormat(),
      m_sprite->pixelFormat(), zoom);
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

  // Draw checked background
  switch (m_bgType) {

    case BgType::CHECKED:
      if (bgLayer && bgLayer->isVisible() && rgba_geta(bg_color) == 255) {
        fill_rect(dstImage, area.dstBounds(), bg_color);
      }
      else {
        renderBackground(dstImage, area, zoom);
        if (bgLayer && bgLayer->isVisible() && rgba_geta(bg_color) > 0) {
          blend_rect(dstImage, area.dst.x, area.dst.y,
                     area.dst.x+area.size.w-1,
                     area.dst.y+area.size.h-1,
                     bg_color, 255);
        }
      }
      break;

    case BgType::TRANSPARENT:
      fill_rect(dstImage, area.dstBounds(), bg_color);
      break;
  }

  // Draw the background layer.
  m_globalOpacity = 255;
  renderLayer(
    m_sprite->folder(), dstImage,
    area, frame, zoom, compositeImage,
    true,
    false,
    BlendMode::UNSPECIFIED);

  // Draw onion skin behind the sprite.
  if (m_onionskin.position() == OnionskinPosition::BEHIND)
    renderOnionskin(dstImage, area, frame, zoom, compositeImage);

  // Draw the transparent layers.
  m_globalOpacity = 255;
  renderLayer(
    m_sprite->folder(), dstImage,
    area, frame, zoom, compositeImage,
    false,
    true,
    BlendMode::UNSPECIFIED);

  // Draw onion skin in front of the sprite.
  if (m_onionskin.position() == OnionskinPosition::INFRONT)
    renderOnionskin(dstImage, area, frame, zoom, compositeImage);

  // Overlay preview image
  if (m_previewImage &&
      m_selectedLayer == nullptr &&
      m_selectedFrame == frame) {
    renderImage(
      dstImage,
      m_previewImage,
      m_sprite->palette(frame),
      m_previewPos.x,
      m_previewPos.y,
      area,
      compositeImage,
      255,
      m_previewBlendMode,
      zoom);
  }
}

void Render::renderOnionskin(
  Image* dstImage,
  const gfx::Clip& area,
  frame_t frame, Zoom zoom,
  CompositeImageFunc compositeImage)
{
  // Onion-skin feature: Draw previous/next frames with different
  // opacity (<255)
  if (m_onionskin.type() != OnionskinType::NONE) {
    FrameTag* loop = m_onionskin.loopTag();
    Layer* onionLayer = (m_onionskin.layer() ? m_onionskin.layer():
                                               m_sprite->folder());
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

      m_globalOpacity = MID(0, m_globalOpacity, 255);
      if (m_globalOpacity > 0) {
        BlendMode blendMode = BlendMode::UNSPECIFIED;
        if (m_onionskin.type() == OnionskinType::MERGE)
          blendMode = BlendMode::NORMAL;
        else if (m_onionskin.type() == OnionskinType::RED_BLUE_TINT)
          blendMode = (frameOut < frame ? BlendMode::RED_TINT: BlendMode::BLUE_TINT);

        renderLayer(
          onionLayer, dstImage,
          area, frameIn, zoom, compositeImage,
          // Render background only for "in-front" onion skinning and
          // when opacity is < 255
          (m_globalOpacity < 255 &&
           m_onionskin.position() == OnionskinPosition::INFRONT),
          true,
          blendMode);
      }
    }
  }
}

void Render::renderBackground(Image* image,
  const gfx::Clip& area,
  Zoom zoom)
{
  int x, y, u, v;
  int tile_w = m_bgCheckedSize.w;
  int tile_h = m_bgCheckedSize.h;

  if (m_bgZoom) {
    tile_w = zoom.apply(tile_w);
    tile_h = zoom.apply(tile_h);
  }

  // Tile size
  if (tile_w < zoom.apply(1)) tile_w = zoom.apply(1);
  if (tile_h < zoom.apply(1)) tile_h = zoom.apply(1);

  if (tile_w < 1) tile_w = 1;
  if (tile_h < 1) tile_h = 1;

  // Tile position (u,v) is the number of tile we start in "area.src" coordinate
  u = (area.src.x / tile_w);
  v = (area.src.y / tile_h);

  // Position where we start drawing the first tile in "image"
  int x_start = -(area.src.x % tile_w);
  int y_start = -(area.src.y % tile_h);

  gfx::Rect dstBounds = area.dstBounds();

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

void Render::renderImage(Image* dst_image, const Image* src_image,
                         const Palette* pal,
                         int x, int y,
                         Zoom zoom, int opacity, BlendMode blendMode)
{
  CompositeImageFunc compositeImage = get_image_composition(
    dst_image->pixelFormat(),
    src_image->pixelFormat(), zoom);
  if (!compositeImage)
    return;

  compositeImage(dst_image, src_image, pal,
    gfx::Clip(x, y, 0, 0,
      zoom.apply(src_image->width()),
      zoom.apply(src_image->height())),
    opacity, blendMode, zoom);
}

void Render::renderLayer(
  const Layer* layer,
  Image *image,
  const gfx::Clip& area,
  frame_t frame, Zoom zoom,
  CompositeImageFunc compositeImage,
  bool render_background,
  bool render_transparent,
  BlendMode blendMode)
{
  // we can't read from this layer
  if (!layer->isVisible())
    return;

  gfx::Rect extraArea;
  bool drawExtra = (m_extraCel &&
                    m_extraCel->frame() == frame &&
                    m_extraImage &&
                    layer == m_currentLayer &&
                    frame == m_currentFrame &&
                    ((layer->isBackground() && render_background) ||
                     (!layer->isBackground() && render_transparent)));
  if (drawExtra) {
    extraArea = gfx::Rect(
      m_extraCel->x(),
      m_extraCel->y(),
      m_extraImage->width(),
      m_extraImage->height());
    extraArea = zoom.apply(extraArea);
    if (zoom.scale() < 1.0) {
      extraArea.w--;
      extraArea.h--;
    }
    if (extraArea.w < 1) extraArea.w = 1;
    if (extraArea.h < 1) extraArea.h = 1;
  }

  switch (layer->type()) {

    case ObjectType::LayerImage: {
      if ((!render_background  &&  layer->isBackground()) ||
          (!render_transparent && !layer->isBackground()))
        break;

      const Cel* cel = layer->cel(frame);
      if (cel) {
        Palette* pal = m_sprite->palette(frame);
        const Image* celImage;
        gfx::Point celPos;

        // Is the 'm_previewImage' set to be used with this layer?
        if ((m_previewImage) &&
            (m_selectedLayer == layer) &&
            (m_selectedFrame == frame)) {
          celImage = m_previewImage;
          celPos = m_previewPos;

          ASSERT(celImage->pixelFormat() == cel->image()->pixelFormat());
        }
        // If not, we use the original cel-image from the images' stock
        else {
          celImage = cel->image();
          celPos = cel->position();
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

          // Multiple three opacities: cel*layer*global
          int t;
          int opacity = cel->opacity();
          opacity = MUL_UN8(opacity, imgLayer->opacity(), t);
          opacity = MUL_UN8(opacity, m_globalOpacity, t);

          ASSERT(celImage->maskColor() == m_sprite->transparentColor());

          // Draw parts outside the "m_extraCel" area
          if (drawExtra && m_extraType == ExtraType::PATCH) {
            gfx::Region originalAreas(area.srcBounds());
            originalAreas.createSubtraction(
              originalAreas, gfx::Region(extraArea));

            for (auto rc : originalAreas) {
              renderCel(
                image, celImage, pal, celPos,
                gfx::Clip(area.dst.x+rc.x-area.src.x,
                          area.dst.y+rc.y-area.src.y, rc), compositeImage,
                opacity, layerBlendMode, zoom);
            }
          }
          // Draw the whole cel
          else {
            renderCel(
              image, celImage, pal,
              celPos, area, compositeImage,
              opacity, layerBlendMode, zoom);
          }
        }
      }
      break;
    }

    case ObjectType::LayerFolder: {
      LayerConstIterator it = static_cast<const LayerFolder*>(layer)->getLayerBegin();
      LayerConstIterator end = static_cast<const LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it) {
        renderLayer(*it, image,
          area, frame, zoom, compositeImage,
          render_background,
          render_transparent,
          blendMode);
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
        m_extraCel->position(),
        gfx::Clip(area.dst.x+extraArea.x-area.src.x,
                  area.dst.y+extraArea.y-area.src.y,
                  extraArea),
        compositeImage,
        m_extraCel->opacity(),
        m_extraBlendMode, zoom);
    }
  }
}

void Render::renderCel(
  Image* dst_image,
  const Image* cel_image,
  const Palette* pal,
  const gfx::Point& celPos,
  const gfx::Clip& area,
  CompositeImageFunc compositeImage,
  int opacity, BlendMode blendMode, Zoom zoom)
{
  renderImage(dst_image,
              cel_image,
              pal,
              celPos.x,
              celPos.y,
              area,
              compositeImage,
              opacity,
              blendMode,
              zoom);
}

void Render::renderImage(
  Image* dst_image,
  const Image* cel_image,
  const Palette* pal,
  const int x,
  const int y,
  const gfx::Clip& area,
  CompositeImageFunc compositeImage,
  int opacity, BlendMode blendMode, Zoom zoom)
{
  int cel_x = zoom.apply(x);
  int cel_y = zoom.apply(y);

  gfx::Rect src_bounds =
    area.srcBounds().createIntersection(
      gfx::Rect(
        cel_x,
        cel_y,
        zoom.apply(cel_image->width()),
        zoom.apply(cel_image->height())));
  if (src_bounds.isEmpty())
    return;

  (*compositeImage)(dst_image, cel_image, pal,
    gfx::Clip(
      area.dst.x + src_bounds.x - area.src.x,
      area.dst.y + src_bounds.y - area.src.y,
      src_bounds.x - cel_x,
      src_bounds.y - cel_y,
      src_bounds.w,
      src_bounds.h),
    opacity, blendMode, zoom);
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
    dst, src, pal, x, y, Zoom(1, 1),
    opacity, blendMode);
}

} // namespace render
