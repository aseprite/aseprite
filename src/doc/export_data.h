// Aseprite Document Library
// Copyright (c) 2014 David Capello
//
// This source file is distributed under the terms of the MIT license,
// please read LICENSE.txt for more information.

#ifndef DOC_EXPORT_DATA_H_INCLUDED
#define DOC_EXPORT_DATA_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"
#include "gfx/rect.h"
#include <string>

namespace doc {

  class ExportData {
  public:
    enum Type {
      None,
      HorizontalStrip,
      VerticalStrip,
      Matrix
    };

    ExportData() : m_type(None) {
    }

    Type type() const { return m_type; }
    int columns() const { return m_columns; }
    const std::string& filename() const { return m_filename; }

    void setType(Type type) { m_type = type; }
    void setColumns(int columns) { m_columns = columns; }
    void setFilename(const std::string& filename) {
      m_filename = filename;
    }

  private:
    Type m_type;
    int m_columns;
    std::string m_filename;
  };

  typedef SharedPtr<ExportData> ExportDataPtr;

} // namespace doc

#endif
