// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOC_EXPORTER_H_INCLUDED
#define APP_DOC_EXPORTER_H_INCLUDED
#pragma once

#include "app/sprite_sheet_type.h"
#include "base/disable_copying.h"
#include "doc/frame.h"
#include "doc/image_buffer.h"
#include "doc/object_id.h"
#include "gfx/fwd.h"

#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace doc {
  class FrameTag;
  class Image;
  class SelectedLayers;
  class SelectedFrames;
}

namespace app {

  class Context;
  class Doc;

  class DocExporter {
  public:
    enum DataFormat {
      JsonHashDataFormat,
      JsonArrayDataFormat,
      DefaultDataFormat = JsonHashDataFormat
    };

    DocExporter();

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
    bool trimByGrid() const { return m_trimByGrid; }
    bool extrude() const { return m_extrude; }
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
    void setTrimByGrid(bool trimByGrid) { m_trimByGrid = trimByGrid; }
    void setExtrude(bool extrude) { m_extrude = extrude; }
    void setFilenameFormat(const std::string& format) { m_filenameFormat = format; }
    void setListFrameTags(bool value) { m_listFrameTags = value; }
    void setListLayers(bool value) { m_listLayers = value; }
    void setListSlices(bool value) { m_listSlices = value; }

    void addDocument(Doc* document,
                     doc::FrameTag* tag,
                     doc::SelectedLayers* selLayers,
                     doc::SelectedFrames* selFrames) {
      m_documents.push_back(Item(document, tag, selLayers, selFrames));
    }

    Doc* exportSheet(Context* ctx);
    gfx::Size calculateSheetSize();

  private:
    class Sample;
    class Samples;
    class LayoutSamples;
    class SimpleLayoutSamples;
    class BestFitLayoutSamples;

    void captureSamples(Samples& samples);
    void layoutSamples(Samples& samples);
    gfx::Size calculateSheetSize(const Samples& samples) const;
    Doc* createEmptyTexture(const Samples& samples) const;
    void renderTexture(Context* ctx, const Samples& samples, doc::Image* textureImage) const;
    void createDataFile(const Samples& samples, std::ostream& os, doc::Image* textureImage);
    void renderSample(const Sample& sample, doc::Image* dst, int x, int y, bool extrude) const;

    class Item {
    public:
      Doc* doc;
      doc::FrameTag* frameTag;
      doc::SelectedLayers* selLayers;
      doc::SelectedFrames* selFrames;

      Item(Doc* doc,
           doc::FrameTag* frameTag,
           doc::SelectedLayers* selLayers,
           doc::SelectedFrames* selFrames);
      Item(Item&& other);
      ~Item();

      Item() = delete;
      Item(const Item&) = delete;
      Item& operator=(const Item&) = delete;

      int frames() const;
      doc::SelectedFrames getSelectedFrames() const;
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
    bool m_trimByGrid;
    bool m_extrude;
    Items m_documents;
    std::string m_filenameFormat;
    doc::ImageBufferPtr m_sampleRenderBuf;
    bool m_listFrameTags;
    bool m_listLayers;
    bool m_listSlices;

    // Displacement for each tag from/to frames in case we export
    // them. It's used in case we trim frames outside tags and they
    // will not be exported at all in the final result.
    std::map<doc::ObjectId, std::pair<int, int> > m_tagDelta;

    DISABLE_COPYING(DocExporter);
  };

} // namespace app

#endif
