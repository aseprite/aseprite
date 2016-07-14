// Aseprite
// Copyright (C) 2016  Carlo Caputo
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "app/thumbnails.h"
#include "doc/cel.h"
#include "doc/doc.h"
#include "doc/algorithm/rotate.h"
#include "doc/algorithm/resize_image.h"
#include "app/pref/preferences.h"
#include "app/document.h"
#include "app/color_utils.h"
#include "render/render.h"
#include "she/surface.h"
#include "she/system.h"
#include "doc/conversion_she.h"
#include "base/bind.h"

namespace app {
  namespace thumb {

    ///////// constructors

    Request::Request() :
      document(nullptr),
      frame(0),
      image(nullptr),
      dimension(0, 0)
    {}

    Request::Request(const app::Document* doc, const doc::frame_t frm, const doc::Image* img, const Dimension dim) :
      document(doc),
      frame(frm),
      image(img),
      dimension(dim)
    {}

    she::Surface* SurfaceData::fetch(const app::Document* doc, const doc::Cel* cel, const gfx::Rect& bounds)
    {
      const Dimension dim = std::make_pair(bounds.w, bounds.h);
      Request req(doc, cel->frame(), cel->image(), dim);
      return generate(req);
    }

    she::Surface* SurfaceData::generate(Request& req)
    {
      DocumentPreferences& docPref = Preferences::instance().document(req.document);

      int opacity = docPref.thumbnails.opacity();
      gfx::Color background = color_utils::color_for_ui(docPref.thumbnails.background());
      doc::algorithm::ResizeMethod resize_method = docPref.thumbnails.quality();

      gfx::Size image_size = req.image->size();
      gfx::Size thumb_size = gfx::Size(req.dimension.first, req.dimension.second);

      double zw = thumb_size.w / (double)image_size.w;
      double zh = thumb_size.h / (double)image_size.h;
      double zoom = MIN(1, MIN(zw, zh));

      gfx::Rect cel_image_on_thumb(
        (int)(thumb_size.w * 0.5 - image_size.w  * zoom * 0.5),
        (int)(thumb_size.h * 0.5 - image_size.h * zoom * 0.5),
        (int)(image_size.w  * zoom),
        (int)(image_size.h * zoom)
      );

      const doc::Sprite* sprite = req.document->sprite();
      base::UniquePtr<doc::Image> thumb_img(doc::Image::create(
        req.image->pixelFormat(), thumb_size.w, thumb_size.h));

      clear_image(thumb_img.get(), background);

      if (opacity == 255 && resize_method == doc::algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR) {
        algorithm::scale_image(
          thumb_img, req.image,
          cel_image_on_thumb.x, cel_image_on_thumb.y,
          cel_image_on_thumb.w, cel_image_on_thumb.h,
          0, 0, image_size.w, image_size.h);
      }
      else {
        base::UniquePtr<doc::Image> scale_img;
        const doc::Image* source = req.image;

        if (zoom != 1) {
          scale_img.reset(doc::Image::create(
            req.image->pixelFormat(), cel_image_on_thumb.w, cel_image_on_thumb.h));


          doc::algorithm::resize_image(
            req.image, scale_img,
            resize_method,
            sprite->palette(req.frame),
            sprite->rgbMap(req.frame),
            sprite->transparentColor());

          source = scale_img.get();
        }

        render::composite_image(
          thumb_img, source,
          sprite->palette(req.frame),
          cel_image_on_thumb.x,
          cel_image_on_thumb.y,
          opacity, BlendMode::NORMAL);
      }

      she::Surface* thumb_surf = she::instance()->createRgbaSurface(
        thumb_img->width(), thumb_img->height());

      convert_image_to_surface(thumb_img, sprite->palette(req.frame), thumb_surf,
        0, 0, 0, 0, thumb_img->width(), thumb_img->height());

      return thumb_surf;
    }

  } // thumb
} // app
