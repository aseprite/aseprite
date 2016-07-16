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
#include "render/quantization.h"

#include <algorithm>

namespace app {
  namespace thumb {

    Sequence Request::m_sequence = 0;
    Sequence SurfaceData::m_count = 0;

    ///////// constructors

    Request::Request() :
      document(nullptr),
      frame(0),
      image(nullptr),
      surface_size(gfx::Size(0, 0)),
      image_on_surface(gfx::Rect(0, 0, 0, 0)),
      timestamp(-1),
      updated(false)
    {}

    Tag::Tag() :
      document(0),
      image(0),
      dimension(0, 0),
      timestamp(-1)
    {}

    Request::Request(const app::Document* doc, const doc::frame_t frm, 
      const doc::Image* img, const gfx::Size surf_size,
      const gfx::Rect img_on_surf) :
      document(doc),
      frame(frm),
      image(img),
      surface_size(surf_size),
      image_on_surface(img_on_surf),
      timestamp(m_sequence++),
      updated(false)
    {}

    Tag::Tag(const Request& req) :
      document(req.document->id()),
      image(req.image->id()),
      dimension(std::make_pair(req.surface_size.w, req.surface_size.h)),
      timestamp(req.timestamp)
    {}

    SurfaceData::SurfaceData() : m_surface(nullptr) { }

    ImageDir::ImageDir() : m_version(-1) {}

    DocumentDir::DocumentDir() {}

    CacheDir::CacheDir() {}

    ///////// destroying

    SurfaceData::~SurfaceData()
    {
      if (m_surface) {
        m_surface->dispose();
        m_count--;
      }
    }

    DocumentDir::~DocumentDir()
    {
      if (m_thumbnailsPrefConn) {
        m_thumbnailsPrefConn.disconnect();
      }
    }

    void DocumentDir::onThumbnailsPrefChange()
    {
      m_images.clear();
    }

    void CacheDir::onRemoveDocument(doc::Document* doc)
    {
      m_documents.erase(doc->id());
    }

    ///////// generating

