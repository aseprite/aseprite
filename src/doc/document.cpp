// Aseprite Document Library
// Copyright (c) 2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/document.h"

#include "doc/export_data.h"

namespace doc {

Document::Document()
{
}

void Document::setFilename(const std::string& filename)
{
  m_filename = filename;
}

void Document::setExportData(const ExportDataPtr& data)
{
  m_exportData = data;
}

} // namespace doc
