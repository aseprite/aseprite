// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_SPRITE_WRAP_H_INCLUDED
#define APP_SCRIPT_SPRITE_WRAP_H_INCLUDED
#pragma once

#include "doc/object_id.h"

#include <map>

namespace doc {
  class Image;
  class Sprite;
}

namespace app {
  class Doc;
  class DocumentView;
  class ImageWrap;
  class Transaction;

  class SpriteWrap {
    typedef std::map<doc::ObjectId, ImageWrap*> Images;

  public:
    SpriteWrap(Doc* doc);
    ~SpriteWrap();

    void commit();
    void commitImages();
    Transaction& transaction();

    Doc* document();
    doc::Sprite* sprite();
    ImageWrap* activeImage();

    ImageWrap* wrapImage(doc::Image* img);

  private:
    Doc* m_doc;
    app::DocumentView* m_view;
    app::Transaction* m_transaction;

    Images m_images;
  };

} // namespace app

#endif