    she::Surface* SurfaceData::generate(Request& req)
    {
      req.updated = true;
      gfx::Size image_size = req.image->size();

      if (req.image_on_surface.isEmpty()) {
        double zw = req.surface_size.w / (double)image_size.w;
        double zh = req.surface_size.h / (double)image_size.h;
        double zoom = MIN(1, MIN(zw, zh));

        req.image_on_surface = gfx::Rect(
          (int)(req.surface_size.w * 0.5 - image_size.w  * zoom * 0.5),
          (int)(req.surface_size.h * 0.5 - image_size.h * zoom * 0.5),
          (int)(image_size.w * zoom),
          (int)(image_size.h * zoom)
        );
      }

      DocumentPreferences& docPref = Preferences::instance().document(req.document);
      gfx::Color background = color_utils::color_for_ui(docPref.thumbnails.background());
      int opacity = docPref.thumbnails.opacity();

      base::UniquePtr<doc::Image> thumb_img(doc::Image::create(
        IMAGE_RGB, req.surface_size.w, req.surface_size.h));

      const doc::Sprite* sprite = req.document->sprite();

      base::UniquePtr<doc::Image> converted_rgb;
      const doc::Image* source = req.image;

      if (source->pixelFormat() != IMAGE_RGB) {
        converted_rgb.reset(
          render::convert_pixel_format(
            req.image, NULL, IMAGE_RGB,
            doc::DitheringMethod::NONE,
            sprite->rgbMap(req.frame),
            sprite->palette(req.frame),
            false, req.image->maskColor())
        );
        source = converted_rgb.get();
      }

      double alpha = opacity / 255.0;
      uint8_t bg_a = rgba_geta(background);
      uint8_t bg_r = rgba_getr(background);
      uint8_t bg_g = rgba_getg(background);
      uint8_t bg_b = rgba_getb(background);

      doc::color_t bg = rgba(bg_r, bg_g, bg_b, (int)(bg_a * alpha));
      clear_image(thumb_img.get(), bg);

      double bg_a0 = bg_a / 255.0;
      double bg_r0 = bg_r * bg_a0;
      double bg_g0 = bg_g * bg_a0;
      double bg_b0 = bg_b * bg_a0;

      if (req.image_on_surface.w == image_size.w     && req.image_on_surface.h == image_size.h
       || req.image_on_surface.w >= image_size.w * 2 && req.image_on_surface.h >= image_size.h * 2)
      { // no scale or 2x+ upscale with nearest-neighbor and alpha overlay
        for (int dst_y = 0; dst_y < req.image_on_surface.h; dst_y++) {
          for (int dst_x = 0; dst_x < req.image_on_surface.w; dst_x++) {
            int src_y = (int)(dst_y * image_size.h / (double)req.image_on_surface.h);
            int src_x = (int)(dst_x * image_size.w / (double)req.image_on_surface.w);
            doc::color_t src = source->getPixel(src_x, src_y);
            uint8_t src_r = rgba_getr(src);
            uint8_t src_g = rgba_getg(src);
            uint8_t src_b = rgba_getb(src);
            uint8_t src_a = rgba_geta(src);
            double src_a0 = src_a / 255.0;
            double src_a1 = 1 - src_a0;
            int dst_a = (int)((bg_a * src_a1 + src_a) * alpha);
            doc::color_t dst = rgba(
              (uint8_t)(bg_r0 * src_a1 + src_r * src_a0),
              (uint8_t)(bg_g0 * src_a1 + src_g * src_a0),
              (uint8_t)(bg_b0 * src_a1 + src_b * src_a0),
              (uint8_t)MIN(dst_a, 0xff)
            );
            thumb_img->putPixel(
              req.image_on_surface.x + dst_x,
              req.image_on_surface.y + dst_y,
              dst
            );
          }
        }
      }
      else { // small upscale or downscale with box sampling and alpha overlay
        for (int dst_y = 0; dst_y < req.image_on_surface.h; dst_y++) {
          int dst_x = 0;
          uint32_t* dst = (uint32_t*)thumb_img->getPixelAddress(
            req.image_on_surface.x + dst_x,
            req.image_on_surface.y + dst_y
          );
          uint32_t* dst_end = dst + req.image_on_surface.w;

          while (dst < dst_end) {
            int src_y0 = (int)(dst_y           * image_size.h / (double)req.image_on_surface.h);
            int src_y1 = (int)ceil((dst_y + 1) * image_size.h / (double)req.image_on_surface.h);
            int src_x0 = (int)(dst_x           * image_size.w / (double)req.image_on_surface.w);
            int src_x1 = (int)ceil((dst_x + 1) * image_size.w / (double)req.image_on_surface.w);
            int src_w = src_x1 - src_x0;
            int src_count = (src_y1 - src_y0) * src_w;
            long long src_r = 0, src_g = 0, src_b = 0, src_a = 0;
            for (int src_y = src_y0; src_y < src_y1; src_y++) {
              uint32_t* src = (uint32_t*)source->getPixelAddress(src_x0, src_y);
              uint32_t* src_end = src + src_w;
              while (src < src_end) {
                int a = rgba_geta(*src);
                src_r += rgba_getr(*src) * a;
                src_g += rgba_getg(*src) * a;
                src_b += rgba_getb(*src) * a;
                src_a += a;
                ++src;
              }
            }
            if (src_a) {
              double src_r2 = src_r / (double)src_a;
              double src_g2 = src_g / (double)src_a;
              double src_b2 = src_b / (double)src_a;
              double src_a2 = src_a / (double)src_count;

              double src_a0 = src_a2 / 255.0;
              double src_a1 = 1 - src_a0;
              int dst_a = (int)((bg_a * src_a1 + src_a2) * alpha);
              *dst = rgba(
                (uint8_t)(bg_r0 * src_a1 + src_r2 * src_a0),
                (uint8_t)(bg_g0 * src_a1 + src_g2 * src_a0),
                (uint8_t)(bg_b0 * src_a1 + src_b2 * src_a0),
                (uint8_t)MIN(dst_a, 0xff)
              );
            }
            ++dst;
            ++dst_x;
          }
        }
      }

      she::Surface* thumb_surf = she::instance()->createRgbaSurface(
        thumb_img->width(), thumb_img->height());

      convert_image_to_surface(thumb_img, NULL, thumb_surf,
        0, 0, 0, 0, thumb_img->width(), thumb_img->height());

      return thumb_surf;
    }

