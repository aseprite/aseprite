// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_DOCUMENT_H_INCLUDED
#define DOC_DOCUMENT_H_INCLUDED
#pragma once

#include <string>

#include "base/unique_ptr.h"
#include "doc/document_observer.h"
#include "doc/object.h"
#include "doc/sprites.h"
#include "obs/observable.h"

namespace doc {

  class Context;

  class Document : public Object
                 , public obs::observable<DocumentObserver> {
  public:
    Document();
    ~Document();

    Context* context() const { return m_ctx; }
    void setContext(Context* ctx);

    const Sprites& sprites() const { return m_sprites; }
    Sprites& sprites() { return m_sprites; }

    const Sprite* sprite() const { return m_sprites.empty() ? NULL: m_sprites.front(); }
    Sprite* sprite() { return m_sprites.empty() ? NULL: m_sprites.front(); }

    int width() const;
    int height() const;
    ColorMode colorMode() const;

    std::string name() const;
    const std::string& filename() const { return m_filename; }
    void setFilename(const std::string& filename);

    void close();

  protected:
    virtual void onContextChanged();

  private:
    void removeFromContext();

    // Document's file name. From where it was loaded, where it is
    // saved.
    std::string m_filename;
    Sprites m_sprites;
    Context* m_ctx;
  };

} // namespace doc

#endif
