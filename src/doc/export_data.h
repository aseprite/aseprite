// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

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
      Matrix,
    };

    ExportData() : m_type(None) {
      m_columns = 0;
      m_width = 0;
      m_height = 0;
      m_bestFit = false;
    }

    Type type() const { return m_type; }
    int columns() const { return m_columns; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    bool bestFit() const { return m_bestFit; }
    const std::string& filename() const { return m_filename; }

    void setType(Type type) { m_type = type; }
    void setColumns(int columns) { m_columns = columns; }
    void setWidth(int width) { m_width = width; }
    void setHeight(int height) { m_height = height; }
    void setBestFit(bool bestFit) { m_bestFit = bestFit; }
    void setFilename(const std::string& filename) {
      m_filename = filename;
    }

  private:
    Type m_type;
    int m_columns;
    int m_width;
    int m_height;
    bool m_bestFit;
    std::string m_filename;
  };

  typedef SharedPtr<ExportData> ExportDataPtr;

} // namespace doc

#endif
