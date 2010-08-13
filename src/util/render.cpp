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

#include "jinete/jlist.h"

#include "core/cfg.h"
#include "raster/image.h"
#include "raster/raster.h"
#include "util/render.h"
#include "settings/settings.h"
#include "ui_context.h"

//////////////////////////////////////////////////////////////////////
// Zoomed merge

template<class DstTraits, class SrcTraits>
class BlenderHelper
{
  BLEND_COLOR m_blend_color;
  ase_uint32 m_mask_color;
public:
  BlenderHelper(const Image* src, const Palette* pal, int blend_mode)
  {
    m_blend_color = SrcTraits::get_blender(blend_mode);
    m_mask_color = src->mask_color;
  }
  inline void operator()(typename DstTraits::address_t& scanline_address,
			 typename DstTraits::address_t& dst_address,
			 typename SrcTraits::address_t& src_address,
			 int opacity)
  {
    if (*src_address != m_mask_color)
      *scanline_address = (*m_blend_color)(*dst_address, *src_address, opacity);
    else
      *scanline_address = *dst_address;
  }
};

template<>
class BlenderHelper<RgbTraits, GrayscaleTraits>
{
  BLEND_COLOR m_blend_color;
  ase_uint32 m_mask_color;
public:
  BlenderHelper(const Image* src, const Palette* pal, int blend_mode)
  {
    m_blend_color = RgbTraits::get_blender(blend_mode);
    m_mask_color = src->mask_color;
  }
  inline void operator()(RgbTraits::address_t& scanline_address,
			 RgbTraits::address_t& dst_address,
			 GrayscaleTraits::address_t& src_address,
			 int opacity)
  {
    if (*src_address != m_mask_color) {
      int v = _graya_getv(*src_address);
      *scanline_address = (*m_blend_color)(*dst_address, _rgba(v, v, v, _graya_geta(*src_address)), opacity);
    }
    else
      *scanline_address = *dst_address;
  }
};

template<>
class BlenderHelper<RgbTraits, IndexedTraits>
{
  const Palette* m_pal;
  int m_blend_mode;
  ase_uint32 m_mask_color;
public:
  BlenderHelper(const Image* src, const Palette* pal, int blend_mode)
  {
    m_blend_mode = blend_mode;
    m_mask_color = src->mask_color;
    m_pal = pal;
  }
  inline void operator()(RgbTraits::address_t& scanline_address,
			 RgbTraits::address_t& dst_address,
			 IndexedTraits::address_t& src_address,
			 int opacity)
  {
    if (m_blend_mode == BLEND_MODE_COPY) {
      *scanline_address = m_pal->getEntry(*src_address);
    }
    else {
      if (*src_address != m_mask_color) {
	*scanline_address = _rgba_blend_normal(*dst_address, m_pal->getEntry(*src_address), opacity);
      }
      else
	*scanline_address = *dst_address;
    }
  }
};

