// Aseprite
// Copyright (C) 2018  David Capello
// Copyright (C) 2016  Carlo Caputo
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/color_utils.h"
#include "app/doc.h"
#include "app/pref/preferences.h"
#include "app/thumbnails.h"
#include "doc/algorithm/resize_image.h"
#include "doc/cel.h"
#include "doc/conversion_to_surface.h"
#include "doc/doc.h"
#include "doc/frame.h"
#include "doc/object.h"
#include "doc/object_id.h"
#include "render/render.h"
#include "os/surface.h"
#include "os/system.h"

namespace app {
namespace thumb {

os::Surface* get_cel_thumbnail(const doc::Cel* cel,
                                const gfx::Size& thumb_size,
                                gfx::Rect cel_image_on_thumb)
{
  Doc* document = static_cast<Doc*>(cel->sprite()->document());
  doc::frame_t frame = cel->frame();
  doc::Image* image = cel->image();

  DocumentPreferences& docPref = Preferences::instance().document(document);

  doc::color_t bg1 = color_utils::color_for_image(docPref.bg.color1(), image->pixelFormat());
  doc::color_t bg2 = color_utils::color_for_image(docPref.bg.color2(), image->pixelFormat());

  gfx::Size image_size = image->size();

  if (cel_image_on_thumb.isEmpty()) {
    double zw = thumb_size.w / (double)image_size.w;
    double zh = thumb_size.h / (double)image_size.h;
    double zoom = MIN(1.0, MIN(zw, zh));

    cel_image_on_thumb = gfx::Rect(
      (int)(thumb_size.w * 0.5 - image_size.w * zoom * 0.5),
      (int)(thumb_size.h * 0.5 - image_size.h * zoom * 0.5),
      MAX(1, (int)(image_size.w * zoom)),
      MAX(1, (int)(image_size.h * zoom)));
  }

  const doc::Sprite* sprite = document->sprite();
  // TODO doc::Image::createCopy() from pre-rendered checkered background
  std::unique_ptr<doc::Image> thumb_img(doc::Image::create(
                                          image->pixelFormat(), thumb_size.w, thumb_size.h));

  int block_size = MID(4, thumb_size.w/8, 16);
  for (int dst_y = 0; dst_y < thumb_size.h; dst_y++) {
    for (int dst_x = 0; dst_x < thumb_size.w; dst_x++) {
      thumb_img->putPixel(dst_x, dst_y,
                          (((dst_x / block_size) % 2) ^
                           ((dst_y / block_size) % 2)) ? bg2: bg1);
    }
  }

  std::unique_ptr<doc::Image> scale_img;
  const doc::Image* source = image;

  if (cel_image_on_thumb.w != image_size.w || cel_image_on_thumb.h != image_size.h) {
    scale_img.reset(doc::Image::create(
                      image->pixelFormat(), cel_image_on_thumb.w, cel_image_on_thumb.h));

    doc::algorithm::resize_image(
      image, scale_img.get(),
      doc::algorithm::ResizeMethod::RESIZE_METHOD_NEAREST_NEIGHBOR,
      sprite->palette(frame),
      sprite->rgbMap(frame),
      sprite->transparentColor());

    source = scale_img.get();
  }

  render::composite_image(
    thumb_img.get(), source,
    sprite->palette(frame),
    cel_image_on_thumb.x,
    cel_image_on_thumb.y,
    255, BlendMode::NORMAL);

  os::Surface* thumb_surf = os::instance()->createRgbaSurface(
    thumb_img->width(), thumb_img->height());

  convert_image_to_surface(thumb_img.get(), sprite->palette(frame), thumb_surf,
                           0, 0, 0, 0, thumb_img->width(), thumb_img->height());

  return thumb_surf;
}

} // thumb
} // app
