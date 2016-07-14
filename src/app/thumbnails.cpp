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
