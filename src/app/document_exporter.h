// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_DOCUMENT_EXPORTER_H_INCLUDED
#define APP_DOCUMENT_EXPORTER_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/image_buffer.h"
#include "gfx/fwd.h"

#include <iosfwd>
#include <string>
#include <vector>

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

    void setTrimCels(bool trim) {
      m_trimCels = trim;
    }

    void setFilenameFormat(const std::string& format) {
      m_filenameFormat = format;
    }

    void addDocument(Document* document, doc::Layer* layer = NULL) {
      m_documents.push_back(Item(document, layer));
    }

    Document* exportSheet();

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
    void renderSample(const Sample& sample, doc::Image* dst);

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
    bool m_trimCels;
    Items m_documents;
    std::string m_filenameFormat;
    doc::ImageBufferPtr m_sampleRenderBuf;

    DISABLE_COPYING(DocumentExporter);
  };

} // namespace app

#endif
