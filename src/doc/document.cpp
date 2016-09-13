// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/document.h"

#include "base/path.h"
#include "doc/context.h"
#include "doc/sprite.h"

namespace doc {

Document::Document()
  : Object(ObjectType::Document)
  , m_sprites(this)
  , m_ctx(NULL)
{
}

Document::~Document()
{
  removeFromContext();
}

void Document::setContext(Context* ctx)
{
  if (ctx == m_ctx)
    return;

  removeFromContext();

  m_ctx = ctx;
  if (ctx)
    ctx->documents().add(this);

  onContextChanged();
}

int Document::width() const
{
  return sprite()->width();
}

int Document::height() const
{
  return sprite()->height();
}

ColorMode Document::colorMode() const
{
  return (ColorMode)sprite()->pixelFormat();
}

std::string Document::name() const
{
  return base::get_file_name(m_filename);
}

void Document::setFilename(const std::string& filename)
{
  // Normalize the path (if the filename has a path)
  if (!base::get_file_path(filename).empty())
    m_filename = base::normalize_path(filename);
  else
    m_filename = filename;

  notify_observers(&DocumentObserver::onFileNameChanged, this);
}

void Document::close()
{
  removeFromContext();
}

void Document::onContextChanged()
{
  // Do nothing
}

void Document::removeFromContext()
{
  if (m_ctx) {
    m_ctx->documents().remove(this);
    m_ctx = NULL;

    onContextChanged();
  }
}

} // namespace doc
