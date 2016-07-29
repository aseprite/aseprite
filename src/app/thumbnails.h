// Aseprite
// Copyright (C) 2016  Carlo Caputo
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_THUMBNAILS_H_INCLUDED
#define APP_THUMBNAILS_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"
#include "base/connection.h"
#include "she/surface.h"
#include "doc/object_id.h"
#include "doc/object.h"
#include "doc/frame.h"

#include <map>
#include <set>
#include <utility>

namespace doc {
  class Cel;
  class Image;
  class Document;
}

namespace app {
  class Document;

  namespace thumb {

    typedef long long Sequence;

    typedef std::pair< int, int > Dimension;

    class Request {
    public:
      Request();
      Request(const app::Document* doc, const doc::frame_t frm, const doc::Image* img, const Dimension dim);
      const app::Document* document;
      const doc::frame_t frame;
      const doc::Image* image;
      const Dimension dimension;
      const Sequence timestamp;
      bool updated;
    private:
      static Sequence m_sequence;
    };

    class Tag {
    public:
      Tag();
      Tag(const Request& req);
      doc::ObjectId document;
      doc::ObjectId image;
      Dimension dimension;
      Sequence timestamp;
    };

    struct TagRefComp
    {
      bool operator()(const Tag* lhs, const Tag* rhs) const;
    };
    typedef std::set< Tag*, TagRefComp > RecentlyUsed;

    class SurfaceData {
    public:
      SurfaceData();
      ~SurfaceData(); // dispose
      static inline Sequence count();
      static she::Surface* generate(Request& req);
      she::Surface* fetch(Request& req);
      void traverse(RecentlyUsed& recentlyUsed);
      void erase(const Tag* tag);

    private:
      Tag m_tag;
      she::Surface* m_surface;
      static Sequence m_count;
    };

    typedef std::map< Dimension, SurfaceData > SurfaceMap;

    class ImageDir {
    public:
      ImageDir();
      she::Surface* fetch(Request& req);
      void traverse(RecentlyUsed& recentlyUsed);
      void erase(const Tag* tag);

    private:
      doc::ObjectVersion m_version;
      SurfaceMap m_surfaces;
    };

    typedef std::map< doc::ObjectId, ImageDir > ImageMap;

    class DocumentDir {
    public:
      DocumentDir(); // constructor must be default
      ~DocumentDir(); // disconnect
      she::Surface* fetch(Request& req); // init here
      void traverse(RecentlyUsed& recentlyUsed);
      base::Connection m_thumbnailsPrefConn;
      void onThumbnailsPrefChange();
      void erase(const Tag* tag);

    private:
      ImageMap m_images;
    };

    typedef std::map< doc::ObjectId, DocumentDir > DocumentMap;

    class CacheDir {
    public:
      CacheDir();
      void onRemoveDocument(doc::Document* doc);
      she::Surface* fetch(Request& req);
      she::Surface* fetch(const app::Document* doc, const doc::Cel* cel, const gfx::Rect& bounds);
      void traverse(RecentlyUsed& recentlyUsed);
      void erase(const Tag* tag);


    private:
      bool active(); // check if should cache and trim if necessary

      thumb::DocumentMap m_documents;
    };

  } // thumbnails

} // app

#endif