template<class DstTraits, class SrcTraits>
static void merge_zoomed_image(Image* dst, const Image* src, const Palette* pal,
			       int x, int y, int opacity,
			       int blend_mode, int zoom)
{
  BlenderHelper<DstTraits, SrcTraits> blender(src, pal, blend_mode);
  typename SrcTraits::address_t src_address;
  typename DstTraits::address_t dst_address, dst_address_end;
  typename DstTraits::address_t scanline, scanline_address;
  int src_x, src_y, src_w, src_h;
  int dst_x, dst_y, dst_w, dst_h;
  int box_x, box_y, box_w, box_h;
  int offsetx, offsety;
  int line_h, bottom;

  box_w = 1<<zoom;
  box_h = 1<<zoom;

  src_x = 0;
  src_y = 0;
  src_w = src->w;
  src_h = src->h;

  dst_x = x;
  dst_y = y;
  dst_w = src->w<<zoom;
  dst_h = src->h<<zoom;

  // clipping...
  if (dst_x < 0) {
    src_x += (-dst_x)>>zoom;
    src_w -= (-dst_x)>>zoom;
    dst_w -= (-dst_x);
    offsetx = box_w - ((-dst_x) % box_w);
    dst_x = 0;
  }
  else
    offsetx = 0;

  if (dst_y < 0) {
    src_y += (-dst_y)>>zoom;
    src_h -= (-dst_y)>>zoom;
    dst_h -= (-dst_y);
    offsety = box_h - ((-dst_y) % box_h);
    dst_y = 0;
  }
  else
    offsety = 0;

  if (dst_x+dst_w > dst->w) {
    src_w -= (dst_x+dst_w-dst->w) >> zoom;
    dst_w = dst->w - dst_x;
  }

  if (dst_y+dst_h > dst->h) {
    src_h -= (dst_y+dst_h-dst->h) >> zoom;
    dst_h = dst->h - dst_y;
  }

  if ((src_w <= 0) || (src_h <= 0) || (dst_w <= 0) || (dst_h <= 0))
    return;

  bottom = dst_y+dst_h-1;

  // the scanline variable is used to blend src/dst pixels one time for each pixel
  scanline = new typename DstTraits::pixel_t[src_w];

  // for each line to draw of the source image...
  for (y=0; y<src_h; y++) {
    ASSERT(src_x >= 0 && src_x < src->w);
    ASSERT(dst_x >= 0 && dst_x < dst->w);

    // get addresses to each line (beginning of 'src', 'dst', etc.)
    src_address = image_address_fast<SrcTraits>(src, src_x, src_y);
    dst_address = image_address_fast<DstTraits>(dst, dst_x, dst_y);
    dst_address_end = dst_address + dst_w;
    scanline_address = scanline;

    // read 'src' and 'dst' and blend them, put the result in `scanline'
    for (x=0; x<src_w; x++) {
      ASSERT(scanline_address >= scanline);
      ASSERT(scanline_address <  scanline + src_w);

      ASSERT(src_address >= image_address_fast<SrcTraits>(src, src_x, src_y));
      ASSERT(src_address <= image_address_fast<SrcTraits>(src, src_x+src_w-1, src_y));
      ASSERT(dst_address >= image_address_fast<DstTraits>(dst, dst_x, dst_y));
      ASSERT(dst_address <= image_address_fast<DstTraits>(dst, dst_x+dst_w-1, dst_y));
      ASSERT(dst_address <  dst_address_end);

      blender(scanline_address, dst_address, src_address, opacity);

      src_address++;
      if (x == 0 && offsetx > 0)
	dst_address += offsetx;
      else
	dst_address += box_w;
      scanline_address++;

      if (dst_address >= dst_address_end)
	break;
    }
    
    // get the 'height' of the line to be painted in 'dst'
    if ((offsety > 0) && (y == 0))
      line_h = offsety;
    else
      line_h = box_h;

    // draw the line in `dst'
    for (box_y=0; box_y<line_h; box_y++) {
      dst_address = image_address_fast<DstTraits>(dst, dst_x, dst_y);
      dst_address_end = dst_address + dst_w;
      scanline_address = scanline;

      x = 0;

      // first pixel
      if (offsetx > 0) {
        for (box_x=0; box_x<offsetx; box_x++) {
	  ASSERT(scanline_address >= scanline);
	  ASSERT(scanline_address <  scanline + src_w);
	  ASSERT(dst_address >= image_address_fast<DstTraits>(dst, dst_x, dst_y));
	  ASSERT(dst_address <= image_address_fast<DstTraits>(dst, dst_x+dst_w-1, dst_y));
	  ASSERT(dst_address <  dst_address_end);

	  (*dst_address++) = (*scanline_address);

	  if (dst_address >= dst_address_end)
	    goto done_with_line;
        }

        scanline_address++;
        x++;
      }

      // the rest of the line
      for (; x<src_w; x++) {
        for (box_x=0; box_x<box_w; box_x++) {
	  ASSERT(dst_address >= image_address_fast<DstTraits>(dst, dst_x, dst_y));
	  ASSERT(dst_address <= image_address_fast<DstTraits>(dst, dst_x+dst_w-1, dst_y));
	  ASSERT(dst_address <  dst_address_end);

	  (*dst_address++) = (*scanline_address);

	  if (dst_address >= dst_address_end)
	    goto done_with_line;
        }

        scanline_address++;
      }

done_with_line:;

      if (++dst_y > bottom)
        goto done_with_blit;
    }

    // go to the next line in the source image
    src_y++;
  }

done_with_blit:;
  delete[] scanline;
}

