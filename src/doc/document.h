// Aseprite Document Library
// Copyright (c) 2014 David Capello
//
// This source file is distributed under the terms of the MIT license,
// please read LICENSE.txt for more information.

#ifndef DOC_DOCUMENT_H_INCLUDED
#define DOC_DOCUMENT_H_INCLUDED
#pragma once

#include <string>

#include "doc/object.h"
#include "doc/export_data.h"

namespace doc {

  class ExportData;

  class Document : public Object {
  public:
    Document();

    const std::string& filename() const { return m_filename; }
    ExportDataPtr exportData() const { return m_exportData; }

    void setFilename(const std::string& filename);
    void setExportData(const ExportDataPtr& data);

  private:
    // Document's file name. From where it was loaded, where it is
    // saved.
    std::string m_filename;

    ExportDataPtr m_exportData;
  };

} // namespace doc

#endif
