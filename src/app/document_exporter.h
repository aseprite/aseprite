/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_DOCUMENT_EXPORTER_H_INCLUDED
#define APP_DOCUMENT_EXPORTER_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "gfx/fwd.h"

#include <iosfwd>
#include <vector>
#include <string>

namespace doc {
  class Image;
  class Layer;
}

namespace app {
  class Document;

  class DocumentExporter {
  public:
    enum DataFormat {
      JsonDataFormat,
      DefaultDataFormat = JsonDataFormat
    };

    enum TextureFormat {
      JsonTextureFormat,
      DefaultTextureFormat = JsonTextureFormat
    };

    enum ScaleMode {
      DefaultScaleMode
    };

    DocumentExporter();

    void setDataFormat(DataFormat format) {
      m_dataFormat = format;
    }

    void setDataFilename(const std::string& filename) {
      m_dataFilename = filename;
    }

    void setTextureFormat(TextureFormat format) {
      m_textureFormat = format;
    }

    void setTextureFilename(const std::string& filename) {
      m_textureFilename = filename;
    }

    void setTextureWidth(int width) {
      m_textureWidth = width;
    }

    void setTextureHeight(int height) {
      m_textureHeight = height;
    }

    void setTexturePack(bool state) {
      m_texturePack = state;
    }

    void setScale(double scale) {
      m_scale = scale;
    }

    void setScaleMode(ScaleMode mode) {
      m_scaleMode = mode;
    }

    void setIgnoreEmptyCels(bool ignore) {
      m_ignoreEmptyCels = ignore;
    }

    void setFilenameFormat(const std::string& format) {
      m_filenameFormat = format;
    }

    void addDocument(Document* document, doc::Layer* layer = NULL) {
      m_documents.push_back(Item(document, layer));
    }

    void exportSheet();

  private:
    class Sample;
    class Samples;
    class LayoutSamples;
    class SimpleLayoutSamples;
    class BestFitLayoutSamples;

    void captureSamples(Samples& samples);
    Document* createEmptyTexture(const Samples& samples);
    void renderTexture(const Samples& samples, doc::Image* textureImage);
    void createDataFile(const Samples& samples, std::ostream& os, doc::Image* textureImage);
    void renderSample(const Sample& sample, doc::Image* dst, int x, int y);

    class Item {
    public:
      Document* doc;
      doc::Layer* layer;
      Item(Document* doc, doc::Layer* layer)
        : doc(doc), layer(layer) {
      }
    };
    typedef std::vector<Item> Items;

    DataFormat m_dataFormat;
    std::string m_dataFilename;
    TextureFormat m_textureFormat;
    std::string m_textureFilename;
    int m_textureWidth;
    int m_textureHeight;
    bool m_texturePack;
    double m_scale;
    ScaleMode m_scaleMode;
    bool m_ignoreEmptyCels;
    Items m_documents;
    std::string m_filenameFormat;

    DISABLE_COPYING(DocumentExporter);
  };

} // namespace app

#endif