//////////////////////////////////////////////////////////////////////
// Render Engine

static RenderEngine::CheckedBgType checked_bg_type;
static bool checked_bg_zoom;
static color_t checked_bg_color1;
static color_t checked_bg_color2;

static int global_opacity = 255;
static Layer *selected_layer = NULL;
static Image *rastering_image = NULL;

void RenderEngine::loadConfig()
{
  checked_bg_type = (CheckedBgType)get_config_int("Options", "CheckedBgType",
						  (int)RenderEngine::CHECKED_BG_16X16);
  checked_bg_zoom = get_config_bool("Options", "CheckedBgZoom", true);
  checked_bg_color1 = get_config_color("Options", "CheckedBgColor1", color_rgb(128, 128, 128));
  checked_bg_color2 = get_config_color("Options", "CheckedBgColor2", color_rgb(192, 192, 192));
}

RenderEngine::CheckedBgType RenderEngine::getCheckedBgType()
{
  return checked_bg_type;
}

void RenderEngine::setCheckedBgType(CheckedBgType type)
{
  checked_bg_type = type;
  set_config_int("Options", "CheckedBgType", (int)type);
}

bool RenderEngine::getCheckedBgZoom()
{
  return checked_bg_zoom;
}

void RenderEngine::setCheckedBgZoom(bool state)
{
  checked_bg_zoom = state;
  set_config_int("Options", "CheckedBgZoom", state);
}

color_t RenderEngine::getCheckedBgColor1()
{
  return checked_bg_color1;
}

void RenderEngine::setCheckedBgColor1(color_t color)
{
  checked_bg_color1 = color;
  set_config_color("Options", "CheckedBgColor1", color);
}

color_t RenderEngine::getCheckedBgColor2()
{
  return checked_bg_color2;
}

void RenderEngine::setCheckedBgColor2(color_t color)
{
  checked_bg_color2 = color;
  set_config_color("Options", "CheckedBgColor2", color);
}

//////////////////////////////////////////////////////////////////////

void RenderEngine::setPreviewImage(Layer *layer, Image *image)
{
  selected_layer = layer;
  rastering_image = image;
}

/**
   Draws the @a frame of animation of the specified @a sprite
   in a new image and return it.

   Positions source_x, source_y, width and height must have the
   zoom applied (sorce_x<<zoom, source_y<<zoom, width<<zoom, etc.)
 */