    ///////// fetching

    she::Surface* CacheDir::fetch(Request& req)
    {
      // TODO throttle the ammount of requests;
      //      generate a empty surface;
      //      update the pixels later, on a thread;
      //      and notify the requester of the update

      return active() ? m_documents[req.document->id()].fetch(req) : SurfaceData::generate(req);
    }

    she::Surface* DocumentDir::fetch(Request& req)
    {
      if (!m_thumbnailsPrefConn) {
        DocumentPreferences& docPref = Preferences::instance().document(req.document);
        m_thumbnailsPrefConn = docPref.thumbnails.AfterChange.connect(
          base::Bind<void>(&DocumentDir::onThumbnailsPrefChange, this));
      }
      return m_images[req.image->id()].fetch(req);
    }

    she::Surface* ImageDir::fetch(Request& req)
    {
      if (m_version != req.image->version()) {
        m_surfaces.clear();
        m_version = req.image->version();
      }
      Dimension dim = std::make_pair(req.surface_size.w, req.surface_size.h);
      return m_surfaces[dim].fetch(req);
    }

    she::Surface* SurfaceData::fetch(Request& req)
    {
      if (m_surface == nullptr) {
        m_surface = SurfaceData::generate(req);
        m_count++;
      }
      m_tag = req;
      return m_surface;
    }

    ///////// garbage collection

    inline Sequence SurfaceData::count() { return m_count;  }

    bool CacheDir::active()
    {
      Preferences& globalPref = Preferences::instance();
      int high = globalPref.thumbnails.cacheLimit();

      if (high < 0) { // no limits for the cache
        return true;
      }

      if (high == 0) { // disable cache
        if (SurfaceData::count() > 0) {
          m_documents.clear();
        }
        return false;
      }

      if (SurfaceData::count() < high) { // not time to clean it yet
        return true;
      }

      // removing the LRU surfaces from the cache
      RecentlyUsed recentlyUsed;
      traverse(recentlyUsed);

      int low = high * 8 / 10;
      while (recentlyUsed.size() > low) {
        RecentlyUsed::iterator it = recentlyUsed.begin();
        Tag* tag = *it;
        erase(tag);
        recentlyUsed.erase(it);
      }

      TRACE("cache reached %d and reduced to %d items\n", high, low);
      return true;
    }

    ///////// traversing

    void CacheDir::traverse(RecentlyUsed& recentlyUsed)
    {
      for (DocumentMap::iterator documents_it = m_documents.begin();
        documents_it != m_documents.end(); documents_it++) {
        DocumentDir &document = (*documents_it).second;
        document.traverse(recentlyUsed);
      }
    }

    void DocumentDir::traverse(RecentlyUsed& recentlyUsed)
    {
      for (ImageMap::iterator images_it = m_images.begin();
        images_it != m_images.end(); images_it++) {
        ImageDir &image = (*images_it).second;
        image.traverse(recentlyUsed);
      }
    }

    void ImageDir::traverse(RecentlyUsed& recentlyUsed)
    {
      for (SurfaceMap::iterator surfaces_it = m_surfaces.begin();
        surfaces_it != m_surfaces.end(); surfaces_it++) {
        SurfaceData &surface = (*surfaces_it).second;
        surface.traverse(recentlyUsed);
      }
    }

    void SurfaceData::traverse(RecentlyUsed& recentlyUsed)
    {
      recentlyUsed.insert(&m_tag);
    }

    bool TagRefComp::operator()(const Tag* lhs, const Tag* rhs) const
    {
      return lhs->timestamp < rhs->timestamp;
    }

    ///////// erasing

    void CacheDir::erase(const Tag* tag)
    {
      m_documents[tag->document].erase(tag);
    }

    void DocumentDir::erase(const Tag* tag)
    {
      m_images[tag->image].erase(tag);
    }

    void ImageDir::erase(const Tag* tag)
    {
      m_surfaces.erase(tag->dimension);
    }

  } // thumb
} // app
