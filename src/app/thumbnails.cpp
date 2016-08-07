// Aseprite
// Copyright (C) 2016  Carlo Caputo
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
      gfx::Color background = color_utils::color_for_ui(docPref.thumbnails.background());
      doc::algorithm::ResizeMethod resize_method = docPref.thumbnails.quality();

      gfx::Size image_size = image->size();

      if (cel_image_on_thumb.isEmpty()) {
        double zw = thumb_size.w / (double)image_size.w;
        double zh = thumb_size.h / (double)image_size.h;
        double zoom = MIN(1, MIN(zw, zh));

        cel_image_on_thumb = gfx::Rect(
          (int)(thumb_size.w * 0.5 - image_size.w  * zoom * 0.5),
          (int)(thumb_size.h * 0.5 - image_size.h * zoom * 0.5),
          (int)(image_size.w  * zoom),
          (int)(image_size.h * zoom)
        );
      }

      const doc::Sprite* sprite = document->sprite();
      base::UniquePtr<doc::Image> thumb_img(doc::Image::create(
        image->pixelFormat(), thumb_size.w, thumb_size.h));

      clear_image(thumb_img.get(), background);

      base::UniquePtr<doc::Image> scale_img;
      const doc::Image* source = image;

      if (cel_image_on_thumb.w != image_size.w || cel_image_on_thumb.h != image_size.h) {
        scale_img.reset(doc::Image::create(
          image->pixelFormat(), cel_image_on_thumb.w, cel_image_on_thumb.h));

        doc::algorithm::resize_image(
          image, scale_img,
          resize_method,
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
        255, BlendMode::NORMAL);

      she::Surface* thumb_surf = she::instance()->createRgbaSurface(
        thumb_img->width(), thumb_img->height());

      convert_image_to_surface(thumb_img, sprite->palette(frame), thumb_surf,
        0, 0, 0, 0, thumb_img->width(), thumb_img->height());

      return thumb_surf;
    }

  } // thumb
} // app