Image* RenderEngine::renderSprite(const Sprite* sprite,
				  int source_x, int source_y,
				  int width, int height,
				  int frame, int zoom,
				  bool draw_tiled_bg)
{
  void (*zoomed_func)(Image*, const Image*, const Palette*, int, int, int, int, int);
  const LayerImage* background = sprite->getBackgroundLayer();
  bool need_checked_bg = (background != NULL ? !background->is_readable(): true);
  ase_uint32 bg_color = 0;
  Image *image;

  switch (sprite->getImgType()) {

    case IMAGE_RGB:
      zoomed_func = merge_zoomed_image<RgbTraits, RgbTraits>;
      break;

    case IMAGE_GRAYSCALE:
      zoomed_func = merge_zoomed_image<RgbTraits, GrayscaleTraits>;
      break;

    case IMAGE_INDEXED:
      zoomed_func = merge_zoomed_image<RgbTraits, IndexedTraits>;
      if (!need_checked_bg)
	bg_color = sprite->getPalette(frame)->getEntry(0);
      break;

    default:
      return NULL;
  }

  // Create a temporary RGB bitmap to draw all to it
  image = image_new(IMAGE_RGB, width, height);
  if (!image)
    return NULL;

  // Draw checked background
  if (need_checked_bg && draw_tiled_bg)
    renderCheckedBackground(image, source_x, source_y, zoom);
  else
    image_clear(image, bg_color);

  // Onion-skin feature: draw the previous frame
  ISettings* settings = UIContext::instance()->getSettings();
  if (settings->getUseOnionskin()) {
    // Draw background layer of the current frame with opacity=255
    global_opacity = 255;
    renderLayer(sprite, sprite->getFolder(), image, source_x, source_y,
		frame, zoom, zoomed_func, true, false);

    // Draw transparent layers of the previous/next frames with different opacity (<255) (it is the onion-skinning)
    {
      int prevs = settings->getOnionskinPrevFrames();
      int nexts = settings->getOnionskinNextFrames();
      int opacity_base = settings->getOnionskinOpacityBase();
      int opacity_step = settings->getOnionskinOpacityStep();

      for (int f=frame-prevs; f <= frame+nexts; ++f) {
	if (f == frame || f < 0 || f >= sprite->getTotalFrames())
	  continue;
	else if (f < frame)
	  global_opacity = opacity_base - opacity_step * ((frame - f)-1);
	else
	  global_opacity = opacity_base - opacity_step * ((f - frame)-1);

	if (global_opacity > 0)
	  renderLayer(sprite, sprite->getFolder(), image, source_x, source_y,
		      f, zoom, zoomed_func, false, true);
      }
    }

    // Draw transparent layers of the current frame with opacity=255
    global_opacity = 255;
    renderLayer(sprite, sprite->getFolder(), image, source_x, source_y,
		frame, zoom, zoomed_func, false, true);
  }
  // Onion-skin is disabled: just draw the current frame
  else {
    renderLayer(sprite, sprite->getFolder(), image, source_x, source_y,
		frame, zoom, zoomed_func, true, true);
  }

  return image;
}

void RenderEngine::renderCheckedBackground(Image* image,
					   int source_x, int source_y,
					   int zoom)
{
  int x, y, u, v;
  int tile_w = 16;
  int tile_h = 16;
  int c1 = get_color_for_image(image->imgtype, checked_bg_color1);
  int c2 = get_color_for_image(image->imgtype, checked_bg_color2);

  switch (checked_bg_type) {

    case CHECKED_BG_16X16:
      tile_w = 16;
      tile_h = 16;
      break;

    case CHECKED_BG_8X8:
      tile_w = 8;
      tile_h = 8;
      break;

    case CHECKED_BG_4X4:
      tile_w = 4;
      tile_h = 4;
      break;

    case CHECKED_BG_2X2:
      tile_w = 2;
      tile_h = 2;
      break;

  }

  if (checked_bg_zoom) {
    tile_w <<= zoom;
    tile_h <<= zoom;
  }

  // Tile size
  if (tile_w < (1<<zoom)) tile_w = (1<<zoom);
  if (tile_h < (1<<zoom)) tile_h = (1<<zoom);

  // Tile position (u,v) is the number of tile we start in (source_x,source_y) coordinate
  u = (source_x / tile_w);
  v = (source_y / tile_h);

  // Position where we start drawing the first tile in "image"
  int x_start = -(source_x % tile_w);
  int y_start = -(source_y % tile_h);

  // Draw checked background (tile by tile)
  int u_start = u;
  for (y=y_start-tile_h; y<image->h+tile_h; y+=tile_h) {
    for (x=x_start-tile_w; x<image->w+tile_w; x+=tile_w) {
      image_rectfill(image, x, y, x+tile_w-1, y+tile_h-1,
		     (((u+v))&1)? c1: c2);
      ++u;
    }
    u = u_start;
    ++v;
  }
}

