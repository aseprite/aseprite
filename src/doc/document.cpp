// Aseprite Document Library
// Copyright (c) 2014 David Capello
//
// This source file is distributed under the terms of the MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/document.h"

namespace doc {

Document::Document()
{
}

void Document::setFilename(const std::string& filename)
{
  m_filename = filename;
}

} // namespace doc
