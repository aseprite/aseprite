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
      dimension(0, 0),
      timestamp(-1),
      updated(false)
    {}

    Tag::Tag() :
      document(0),
      image(0),
      dimension(0, 0),
      timestamp(-1)
    {}

    Request::Request(const app::Document* doc, const doc::frame_t frm, const doc::Image* img, const Dimension dim) :
      document(doc),
      frame(frm),
      image(img),
      dimension(dim),
      timestamp(m_sequence++),
      updated(false)
    {}

    Tag::Tag(const Request& req) :
      document(req.document->id()),
      image(req.image->id()),
      dimension(req.dimension),
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
      DocumentPreferences& docPref = Preferences::instance().document(req.document);

      int opacity = docPref.thumbnails.opacity();
      gfx::Color background = color_utils::color_for_ui(docPref.thumbnails.background());

      gfx::Size image_size = req.image->size();
      gfx::Size thumb_size = gfx::Size(req.dimension.first, req.dimension.second);

      double zw = thumb_size.w / (double)image_size.w;
      double zh = thumb_size.h / (double)image_size.h;
      double zoom = MIN(1, MIN(zw, zh));

      gfx::Rect cel_image_on_thumb(
        (int)(thumb_size.w * 0.5 - image_size.w  * zoom * 0.5),
        (int)(thumb_size.h * 0.5 - image_size.h * zoom * 0.5),
        (int)(image_size.w * zoom),
        (int)(image_size.h * zoom)
      );

      const doc::Sprite* sprite = req.document->sprite();

      base::UniquePtr<doc::Image> thumb_img(doc::Image::create(
        IMAGE_RGB, thumb_size.w, thumb_size.h));

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

      clear_image(thumb_img.get(), rgba(
        rgba_getr(background),
        rgba_getg(background),
        rgba_getb(background),
        rgba_geta(background) * alpha
     ));

      uint8_t bg_a = rgba_geta(background);
      double bg_a0 = bg_a / 255.0;
      double bg_r0 = rgba_getr(background) * bg_a0;
      double bg_g0 = rgba_getg(background) * bg_a0;
      double bg_b0 = rgba_getb(background) * bg_a0;

      if (zoom == 1) {
        for (int y = 0; y < cel_image_on_thumb.h; y++) {

          uint32_t* dst = (uint32_t*)thumb_img->getPixelAddress(cel_image_on_thumb.x, cel_image_on_thumb.y + y);
          uint32_t* src = (uint32_t*)source->getPixelAddress(0, y);
          uint32_t* src_end  = src + cel_image_on_thumb.w;

          while (src < src_end) {
            uint8_t src_a = rgba_geta(*src);
            double src_a0 = src_a / 255.0, src_a1 = 1 - src_a0;
            uint32_t dst_a = (bg_a * src_a1 + src_a) * alpha;
            *(dst++) = rgba(
              bg_r0 * src_a1 + rgba_getr(*src) * src_a0,
              bg_g0 * src_a1 + rgba_getg(*src) * src_a0,
              bg_b0 * src_a1 + rgba_getb(*src) * src_a0,
              MIN(dst_a, 0xff)
            );
            ++src;
          }
        }
      }
      else { // downscale with box sampling
        for (int dst_y = 0; dst_y < cel_image_on_thumb.h; dst_y++) {
          int dst_x = 0;
          uint32_t* dst = (uint32_t*)thumb_img->getPixelAddress(
            cel_image_on_thumb.x + dst_x, 
            cel_image_on_thumb.y + dst_y
          );
          uint32_t* dst_end = dst + cel_image_on_thumb.w;

          while (dst < dst_end) {
            double src_r = 0, src_g = 0, src_b = 0, src_a = 0;
            int src_y0 =  dst_y      * image_size.h / cel_image_on_thumb.h;
            int src_y1 = (dst_y + 1) * image_size.h / cel_image_on_thumb.h;
            int src_x0 =  dst_x      * image_size.w / cel_image_on_thumb.w;
            int src_w  =               image_size.w / cel_image_on_thumb.w;
            int sn = (src_y1 - src_y0) * src_w;
            for (int src_y = src_y0; src_y < src_y1; src_y++) {
              uint32_t* src = (uint32_t*)source->getPixelAddress(src_x0, src_y);
              uint32_t* src_end = src + src_w;
              while (src < src_end) {
                src_r += rgba_getr(*src);
                src_g += rgba_getg(*src);
                src_b += rgba_getb(*src);
                src_a += rgba_geta(*src);
                ++src;
              }
            }
            src_r /= sn;
            src_g /= sn;
            src_b /= sn;
            src_a /= sn;
            double src_a0 = src_a / 255.0, src_a1 = 1 - src_a0;
            uint32_t dst_a = (bg_a * src_a1 + src_a) * alpha;
            *(dst++) = rgba(
              bg_r0 * src_a1 + src_r * src_a0,
              bg_g0 * src_a1 + src_g * src_a0,
              bg_b0 * src_a1 + src_b * src_a0,
              MIN(dst_a, 0xff)
            );
            ++dst_x;
          }
        }
      }

      she::Surface* thumb_surf = she::instance()->createRgbaSurface(
        thumb_img->width(), thumb_img->height());

      convert_image_to_surface(thumb_img, NULL, thumb_surf,
        0, 0, 0, 0, thumb_img->width(), thumb_img->height());

      req.updated = true;
      return thumb_surf;
    }

    ///////// fetching

    she::Surface* CacheDir::fetch(const app::Document* doc, const doc::Cel* cel, const gfx::Rect& bounds)
    {
      const Dimension dim = std::make_pair(bounds.w, bounds.h);
      Request req(doc, cel->frame(), cel->image(), dim);
      return fetch(req);
    }

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
      return m_surfaces[req.dimension].fetch(req);
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

    ///////// trimming

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
