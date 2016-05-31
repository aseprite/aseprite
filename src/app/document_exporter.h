// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_DOCUMENT_EXPORTER_H_INCLUDED
#define APP_DOCUMENT_EXPORTER_H_INCLUDED
#pragma once

#include "app/sprite_sheet_type.h"
#include "base/disable_copying.h"
#include "doc/image_buffer.h"
#include "gfx/fwd.h"

#include <iosfwd>
#include <string>
#include <vector>

namespace doc {
  class FrameTag;
  class Image;
  class Layer;
}

namespace app {
  class Document;

  class DocumentExporter {
  public:
    enum DataFormat {
      JsonHashDataFormat,
      JsonArrayDataFormat,
      DefaultDataFormat = JsonHashDataFormat
    };

    DocumentExporter();
    ~DocumentExporter();

    DataFormat dataFormat() const { return m_dataFormat; }
    const std::string& dataFilename() { return m_dataFilename; }
    const std::string& textureFilename() { return m_textureFilename; }
    int textureWidth() const { return m_textureWidth; }
    int textureHeight() const { return m_textureHeight; }
    SpriteSheetType spriteSheetType() { return m_sheetType; }
    bool ignoreEmptyCels() { return m_ignoreEmptyCels; }
    int borderPadding() const { return m_borderPadding; }
    int shapePadding() const { return m_shapePadding; }
    int innerPadding() const { return m_innerPadding; }
    bool trimCels() const { return m_trimCels; }
    const std::string& filenameFormat() const { return m_filenameFormat; }
    bool listFrameTags() const { return m_listFrameTags; }
    bool listLayers() const { return m_listLayers; }

    void setDataFormat(DataFormat format) { m_dataFormat = format; }
    void setDataFilename(const std::string& filename) { m_dataFilename = filename; }
    void setTextureFilename(const std::string& filename) { m_textureFilename = filename; }
    void setTextureWidth(int width) { m_textureWidth = width; }
    void setTextureHeight(int height) { m_textureHeight = height; }
    void setSpriteSheetType(SpriteSheetType type) { m_sheetType = type; }
    void setIgnoreEmptyCels(bool ignore) { m_ignoreEmptyCels = ignore; }
    void setBorderPadding(int padding) { m_borderPadding = padding; }
    void setShapePadding(int padding) { m_shapePadding = padding; }
    void setInnerPadding(int padding) { m_innerPadding = padding; }
    void setTrimCels(bool trim) { m_trimCels = trim; }
    void setFilenameFormat(const std::string& format) { m_filenameFormat = format; }
    void setListFrameTags(bool value) { m_listFrameTags = value; }
    void setListLayers(bool value) { m_listLayers = value; }

    void addDocument(Document* document,
                     doc::Layer* layer = nullptr,
                     doc::FrameTag* tag = nullptr,
                     bool temporalTag = false) {
      m_documents.push_back(Item(document, layer, tag, temporalTag));
    }

    Document* exportSheet();
    gfx::Size calculateSheetSize();

  private:
    class Sample;
    class Samples;
    class LayoutSamples;
    class SimpleLayoutSamples;
    class BestFitLayoutSamples;

    void captureSamples(Samples& samples) const;
    void layoutSamples(Samples& samples);
    gfx::Size calculateSheetSize(const Samples& samples) const;
    Document* createEmptyTexture(const Samples& samples) const;
    void renderTexture(const Samples& samples, doc::Image* textureImage) const;
    void createDataFile(const Samples& samples, std::ostream& os, doc::Image* textureImage) const;
    void renderSample(const Sample& sample, doc::Image* dst, int x, int y) const;

    class Item {
    public:
      Document* doc;
      doc::Layer* layer;
      doc::FrameTag* frameTag;
      bool temporalTag;

      Item(Document* doc,
           doc::Layer* layer,
           doc::FrameTag* frameTag,
           bool temporalTag)
        : doc(doc), layer(layer), frameTag(frameTag)
        , temporalTag(temporalTag) {
      }

      int frames() const;
      int fromFrame() const;
      int toFrame() const;
    };
    typedef std::vector<Item> Items;

    DataFormat m_dataFormat;
    std::string m_dataFilename;
    std::string m_textureFilename;
    int m_textureWidth;
    int m_textureHeight;
    SpriteSheetType m_sheetType;
    bool m_ignoreEmptyCels;
    int m_borderPadding;
    int m_shapePadding;
    int m_innerPadding;
    bool m_trimCels;
    Items m_documents;
    std::string m_filenameFormat;
    doc::ImageBufferPtr m_sampleRenderBuf;
    bool m_listFrameTags;
    bool m_listLayers;

    DISABLE_COPYING(DocumentExporter);
  };

} // namespace app

#endif
