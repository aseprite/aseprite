// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOC_EXPORTER_H_INCLUDED
#define APP_DOC_EXPORTER_H_INCLUDED
#pragma once

#include "app/sprite_sheet_data_format.h"
#include "app/sprite_sheet_type.h"
#include "base/disable_copying.h"
#include "base/task.h"
#include "doc/frame.h"
#include "doc/image_buffer.h"
#include "doc/object_id.h"
#include "doc/object_version.h"
#include "gfx/fwd.h"
#include "gfx/rect.h"

#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace doc {
  class Image;
  class SelectedFrames;
  class SelectedLayers;
  class Sprite;
  class Tag;
}

namespace app {

  class Context;
  class Doc;

  class DocExporter {
  public:
    DocExporter();

    void reset();
    void setDocImageBuffer(const doc::ImageBufferPtr& docBuf);

    SpriteSheetDataFormat dataFormat() const { return m_dataFormat; }
    const std::string& dataFilename() { return m_dataFilename; }
    const std::string& textureFilename() { return m_textureFilename; }
    SpriteSheetType spriteSheetType() { return m_sheetType; }
    const std::string& filenameFormat() const { return m_filenameFormat; }

    void setDataFormat(SpriteSheetDataFormat format) { m_dataFormat = format; }
    void setDataFilename(const std::string& filename) { m_dataFilename = filename; }
    void setTextureFilename(const std::string& filename) { m_textureFilename = filename; }
    void setTextureWidth(int width) { m_textureWidth = width; }
    void setTextureHeight(int height) { m_textureHeight = height; }
    void setTextureColumns(int columns) { m_textureColumns = columns; }
    void setTextureRows(int rows) { m_textureRows = rows; }
    void setSpriteSheetType(SpriteSheetType type) { m_sheetType = type; }
    void setIgnoreEmptyCels(bool ignore) { m_ignoreEmptyCels = ignore; }
    void setMergeDuplicates(bool merge) { m_mergeDuplicates = merge; }
    void setBorderPadding(int padding) { m_borderPadding = padding; }
    void setShapePadding(int padding) { m_shapePadding = padding; }
    void setInnerPadding(int padding) { m_innerPadding = padding; }
    void setTrimSprite(bool trim) { m_trimSprite = trim; }
    void setTrimCels(bool trim) { m_trimCels = trim; }
    void setTrimByGrid(bool trimByGrid) { m_trimByGrid = trimByGrid; }
    void setExtrude(bool extrude) { m_extrude = extrude; }
    void setFilenameFormat(const std::string& format) { m_filenameFormat = format; }
    void setSplitLayers(bool splitLayers) { m_splitLayers = splitLayers; }
    void setSplitTags(bool splitTags) { m_splitTags = splitTags; }
    void setListTags(bool value) { m_listTags = value; }
    void setListLayers(bool value) { m_listLayers = value; }
    void setListSlices(bool value) { m_listSlices = value; }

    void addDocument(
      Doc* doc,
      const doc::Tag* tag,
      const doc::SelectedLayers* selLayers,
      const doc::SelectedFrames* selFrames);

    int addDocumentSamples(
      Doc* doc,
      const doc::Tag* tag,
      const bool splitLayers,
      const bool splitTags,
      const doc::SelectedLayers* selLayers,
      const doc::SelectedFrames* selFrames);

    Doc* exportSheet(Context* ctx, base::task_token& token);
    gfx::Size calculateSheetSize();

  private:
    class Sample;
    class Samples;
    class LayoutSamples;
    class SimpleLayoutSamples;
    class BestFitLayoutSamples;

    void captureSamples(Samples& samples,
                        base::task_token& token);
    void layoutSamples(Samples& samples,
                       base::task_token& token);
    gfx::Size calculateSheetSize(const Samples& samples,
                                 base::task_token& token) const;
    Doc* createEmptyTexture(const Samples& samples,
                            base::task_token& token) const;
    void renderTexture(Context* ctx,
                       const Samples& samples,
                       doc::Image* textureImage,
                       base::task_token& token) const;
    void trimTexture(const Samples& samples, doc::Sprite* texture) const;
    void createDataFile(const Samples& samples, std::ostream& os, doc::Sprite* texture);

    class Item {
    public:
      Doc* doc;
      const doc::Tag* tag;
      doc::SelectedLayers* selLayers;
      doc::SelectedFrames* selFrames;

      Item(Doc* doc,
           const doc::Tag* tag,
           const doc::SelectedLayers* selLayers,
           const doc::SelectedFrames* selFrames);
      Item(Item&& other);
      ~Item();

      Item() = delete;
      Item(const Item&) = delete;
      Item& operator=(const Item&) = delete;

      int frames() const;
      doc::SelectedFrames getSelectedFrames() const;
    };
    typedef std::vector<Item> Items;

    SpriteSheetType m_sheetType;
    SpriteSheetDataFormat m_dataFormat;
    std::string m_dataFilename;
    std::string m_textureFilename;
    std::string m_filenameFormat;
    int m_textureWidth;
    int m_textureHeight;
    int m_textureColumns;
    int m_textureRows;
    int m_borderPadding;
    int m_shapePadding;
    int m_innerPadding;
    bool m_ignoreEmptyCels;
    bool m_mergeDuplicates;
    bool m_trimSprite;
    bool m_trimCels;
    bool m_trimByGrid;
    bool m_extrude;
    bool m_splitLayers;
    bool m_splitTags;
    bool m_listTags;
    bool m_listLayers;
    bool m_listSlices;
    Items m_documents;

    // Displacement for each tag from/to frames in case we export
    // them. It's used in case we trim frames outside tags and they
    // will not be exported at all in the final result.
    std::map<doc::ObjectId, std::pair<int, int> > m_tagDelta;

    // Buffers used
    doc::ImageBufferPtr m_docBuf;
    doc::ImageBufferPtr m_sampleBuf;

    // Trimmed bounds of a specific sprite (to avoid recalculating
    // this)
    struct Cache {
      doc::ObjectId spriteId;
      doc::ObjectVersion spriteVer;
      gfx::Rect trimmedBounds;
      bool trimmedByGrid;
    } m_cache;

    DISABLE_COPYING(DocExporter);
  };

} // namespace app

#endif
