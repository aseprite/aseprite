// Aseprite
// Copyright (C) 2016  Carlo Caputo
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/color_utils.h"
#include "app/document.h"
#include "app/pref/preferences.h"
#include "app/thumbnails.h"
#include "doc/algorithm/resize_image.h"
#include "doc/cel.h"
#include "doc/conversion_she.h"
#include "doc/doc.h"
#include "doc/frame.h"
#include "doc/object.h"
#include "doc/object_id.h"
#include "render/render.h"
#include "she/surface.h"
#include "she/system.h"

namespace app {
  namespace thumb {

    she::Surface* get_cel_thumbnail(const doc::Cel* cel, gfx::Size thumb_size,
      gfx::Rect cel_image_on_thumb)
    {
      app::Document* document = static_cast<app::Document*>(cel->sprite()->document());
      doc::frame_t frame = cel->frame();
      doc::Image* image = cel->image();

      DocumentPreferences& docPref = Preferences::instance().document(document);

      int opacity = docPref.thumbnails.opacity();
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
          MAX(1, (int)(image_size.h * zoom))
        );
      }

      const doc::Sprite* sprite = document->sprite();
      // TODO doc::Image::createCopy() from pre-rendered checkered background
      base::UniquePtr<doc::Image> thumb_img(doc::Image::create(
        image->pixelFormat(), thumb_size.w, thumb_size.h));

      double alpha = opacity / 255.0;
      uint8_t bg_r[] = { rgba_getr(bg1), rgba_getr(bg2) };
      uint8_t bg_g[] = { rgba_getg(bg1), rgba_getg(bg2) };
      uint8_t bg_b[] = { rgba_getb(bg1), rgba_getb(bg2) };
      uint8_t bg_a[] = { rgba_geta(bg1), rgba_geta(bg2) };

      doc::color_t bg[] = {
        rgba(bg_r[0], bg_g[0], bg_b[0], (int)(bg_a[0] * alpha)),
        rgba(bg_r[1], bg_g[1], bg_b[1], (int)(bg_a[1] * alpha))
      };

      int block_size = MID(4, thumb_size.w/8, 16);
      for (int dst_y = 0; dst_y < thumb_size.h; dst_y++) {
        for (int dst_x = 0; dst_x < thumb_size.w; dst_x++) {
          thumb_img->putPixel(dst_x, dst_y,
            bg[
              ((dst_x / block_size) % 2) ^
              ((dst_y / block_size) % 2)
            ]
          );
        }
      }

      base::UniquePtr<doc::Image> scale_img;
      const doc::Image* source = image;

      if (cel_image_on_thumb.w != image_size.w || cel_image_on_thumb.h != image_size.h) {
        scale_img.reset(doc::Image::create(
          image->pixelFormat(), cel_image_on_thumb.w, cel_image_on_thumb.h));

        doc::algorithm::resize_image(
          image, scale_img,
          doc::algorithm::ResizeMethod::RESIZE_METHOD_NEAREST_NEIGHBOR,
          sprite->palette(frame),
          sprite->rgbMap(frame),
          sprite->transparentColor());

        source = scale_img.get();
      }

      render::composite_image(
        thumb_img, source,
        sprite->palette(frame),
        cel_image_on_thumb.x,
        cel_image_on_thumb.y,
        opacity, BlendMode::NORMAL);

      she::Surface* thumb_surf = she::instance()->createRgbaSurface(
        thumb_img->width(), thumb_img->height());

      convert_image_to_surface(thumb_img, sprite->palette(frame), thumb_surf,
        0, 0, 0, 0, thumb_img->width(), thumb_img->height());

      return thumb_surf;
    }

  } // thumb
} // app