void RenderEngine::renderImage(Image* rgb_image, Image* src_image, const Palette* pal,
			       int x, int y, int zoom)
{
  void (*zoomed_func)(Image*, const Image*, const Palette*, int, int, int, int, int);

  ASSERT(rgb_image->imgtype == IMAGE_RGB && "renderImage accepts RGB destination images only");

  switch (src_image->imgtype) {

    case IMAGE_RGB:
      zoomed_func = merge_zoomed_image<RgbTraits, RgbTraits>;
      break;

    case IMAGE_GRAYSCALE:
      zoomed_func = merge_zoomed_image<RgbTraits, GrayscaleTraits>;
      break;

    case IMAGE_INDEXED:
      zoomed_func = merge_zoomed_image<RgbTraits, IndexedTraits>;
      break;

    default:
      return;
  }

  (*zoomed_func)(rgb_image, src_image, pal, x, y, 255, BLEND_MODE_NORMAL, zoom);
}

void RenderEngine::renderLayer(const Sprite *sprite,
			       const Layer *layer,
			       Image *image,
			       int source_x, int source_y,
			       int frame, int zoom,
			       void (*zoomed_func)(Image*, const Image*, const Palette*, int, int, int, int, int),
			       bool render_background,
			       bool render_transparent)
{
  // we can't read from this layer
  if (!layer->is_readable())
    return;

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE: {
      if ((!render_background  &&  layer->is_background()) ||
	  (!render_transparent && !layer->is_background()))
	break;

      const Cel* cel = static_cast<const LayerImage*>(layer)->get_cel(frame);
      if (cel != NULL) {
	const Image* src_image;

	/* is the 'rastering_image' setted to be used with this layer? */
	if ((frame == sprite->getCurrentFrame()) &&
	    (selected_layer == layer) &&
	    (rastering_image != NULL)) {
	  src_image = rastering_image;
	}
	/* if not, we use the original cel-image from the images' stock */
	else if ((cel->image >= 0) &&
		 (cel->image < layer->getSprite()->getStock()->nimage))
	  src_image = layer->getSprite()->getStock()->image[cel->image];
	else
	  src_image = NULL;

	if (src_image) {
	  int output_opacity;
	  register int t;

	  output_opacity = MID(0, cel->opacity, 255);
	  output_opacity = INT_MULT(output_opacity, global_opacity, t);

	  (*zoomed_func)(image, src_image, sprite->getPalette(frame),
			 (cel->x << zoom) - source_x,
			 (cel->y << zoom) - source_y,
			 output_opacity,
			 static_cast<const LayerImage*>(layer)->get_blend_mode(), zoom);
	}
      }
      break;
    }

    case GFXOBJ_LAYER_FOLDER: {
      LayerConstIterator it = static_cast<const LayerFolder*>(layer)->get_layer_begin();
      LayerConstIterator end = static_cast<const LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it) {
	renderLayer(sprite, *it, image,
		    source_x, source_y,
		    frame, zoom, zoomed_func,
		    render_background,
		    render_transparent);
      }
      break;
    }

  }

  // Draw extras
  if (layer == sprite->getCurrentLayer() && sprite->getExtraCel() != NULL) {
    Cel* extraCel = sprite->getExtraCel();
    if (extraCel->opacity > 0) {
      Image* extraImage = sprite->getExtraCelImage();
      (*zoomed_func)(image, extraImage, sprite->getPalette(frame),
		     (extraCel->x << zoom) - source_x,
		     (extraCel->y << zoom) - source_y,
		     extraCel->opacity, BLEND_MODE_NORMAL, zoom);
    }
  }
}
