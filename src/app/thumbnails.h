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

    typedef std::pair< int, int > Dimension;

    class Request {
    public:
      Request();
      Request(const app::Document* doc, const doc::frame_t frm, const doc::Image* img, const Dimension dim);
      const app::Document* document;
      const doc::frame_t frame;
      const doc::Image* image;
      const Dimension dimension;
    };

    class SurfaceData {
    public:
      static she::Surface* generate(Request& req);
      static she::Surface* fetch(const app::Document* doc, const doc::Cel* cel, const gfx::Rect& bounds);
    };

  } // thumbnails

} // app

#endif
