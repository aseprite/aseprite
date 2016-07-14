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
      const Dimension dimension;
      const app::Document* document;
      const doc::Image* image;
      const doc::frame_t frame;
      const Sequence timestamp;
      bool updated;
    private:
      static Sequence m_sequence;
    };

    class Tag {
    public:
      Tag();
      Tag(const Request& req);
      Dimension dimension;
      doc::ObjectId document;
      doc::ObjectId image;
      Sequence timestamp;
    };

    struct TagRefComp
    {
      bool operator()(const Tag* lhs, const Tag* rhs) const;
    };
    typedef std::set< Tag*, TagRefComp > RecentlyUsed;

    class Tier {
      virtual she::Surface* fetch(Request& req) = 0;
      virtual void traverse(RecentlyUsed& recentlyUsed) = 0;
    };

    class Data : Tier {
      // static inline Sequence count() = 0;
      // static she::Surface* generate(Request& req) = 0;
    };

    class Directory : Tier {
      virtual void erase(const Tag* tag) = 0;
    };


    class Surface : Data {
    public:
      Surface();
      ~Surface(); // dispose
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

    //    typedef base::SharedPtr<Surface> SurfaceRef;
    typedef std::map< Dimension, Surface > SurfaceMap;

    class Image : Directory {
    public:
      Image();
      she::Surface* fetch(Request& req);
      void traverse(RecentlyUsed& recentlyUsed);
      void erase(const Tag* tag);

    private:
      doc::ObjectVersion m_version;
      SurfaceMap m_surfaces;
    };

    //    typedef base::SharedPtr<Image> ImageRef;
    typedef std::map< doc::ObjectId, Image > ImageMap;

    class Document : Directory {
    public:
      Document(); // constructor must be default
      ~Document(); // disconnect
      she::Surface* fetch(Request& req); // init here
      void traverse(RecentlyUsed& recentlyUsed);
      base::Connection m_thumbnailsPrefConn;
      void onThumbnailsPrefChange();
      void erase(const Tag* tag);

    private:
      ImageMap m_images;
    };

    //    typedef base::SharedPtr<Document> DocumentRef;
    typedef std::map< doc::ObjectId, Document > DocumentMap;

    class Cache : Directory {
    public:
      Cache();
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
