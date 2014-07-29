// Aseprite Document Library
// Copyright (c) 2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/document.h"

#include "base/path.h"
#include "doc/context.h"
#include "doc/export_data.h"
#include "doc/sprite.h"

namespace doc {

Document::Document()
  : m_ctx(NULL)
  , m_sprites(this)
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
}

int Document::width() const
{
  return sprite()->getWidth();
}

int Document::height() const
{
  return sprite()->getHeight();
}

ColorMode Document::colorMode() const
{
  return (ColorMode)sprite()->getPixelFormat();
}

std::string Document::name() const
{
  return base::get_file_name(m_filename);
}

void Document::setFilename(const std::string& filename)
{
  m_filename = filename;
  notifyObservers(&DocumentObserver::onFileNameChanged, this);
}

void Document::setExportData(const ExportDataPtr& data)
{
  m_exportData = data;
}

void Document::close()
{
  removeFromContext();
}

void Document::removeFromContext()
{
  if (m_ctx) {
    m_ctx->documents().remove(this);
    m_ctx = NULL;
  }
}

} // namespace doc
