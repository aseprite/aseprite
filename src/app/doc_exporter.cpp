// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/doc_exporter.h"

#include "app/cmd/set_pixel_format.h"
#include "app/console.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/filename_formatter.h"
#include "app/restore_visible_layers.h"
#include "app/snap_to_grid.h"
#include "app/util/autocrop.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "base/replace_string.h"
#include "base/string.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/images_map.h"
#include "doc/images_map.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/selected_frames.h"
#include "doc/selected_layers.h"
#include "doc/slice.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "gfx/packing_rects.h"
#include "gfx/rect_io.h"
#include "gfx/size.h"
#include "render/dithering.h"
#include "render/ordered_dither.h"
#include "render/render.h"
#include "ver/info.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <vector>

#define DX_TRACE(...) // TRACEARGS

using namespace doc;

namespace {

std::string escape_for_json(const std::string& path)
{
  std::string res = path;
  base::replace_string(res, "\\", "\\\\");
  base::replace_string(res, "\"", "\\\"");
  return res;
}

std::ostream& operator<<(std::ostream& os, const doc::UserData& data)
{
  doc::color_t color = data.color();
  if (doc::rgba_geta(color)) {
    os << ", \"color\": \"#"
       << std::hex << std::setfill('0')
       << std::setw(2) << (int)doc::rgba_getr(color)
       << std::setw(2) << (int)doc::rgba_getg(color)
       << std::setw(2) << (int)doc::rgba_getb(color)
       << std::setw(2) << (int)doc::rgba_geta(color)
       << std::dec
       << "\"";
  }
  if (!data.text().empty())
    os << ", \"data\": \"" << escape_for_json(data.text()) << "\"";
  return os;
}

} // anonymous namespace

namespace app {

typedef std::shared_ptr<gfx::Rect> SharedRectPtr;

DocExporter::Item::Item(Doc* doc,
                        const doc::Tag* tag,
                        const doc::SelectedLayers* selLayers,
                        const doc::SelectedFrames* selFrames,
                        const bool splitGrid)
  : doc(doc)
  , tag(tag)
  , selLayers(selLayers ? std::make_unique<doc::SelectedLayers>(*selLayers): nullptr)
  , selFrames(selFrames ? std::make_unique<doc::SelectedFrames>(*selFrames): nullptr)
  , splitGrid(splitGrid)
{
}

DocExporter::Item::Item(Doc* doc,
                        const doc::ImageRef& image)
  : doc(doc)
  , image(image)
{
}

DocExporter::Item::Item(Item&& other) = default;
DocExporter::Item::~Item() = default;

int DocExporter::Item::frames() const
{
  if (selFrames)
    return selFrames->size();
  else if (tag) {
    int result = tag->toFrame() - tag->fromFrame() + 1;
    return std::clamp(result, 1, doc->sprite()->totalFrames());
  }
  else
    return doc->sprite()->totalFrames();
}

doc::SelectedFrames DocExporter::Item::getSelectedFrames() const
{
  if (selFrames)
    return *selFrames;

  doc::SelectedFrames frames;
  if (tag) {
    frames.insert(std::clamp(tag->fromFrame(), 0, doc->sprite()->lastFrame()),
                  std::clamp(tag->toFrame(), 0, doc->sprite()->lastFrame()));
  }
  else if (isOneImageOnly())
    frames.insert(0);
  else {
    frames.insert(0, doc->sprite()->lastFrame());
  }
  return frames;
}

class DocExporter::Sample {
public:
  Sample(const gfx::Size& size,
         Doc* document,
         Sprite* sprite,
         const ImageRef& image,
         SelectedLayers* selLayers,
         frame_t frame,
         const Tag* tag,
         const std::string& filename,
         const int innerPadding,
         const bool extrude) :
    m_document(document),
    m_sprite(sprite),
    m_image(image),
    m_selLayers(selLayers),
    m_frame(frame),
    m_tag(tag),
    m_filename(filename),
    m_innerPadding(innerPadding),
    m_extrude(extrude),
    m_isLinked(false),
    m_isDuplicated(false),
    m_originalSize(size),
    m_trimmedBounds(size),
    m_inTextureBounds(std::make_shared<gfx::Rect>(size)) {
  }

  Doc* document() const { return m_document; }
  Sprite* sprite() const { return m_sprite; }
  Layer* layer() const {
    return (m_selLayers && m_selLayers->size() == 1 ? *m_selLayers->begin():
                                                      nullptr);
  }
  const Tag* tag() const { return m_tag; }
  SelectedLayers* selectedLayers() const { return m_selLayers; }
  frame_t frame() const { return m_frame; }
  std::string filename() const { return m_filename; }
  const gfx::Size& originalSize() const { return m_originalSize; }
  const gfx::Rect& trimmedBounds() const { return m_trimmedBounds; }
  const gfx::Rect& inTextureBounds() const { return *m_inTextureBounds; }
  const SharedRectPtr& sharedBounds() const { return m_inTextureBounds; }

  gfx::Size requiredSize() const {
    // if extrude option is enabled, an extra pixel is needed for each side
    // left+right borders and top+bottom borders
    int extraExtrudePixels = m_extrude ? 2 : 0;
    gfx::Size size = m_trimmedBounds.size();
    size.w += 2*m_innerPadding + extraExtrudePixels;
    size.h += 2*m_innerPadding + extraExtrudePixels;
    return size;
  }

  bool trimmed() const {
    return (m_trimmedBounds.x > 0 ||
            m_trimmedBounds.y > 0 ||
            m_trimmedBounds.w != m_originalSize.w ||
            m_trimmedBounds.h != m_originalSize.h);
  }

  void setTrimmedBounds(const gfx::Rect& bounds) {
    // TODO we cannot assign an empty rectangle (samples that are
    // completely trimmed out should be included as a sample of size 1x1)
    ASSERT(!bounds.isEmpty());
    m_trimmedBounds = bounds;
  }

  void setInTextureBounds(const gfx::Rect& bounds) {
    ASSERT(!bounds.isEmpty());
    *m_inTextureBounds = bounds;
  }

  void setSharedBounds(const SharedRectPtr& bounds) {
    m_inTextureBounds = bounds;
  }

  bool isLinked() const { return m_isLinked; }
  bool isDuplicated() const { return m_isDuplicated; }
  bool isEmpty() const {
    // TODO trimmed bounds cannot be empty now (samples that are
    // completely trimmed out are included as a sample of size 1x1)
    ASSERT(!m_trimmedBounds.isEmpty());
    return m_trimmedBounds.isEmpty();
  }

  void setLinked() { m_isLinked = true; }
  void setDuplicated() { m_isDuplicated = true; }

  ImageRef createRender(ImageBufferPtr& imageBuf) {
    ASSERT(m_sprite);

    // We use the m_image as it is, it doesn't require a special
    // render.
    if (m_image)
      return m_image;

    ImageRef render(
      Image::create(m_sprite->pixelFormat(),
                    m_trimmedBounds.w,
                    m_trimmedBounds.h,
                    imageBuf));
    render->setMaskColor(m_sprite->transparentColor());
    clear_image(render.get(), m_sprite->transparentColor());
    renderSample(render.get(), 0, 0, false);
    return render;
  }

  void renderSample(doc::Image* dst, int x, int y, bool extrude) const {
    RestoreVisibleLayers layersVisibility;
    if (m_selLayers)
      layersVisibility.showSelectedLayers(m_sprite,
                                          *m_selLayers);

    render::Render render;

    // 1) We cannot use the Preferences because this is called from a non-UI thread
    // 2) We should use the new blend mode always when we're saving files
    //render.setNewBlend(Preferences::instance().experimental.newBlend());

    if (extrude) {
      const gfx::Rect& trim = m_trimmedBounds;

      // Displaced position onto the destination texture
      int dx[] = { 0, 1, trim.w+1 };
      int dy[] = { 0, 1, trim.h+1 };

      // Starting point of the area to be copied from the original image
      // taking into account the size of the trimmed sprite
      int srcx[] = { trim.x, trim.x, trim.x2()-1 };
      int srcy[] = { trim.y, trim.y, trim.y2()-1 };

      // Size of the area to be copied from original image, starting at
      // the point (srcx[i], srxy[j])
      int szx[] = { 1, trim.w, 1 };
      int szy[] = { 1, trim.h, 1 };

      // Render a 9-patch image extruding the sample one pixel on each
      // side.
      for (int j=0; j<3; ++j) {
        for (int i=0; i<3; ++i) {
          gfx::Clip clip(x+dx[i], y+dy[j], gfx::RectT<int>(srcx[i], srcy[j], szx[i], szy[j]));
          if (m_image) {
            dst->copy(m_image.get(), clip);
          }
          else {
            render.renderSprite(dst, m_sprite, m_frame, clip);
          }
        }
      }
    }
    else {
      gfx::Clip clip(x, y, m_trimmedBounds);
      if (m_image) {
        dst->copy(m_image.get(), clip);
      }
      else {
        render.renderSprite(dst, m_sprite, m_frame, clip);
      }
    }
  }

private:
  Doc* m_document;
  Sprite* m_sprite;
  // In case that this Sample references just one image to export
  // (e.g. like a Tileset tile image) this can be != nullptr.
  ImageRef m_image;
  SelectedLayers* m_selLayers;
  frame_t m_frame;
  const Tag* m_tag;
  std::string m_filename;
  int m_innerPadding;
  bool m_extrude;
  bool m_isLinked;
  bool m_isDuplicated;
  gfx::Size m_originalSize;
  gfx::Rect m_trimmedBounds;
  SharedRectPtr m_inTextureBounds;
};

class DocExporter::Samples {
public:
  typedef std::vector<Sample> List;
  typedef List::iterator iterator;
  typedef List::const_iterator const_iterator;

  bool empty() const { return m_samples.empty(); }
  int size() const { return int(m_samples.size()); }

  void addSample(const Sample& sample) {
    m_samples.push_back(sample);
  }

  const Sample& operator[](const size_t i) const {
    return m_samples[i];
  }

  iterator begin() { return m_samples.begin(); }
  iterator end() { return m_samples.end(); }
  const_iterator begin() const { return m_samples.begin(); }
  const_iterator end() const { return m_samples.end(); }

private:
  List m_samples;
};

class DocExporter::LayoutSamples {
public:
  virtual ~LayoutSamples() { }
  virtual void layoutSamples(Samples& samples,
                             int borderPadding,
                             int shapePadding,
                             int& width, int& height,
                             base::task_token& token) = 0;
};

class DocExporter::SimpleLayoutSamples : public DocExporter::LayoutSamples {
public:
  SimpleLayoutSamples(SpriteSheetType type,
                      int maxCols, int maxRows,
                      bool splitLayers, bool splitTags,
                      bool mergeDups)
    : m_type(type)
    , m_maxCols(maxCols)
    , m_maxRows(maxRows)
    , m_splitLayers(splitLayers)
    , m_splitTags(splitTags)
    , m_mergeDups(mergeDups) {
  }

  void layoutSamples(Samples& samples,
                     int borderPadding,
                     int shapePadding,
                     int& width, int& height,
                     base::task_token& token) override {
    DX_TRACE("DX: SimpleLayoutSamples type", (int)m_type, width, height);

    const bool breakBands =
      (m_type == SpriteSheetType::Columns ||
       m_type == SpriteSheetType::Rows);

    const Sprite* oldSprite = nullptr;
    const Layer* oldLayer = nullptr;
    const Tag* oldTag = nullptr;

    doc::ImagesMap duplicates;
    gfx::Point framePt(borderPadding, borderPadding);
    gfx::Size rowSize(0, 0);

    int i = 0;
    int itemInBand = 0;
    int itemsPerBand = -1;
    if (breakBands) {
      if (m_type == SpriteSheetType::Columns && m_maxRows > 0)
        itemsPerBand = m_maxRows;
      if (m_type == SpriteSheetType::Rows && m_maxCols > 0)
        itemsPerBand = m_maxCols;
    }

    for (auto& sample : samples) {
      if (token.canceled())
        return;
      token.set_progress(0.2f + 0.2f * i / samples.size());

      if (sample.isEmpty()) {
        sample.setInTextureBounds(gfx::Rect(0, 0, 0, 0));
        ++i;
        continue;
      }

      if (m_mergeDups || sample.isLinked()) {
        doc::ImageBufferPtr sampleBuf = std::make_shared<doc::ImageBuffer>();
        doc::ImageRef sampleRender(sample.createRender(sampleBuf));
        auto it = duplicates.find(sampleRender);
        if (it != duplicates.end()) {
          const uint32_t j = it->second;

          sample.setDuplicated();
          sample.setSharedBounds(samples[j].sharedBounds());
          ++i;
          continue;
        }
        else {
          duplicates[sampleRender] = i;
        }
      }

      const Sprite* sprite = sample.sprite();
      const Layer* layer = sample.layer();
      const Tag* tag = sample.tag();
      gfx::Size size = sample.requiredSize();

      if (breakBands && oldSprite) {
        const bool nextBand =
          ((oldSprite != sprite) ||
           (m_splitLayers && oldLayer != layer) ||
           (m_splitTags && oldTag != tag) ||
           (itemInBand == itemsPerBand));

        if (m_type == SpriteSheetType::Columns) {
          // If the user didn't specify a height for the texture, we
          // put each sprite/layer in a different column.
          if (height == 0) {
            // New sprite or layer, go to next column.
            if (nextBand) {
              framePt.x += rowSize.w + shapePadding;
              framePt.y = borderPadding;
              rowSize = size;
              itemInBand = 0;
            }
          }
          // When a texture height is specified, we can put different
          // sprites/layers in each column until we reach the texture
          // bottom-border.
          else if (framePt.y+size.h > height-borderPadding) {
            framePt.x += rowSize.w + shapePadding;
            framePt.y = borderPadding;
            rowSize = size;
          }
        }
        else if (m_type == SpriteSheetType::Rows) {
          // If the user didn't specify a width for the texture, we put
          // each sprite/layer in a different row.
          if (width == 0) {
            // New sprite or layer, go to next row.
            if (nextBand) {
              framePt.x = borderPadding;
              framePt.y += rowSize.h + shapePadding;
              rowSize = size;
              itemInBand = 0;
            }
          }
          // When a texture width is specified, we can put different
          // sprites/layers in each row until we reach the texture
          // right-border.
          else if (framePt.x+size.w > width-borderPadding) {
            framePt.x = borderPadding;
            framePt.y += rowSize.h + shapePadding;
            rowSize = size;
          }
        }
        else {
          ASSERT(false);
        }
      }

      sample.setInTextureBounds(gfx::Rect(framePt, size));

      // Next frame position.
      if (m_type == SpriteSheetType::Vertical ||
          m_type == SpriteSheetType::Columns) {
        framePt.y += size.h + shapePadding;
      }
      else if (m_type == SpriteSheetType::Horizontal ||
               m_type == SpriteSheetType::Rows) {
        framePt.x += size.w + shapePadding;
      }

      rowSize = rowSize.createUnion(size);

      oldSprite = sprite;
      oldLayer = layer;
      oldTag = tag;
      ++itemInBand;
      ++i;
    }

    DX_TRACE("DX: -> SimpleLayoutSamples", width, height);
  }

private:
  SpriteSheetType m_type;
  int m_maxCols;
  int m_maxRows;
  bool m_splitLayers;
  bool m_splitTags;
  bool m_mergeDups;
};

class DocExporter::BestFitLayoutSamples : public DocExporter::LayoutSamples {
public:
  void layoutSamples(Samples& samples,
                     int borderPadding,
                     int shapePadding,
                     int& width, int& height,
                     base::task_token& token) override {
    gfx::PackingRects pr(borderPadding, shapePadding);
    doc::ImagesMap duplicates;

    uint32_t i = 0;
    for (auto& sample : samples) {
      if (token.canceled())
        return;
      token.set_progress_range(0.2f, 0.3f);
      token.set_progress(float(i) / samples.size());

      if (sample.isEmpty()) {
        ++i;
        continue;
      }

      // We have to use one ImageBuffer for each image because we're
      // going to store all images in the "duplicates" map.
      doc::ImageBufferPtr sampleBuf = std::make_shared<doc::ImageBuffer>();
      doc::ImageRef sampleRender(sample.createRender(sampleBuf));
      auto it = duplicates.find(sampleRender);
      if (it != duplicates.end()) {
        const uint32_t j = it->second;

        sample.setDuplicated();
        sample.setSharedBounds(samples[j].sharedBounds());
      }
      else {
        duplicates[sampleRender] = i;
        pr.add(sample.requiredSize());
      }
      ++i;
    }

    token.set_progress_range(0.3f, 0.4f);
    if (width == 0 || height == 0) {
      gfx::Size sz = pr.bestFit(token, width, height);
      width = sz.w;
      height = sz.h;
    }
    else {
      pr.pack(gfx::Size(width, height), token);
    }
    token.set_progress_range(0.0f, 1.0f);

    auto it = pr.begin();
    for (auto& sample : samples) {
      if (sample.isLinked() ||
          sample.isDuplicated() ||
          sample.isEmpty())
        continue;

      ASSERT(it != pr.end());
      sample.setInTextureBounds(*(it++));
    }
  }
};

DocExporter::DocExporter()
  : m_docBuf(std::make_shared<doc::ImageBuffer>())
  , m_sampleBuf(std::make_shared<doc::ImageBuffer>())
{
  m_cache.spriteId = doc::NullId;
  reset();
}

void DocExporter::reset()
{
  m_sheetType = SpriteSheetType::None;
  m_dataFormat = SpriteSheetDataFormat::Default;
  m_dataFilename.clear();
  m_textureFilename.clear();
  m_filenameFormat.clear();
  m_tagnameFormat.clear();
  m_textureWidth = 0;
  m_textureHeight = 0;
  m_textureColumns = 0;
  m_textureRows = 0;
  m_borderPadding = 0;
  m_shapePadding = 0;
  m_innerPadding = 0;
  m_ignoreEmptyCels = false;
  m_mergeDuplicates = false;
  m_trimSprite = false;
  m_trimCels = false;
  m_trimByGrid = false;
  m_extrude = false;
  m_splitLayers = false;
  m_splitTags = false;
  m_listTags = false;
  m_listLayers = false;
  m_listSlices = false;
  m_documents.clear();
}

void DocExporter::setDocImageBuffer(const doc::ImageBufferPtr& docBuf)
{
  m_docBuf = docBuf;
}

Doc* DocExporter::exportSheet(Context* ctx, base::task_token& token)
{
  // We output the metadata to std::cout if the user didn't specify a file.
  std::ofstream fos;
  std::streambuf* osbuf = nullptr;
  if (m_dataFilename.empty()) {
    // Redirect to stdout if we are running in batch mode
    if (!ctx->isUIAvailable())
      osbuf = std::cout.rdbuf();
  }
  else {
    // Make missing directories for the json file
    {
      std::string dir = base::get_file_path(m_dataFilename);
      try {
        if (!base::is_directory(dir))
          base::make_all_directories(dir);
      }
      catch (const std::exception& ex) {
        Console console;
        console.printf("Error creating directory \"%s\"\n%s",
                       dir.c_str(), ex.what());
      }
    }

    fos.open(FSTREAM_PATH(m_dataFilename), std::ios::out);
    osbuf = fos.rdbuf();
  }
  std::ostream os(osbuf);

  // Steps for sheet construction:
  // 1) Capture the samples (each sprite+frame pair)
  Samples samples;
  captureSamples(samples, token);
  if (samples.empty()) {
    if (!ctx->isUIAvailable()) {
      Console console;
      console.printf("No documents to export");
    }
    return nullptr;
  }
  if (token.canceled())
    return nullptr;
  token.set_progress(0.2f);

  // 2) Layout those samples in a texture field.
  layoutSamples(samples, token);
  if (token.canceled())
    return nullptr;
  token.set_progress(0.4f);

  // 3) Create and render the texture.
  std::unique_ptr<Doc> textureDocument(
    createEmptyTexture(samples, token));
  if (token.canceled())
    return nullptr;
  token.set_progress(0.6f);

  Sprite* texture = textureDocument->sprite();
  Image* textureImage = texture->root()->firstLayer()
    ->cel(frame_t(0))->image();

  renderTexture(ctx, samples, textureImage, token);
  if (token.canceled())
    return nullptr;
  token.set_progress(0.8f);

  // Trim texture
  if (m_trimSprite || m_trimCels)
    trimTexture(samples, texture);
  token.set_progress(0.9f);

  // Save the metadata.
  if (osbuf)
    createDataFile(samples, os, texture);
  token.set_progress(0.95f);

  // Save the image files.
  if (!m_textureFilename.empty()) {
    DX_TRACE("DX: exportSheet", m_textureFilename);
    textureDocument->setFilename(m_textureFilename.c_str());
    int ret = save_document(ctx, textureDocument.get());
    if (ret == 0)
      textureDocument->markAsSaved();
  }

  token.set_progress(1.0f);

  return textureDocument.release();
}

gfx::Size DocExporter::calculateSheetSize()
{
  base::task_token token;
  Samples samples;
  captureSamples(samples, token);
  layoutSamples(samples, token);
  return calculateSheetSize(samples, token);
}

void DocExporter::addDocument(
  Doc* doc,
  const doc::Tag* tag,
  const doc::SelectedLayers* selLayers,
  const doc::SelectedFrames* selFrames,
  const bool splitGrid)
{
  DX_TRACE("DX: addDocument doc=", doc, "tag=", tag);
  m_documents.push_back(Item(doc, tag, selLayers, selFrames, splitGrid));
}

void DocExporter::addImage(
  Doc* doc,
  const doc::ImageRef& image)
{
  DX_TRACE("DX: addImage doc=", doc, "image=", image.get());
  m_documents.push_back(Item(doc, image));
}

int DocExporter::addDocumentSamples(
  Doc* doc,
  const doc::Tag* thisTag,
  const bool splitLayers,
  const bool splitTags,
  const bool splitGrid,
  const doc::SelectedLayers* selLayers,
  const doc::SelectedFrames* selFrames)
{
  DX_TRACE("DX: addDocumentSamples");

  std::vector<const Tag*> tags;

  if (thisTag)
    tags.push_back(thisTag);
  else if (splitTags) {
    if (selFrames) {
      const Tag* oldTag = nullptr;
      for (frame_t frame : *selFrames) {
        const Tag* tag = doc->sprite()->tags().innerTag(frame);
        if (oldTag != tag) {
          oldTag = tag;
          // Do not include untagged frames
          if (tag)
            tags.push_back(tag);
        }
      }
    }
    else {
      for (const Tag* tag : doc->sprite()->tags())
        tags.push_back(tag);
    }
    if (tags.empty())
      tags.push_back(nullptr);
  }
  else {
    tags.push_back(nullptr);
  }

  doc::SelectedFrames selFramesTmp;
  int items = 0;
  for (const Tag* tag : tags) {
    const doc::SelectedFrames* thisSelFrames = nullptr;

    if (selFrames) {
      if (tag) {
        selFramesTmp.clear();
        for (frame_t frame=tag->fromFrame(); frame<=tag->toFrame(); ++frame) {
          if (selFrames->contains(frame))
            selFramesTmp.insert(frame);
        }
        thisSelFrames = &selFramesTmp;
      }
      else {
        selFramesTmp = *selFrames;
        thisSelFrames = &selFramesTmp;
      }
    }
    else if (tag) {
      ASSERT(tag);
      selFramesTmp.clear();
      selFramesTmp.insert(tag->fromFrame(),
                          tag->toFrame());
      thisSelFrames = &selFramesTmp;
    }

    if (splitLayers) {
      if (selLayers) {
        for (auto layer : selLayers->toAllLayersList()) {
          if (layer->isGroup()) // Ignore groups
            continue;

          SelectedLayers oneLayer;
          oneLayer.insert(layer);
          addDocument(doc, tag, &oneLayer, thisSelFrames, splitGrid);
          ++items;
        }
      }
      else {
        for (auto layer : doc->sprite()->allVisibleLayers()) {
          if (layer->isGroup()) // Ignore groups
            continue;

          SelectedLayers oneLayer;
          oneLayer.insert(layer);
          addDocument(doc, tag, &oneLayer, thisSelFrames, splitGrid);
          ++items;
        }
      }
    }
    else {
      addDocument(doc, tag, selLayers, thisSelFrames, splitGrid);
      ++items;
    }
  }
  return std::max(1, items);
}

int DocExporter::addTilesetsSamples(
  Doc* doc,
  const doc::SelectedLayers* selLayers)
{
  LayerList layers;
  if (selLayers)
    layers = selLayers->toAllLayersList();
  else
    layers = doc->sprite()->allVisibleLayers();

  std::set<doc::ObjectId> alreadyExported;
  int items = 0;
  for (auto& layer : layers) {
    if (layer->isTilemap()) {
      Tileset* ts = dynamic_cast<LayerTilemap*>(layer)->tileset();

      if (alreadyExported.find(ts->id()) == alreadyExported.end()) {
        for (const auto& tile : *ts) {
          addImage(doc, tile.image);
          ++items;
        }
        alreadyExported.insert(ts->id());
      }
    }
  }

  DX_TRACE("DX: addTilesetsSamples items=", items);
  return items;
}

void DocExporter::captureSamples(Samples& samples,
                                 base::task_token& token)
{
  DX_TRACE("DX: Capture samples");

  for (auto& item : m_documents) {
    if (token.canceled())
      return;

    Doc* doc = item.doc;
    Sprite* sprite = doc->sprite();
    Layer* layer = (item.selLayers && item.selLayers->size() == 1 ?
                    *item.selLayers->begin(): nullptr);
    const Tag* tag = item.tag;
    int frames = item.frames();

    DX_TRACE("DX: - Item:", doc->filename(),
             "Frames:", frames,
             "Layer:", layer ? layer->name(): "-",
             "Tag:", tag ? tag->name(): "-");

    std::string format = m_filenameFormat;
    if (format.empty()) {
      format = get_default_filename_format_for_sheet(
        doc->filename(),
        (frames > 1),                   // Has frames
        (layer != nullptr),             // Has layer
        (tag != nullptr));              // Has tag
    }

    gfx::Rect spriteBounds;

    // This item is only one image (e.g. a tileset tile)
    if (item.isOneImageOnly()) {
      ASSERT(item.image);
      spriteBounds = item.image->bounds();
    }
    // This item comes from the sprite canvas
    else {
      spriteBounds = sprite->bounds();
      if (m_trimSprite) {
        if (m_cache.spriteId == sprite->id() &&
            m_cache.spriteVer == sprite->version() &&
            m_cache.trimmedByGrid == m_trimByGrid) {
          spriteBounds = m_cache.trimmedBounds;
        }
        else {
          spriteBounds = get_trimmed_bounds(sprite, m_trimByGrid);
          if (spriteBounds.isEmpty())
            spriteBounds = gfx::Rect(0, 0, 1, 1);

          // Cache trimmed bounds so we don't have to recalculate them
          // in the next iteration/preview.
          m_cache.spriteId = sprite->id();
          m_cache.spriteVer = sprite->version();
          m_cache.trimmedByGrid = m_trimByGrid;
          m_cache.trimmedBounds = spriteBounds;
        }
      }
    }

    frame_t outputFrame = 0;
    for (frame_t frame : item.getSelectedFrames()) {
      if (token.canceled())
        return;

      const Tag* innerTag = (tag ? tag: sprite->tags().innerTag(frame));
      const Tag* outerTag = sprite->tags().outerTag(frame);
      FilenameInfo fnInfo;
      fnInfo
        .filename(doc->filename())
        .layerName(layer ? layer->name(): "")
        .groupName(layer && layer->parent() != sprite->root() ? layer->parent()->name(): "")
        .innerTagName(innerTag ? innerTag->name(): "")
        .outerTagName(outerTag ? outerTag->name(): "")
        .frame(outputFrame)
        .tagFrame(innerTag ? frame - innerTag->fromFrame():
                             outputFrame)
        .duration(sprite->frameDuration(frame));
      ++outputFrame;

      std::string filename = filename_formatter(format, fnInfo);

      Sample sample(
        (item.image ? item.image->size():
         item.splitGrid ? sprite->gridBounds().size():
                          sprite->size()),
        doc, sprite, item.image, item.selLayers.get(),
        frame, innerTag, filename,
        m_innerPadding, m_extrude);
      Cel* cel = nullptr;
      Cel* link = nullptr;
      bool done = false;

      if (layer && layer->isImage()) {
        cel = layer->cel(frame);
        if (cel)
          link = cel->link();
      }

      // Re-use linked samples
      bool alreadyTrimmed = false;
      if (link && m_mergeDuplicates &&
          !item.isOneImageOnly()) {
        for (const Sample& other : samples) {
          if (token.canceled())
            return;

          if (other.sprite() == sprite &&
              other.layer() == layer &&
              other.frame() == link->frame()) {
            ASSERT(!other.isLinked());

            sample.setLinked();
            sample.setTrimmedBounds(other.trimmedBounds());
            sample.setSharedBounds(other.sharedBounds());
            alreadyTrimmed = true;
            done = true;
            break;
          }
        }
        // "done" variable can be false here, e.g. when we export a
        // frame tag and the first linked cel is outside the tag range.
        ASSERT(done || (!done && tag));
      }

      if (!done && (m_ignoreEmptyCels || m_trimCels) &&
          !item.isOneImageOnly()) {
        // Ignore empty cels
        if (layer && layer->isImage() && !cel && m_ignoreEmptyCels)
          continue;

        ImageRef sampleRender(sample.createRender(m_sampleBuf));

        gfx::Rect frameBounds;
        doc::color_t refColor = 0;

        if (m_trimCels) {
          if ((layer &&
               layer->isBackground()) ||
              (!layer &&
               sprite->backgroundLayer() &&
               sprite->backgroundLayer()->isVisible())) {
            refColor = get_pixel(sampleRender.get(), 0, 0);
          }
          else {
            refColor = sprite->transparentColor();
          }
        }
        else if (m_ignoreEmptyCels)
          refColor = sprite->transparentColor();

        if (!algorithm::shrink_bounds(sampleRender.get(),
                                      refColor,
                                      nullptr,        // layer
                                      spriteBounds,   // startBounds
                                      frameBounds)) { // output bounds
          // If shrink_bounds() returns false, it's because the whole
          // image is transparent (equal to the mask color).

          // Should we ignore this empty frame? (i.e. don't include
          // the frame in the sprite sheet)
          if (m_ignoreEmptyCels)
            continue;

          // Create an entry with Size(1, 1) for this completely
          // trimmed frame anyway so we conserve the frame information
          // (position and duration of the frame in the JSON data, and
          // the relative position of the frame in frame tags).
          sample.setTrimmedBounds(frameBounds = gfx::Rect(0, 0, 1, 1));
        }

        if (m_trimCels) {
          // TODO merge this code with the code in DocApi::trimSprite()
          if (m_trimByGrid) {
            const gfx::Rect& gridBounds = doc->sprite()->gridBounds();
            gfx::Point posTopLeft =
              snap_to_grid(gridBounds,
                           frameBounds.origin(),
                           PreferSnapTo::FloorGrid);
            gfx::Point posBottomRight =
              snap_to_grid(gridBounds,
                           frameBounds.point2(),
                           PreferSnapTo::CeilGrid);
            frameBounds = gfx::Rect(posTopLeft, posBottomRight);
          }
          sample.setTrimmedBounds(frameBounds);
          alreadyTrimmed = true;
        }
      }
      // If "Ignore Empty" is checked and the item is a tile...
      else if (m_ignoreEmptyCels && item.isOneImageOnly()) {
        // Skip empty tile
        if (is_empty_image(item.image.get()))
          continue;
      }

      if (!alreadyTrimmed && m_trimSprite)
        sample.setTrimmedBounds(spriteBounds);

      if (item.splitGrid) {
        const gfx::Rect& gridBounds = sprite->gridBounds();
        gfx::Point initPos(0, 0), pos;
        initPos = pos = snap_to_grid(gridBounds, initPos, PreferSnapTo::BoxOrigin);

        for (; pos.y+gridBounds.h <= spriteBounds.h; pos.y+=gridBounds.h) {
          for (pos.x=initPos.x; pos.x+gridBounds.w <= spriteBounds.w; pos.x+=gridBounds.w) {
            const gfx::Rect cellBounds(pos, gridBounds.size());
            sample.setTrimmedBounds(cellBounds);
            sample.setSharedBounds(std::make_shared<gfx::Rect>(sample.inTextureBounds()));
            samples.addSample(sample);
          }
        }
      }
      else {
        samples.addSample(sample);
      }

      DX_TRACE("DX:   - Sample:",
               sample.document()->filename(),
               "Layer:", sample.layer() ? sample.layer()->name(): "-",
               "TrimmedBounds:", sample.trimmedBounds(),
               "InTextureBounds:", sample.inTextureBounds());
    }
  }
}

void DocExporter::layoutSamples(Samples& samples,
                                base::task_token& token)
{
  int width = m_textureWidth;
  int height = m_textureHeight;

  switch (m_sheetType) {
    case SpriteSheetType::Packed: {
      BestFitLayoutSamples layout;
      layout.layoutSamples(
        samples, m_borderPadding, m_shapePadding,
        width, height, token);
      break;
    }
    default: {
      SimpleLayoutSamples layout(
        m_sheetType,
        m_textureColumns, m_textureRows,
        m_splitLayers, m_splitTags,
        m_mergeDuplicates);
      layout.layoutSamples(
        samples, m_borderPadding, m_shapePadding,
        width, height, token);
      break;
    }
  }
}

gfx::Size DocExporter::calculateSheetSize(const Samples& samples,
                                          base::task_token& token) const
{
  DX_TRACE("DX: calculateSheetSize predefined texture size",
           m_textureWidth, m_textureHeight);

  gfx::Rect fullTextureBounds(0, 0, m_textureWidth, m_textureHeight);

  for (const auto& sample : samples) {
    if (token.canceled())
      return gfx::Size(0, 0);

    if (sample.isLinked() ||
        sample.isDuplicated() ||
        sample.isEmpty())
      continue;

    gfx::Rect sampleBounds = sample.inTextureBounds();

    // If the user specified a fixed sprite sheet size, we add the
    // border padding in the sample size to do an union between
    // fullTextureBounds and sample's inTextureBounds (generally, it
    // shouldn't make fullTextureBounds bigger).
    if (m_textureWidth > 0) sampleBounds.w += m_borderPadding;
    if (m_textureHeight > 0) sampleBounds.h += m_borderPadding;

    fullTextureBounds |= sampleBounds;
  }

  // If the user didn't specified the sprite sheet size, the border is
  // added right here (the left/top border padding should be added by
  // the DocExporter::LayoutSamples() impl).
  if (m_textureWidth == 0) fullTextureBounds.w += m_borderPadding;
  if (m_textureHeight == 0) fullTextureBounds.h += m_borderPadding;

  DX_TRACE("DX: calculateSheetSize -> ",
           fullTextureBounds.x+fullTextureBounds.w,
           fullTextureBounds.y+fullTextureBounds.h);

  return gfx::Size(fullTextureBounds.x+fullTextureBounds.w,
                   fullTextureBounds.y+fullTextureBounds.h);
}

Doc* DocExporter::createEmptyTexture(const Samples& samples,
                                     base::task_token& token) const
{
  ColorMode colorMode = ColorMode::INDEXED;
  Palette* palette = nullptr;
  int maxColors = 256;
  gfx::ColorSpaceRef colorSpace;
  color_t transparentColor = 0;

  for (const auto& sample : samples) {
    if (token.canceled())
      return nullptr;

    if (sample.isLinked() ||
        sample.isDuplicated() ||
        sample.isEmpty())
      continue;

    // TODO throw a warning if samples contain different color spaces
    if (!colorSpace) {
      if (sample.sprite())
        colorSpace = sample.sprite()->colorSpace();
    }

    // We try to render an indexed image. But if we find a sprite with
    // two or more palettes, or two of the sprites have different
    // palettes, we've to use RGB format.
    if (colorMode == ColorMode::INDEXED) {
      if (sample.sprite()->colorMode() != ColorMode::INDEXED) {
        colorMode = ColorMode::RGB;
      }
      else if (sample.sprite()->getPalettes().size() > 1) {
        colorMode = ColorMode::RGB;
      }
      else if (palette &&
               palette->countDiff(sample.sprite()->palette(frame_t(0)),
                                  nullptr, nullptr) > 0) {
        colorMode = ColorMode::RGB;
      }
      else if (!palette) {
        palette = sample.sprite()->palette(frame_t(0));
        transparentColor = sample.sprite()->transparentColor();
      }
    }
  }

  gfx::Size textureSize = calculateSheetSize(samples, token);
  if (token.canceled())
    return nullptr;

  std::unique_ptr<Sprite> sprite(
    Sprite::MakeStdSprite(
      ImageSpec(colorMode,
                std::max(textureSize.w, m_textureWidth),
                std::max(textureSize.h, m_textureHeight),
                transparentColor,
                (colorSpace ? colorSpace: gfx::ColorSpace::MakeNone())),
      maxColors,
      m_docBuf));

  if (palette)
    sprite->setPalette(palette, false);

  std::unique_ptr<Doc> document(new Doc(sprite.get()));
  sprite.release();

  return document.release();
}

void DocExporter::renderTexture(Context* ctx,
                                const Samples& samples,
                                Image* textureImage,
                                base::task_token& token) const
{
  textureImage->clear(textureImage->maskColor());

  int i = 0;
  for (const auto& sample : samples) {
    if (token.canceled())
      return;
    token.set_progress(0.6f + 0.2f * i / int(samples.size()));

    if (sample.isLinked() ||
        sample.isDuplicated() ||
        sample.isEmpty()) {
      ++i;
      continue;
    }

    // Make the sprite compatible with the texture so the render()
    // works correctly.
    if (sample.sprite()->pixelFormat() != textureImage->pixelFormat()) {
      cmd::SetPixelFormat(
        sample.sprite(),
        textureImage->pixelFormat(),
        render::Dithering(),
        Sprite::DefaultRgbMapAlgorithm(), // TODO add rgbmap algorithm preference
        nullptr, // toGray is not needed because the texture is Indexed or RGB
        nullptr) // TODO add a delegate to show progress
        .execute(ctx);
    }

    sample.renderSample(
      textureImage,
      sample.inTextureBounds().x+m_innerPadding,
      sample.inTextureBounds().y+m_innerPadding,
      m_extrude);
    ++i;
  }
}

void DocExporter::trimTexture(const Samples& samples,
                              doc::Sprite* texture) const
{
  if (m_textureWidth > 0 && m_textureHeight > 0)
    return;

  gfx::Size size = texture->size();
  gfx::Rect bounds(0, 0, 1, 1);

  for (const auto& sample : samples) {
    if (sample.isLinked() ||
        sample.isDuplicated() ||
        sample.isEmpty())
      continue;

    bounds |= sample.inTextureBounds();
  }

  if (m_textureWidth == 0) {
    ASSERT(size.w >= bounds.w);
    size.w = bounds.w;
  }
  if (m_textureHeight == 0) {
    ASSERT(size.h >= bounds.h);
    size.h = bounds.h;
  }

  texture->setSize(m_textureWidth > 0 ? m_textureWidth: size.w,
                   m_textureHeight > 0 ? m_textureHeight: size.h);
}

void DocExporter::createDataFile(const Samples& samples,
                                 std::ostream& os,
                                 doc::Sprite* texture)
{
  std::string frames_begin;
  std::string frames_end;
  bool filename_as_key = false;
  bool filename_as_attr = false;
  int nonExtrudedPosition = 0;
  int nonExtrudedSize = 0;

  // if the the image was extruded then the exported meta-information (JSON)
  // should inform where start the real image (+1 displaced) and its
  // size (-2 pixels: one per each dimension compared the extruded image)
  if (m_extrude) {
    nonExtrudedPosition += 1;
    nonExtrudedSize -= 2;
  }

  // TODO we should use some string templates system here
  switch (m_dataFormat) {
    case SpriteSheetDataFormat::JsonHash:
      frames_begin = "{";
      frames_end = "}";
      filename_as_key = true;
      filename_as_attr = false;
      break;
    case SpriteSheetDataFormat::JsonArray:
      frames_begin = "[";
      frames_end = "]";
      filename_as_key = false;
      filename_as_attr = true;
      break;
  }

  os << "{ \"frames\": " << frames_begin << "\n";
  for (Samples::const_iterator
         it = samples.begin(),
         end = samples.end(); it != end; ) {
    const Sample& sample = *it;
    gfx::Size srcSize = sample.originalSize();
    gfx::Rect spriteSourceBounds = sample.trimmedBounds();
    gfx::Rect frameBounds = sample.inTextureBounds();

    if (filename_as_key)
      os << "   \"" << escape_for_json(sample.filename()) << "\": {\n";
    else if (filename_as_attr)
      os << "   {\n"
         << "    \"filename\": \"" << escape_for_json(sample.filename()) << "\",\n";

    os << "    \"frame\": { "
       << "\"x\": " << frameBounds.x + nonExtrudedPosition << ", "
       << "\"y\": " << frameBounds.y + nonExtrudedPosition << ", "
       << "\"w\": " << frameBounds.w + nonExtrudedSize << ", "
       << "\"h\": " << frameBounds.h + nonExtrudedSize << " },\n"
       << "    \"rotated\": false,\n"
       << "    \"trimmed\": " << (sample.trimmed() ? "true": "false") << ",\n"
       << "    \"spriteSourceSize\": { "
       << "\"x\": " << spriteSourceBounds.x << ", "
       << "\"y\": " << spriteSourceBounds.y << ", "
       << "\"w\": " << spriteSourceBounds.w << ", "
       << "\"h\": " << spriteSourceBounds.h << " },\n"
       << "    \"sourceSize\": { "
       << "\"w\": " << srcSize.w << ", "
       << "\"h\": " << srcSize.h << " },\n"
       << "    \"duration\": " << sample.sprite()->frameDuration(sample.frame()) << "\n"
       << "   }";

    if (++it != samples.end())
      os << ",\n";
    else
      os << "\n";
  }
  os << " " << frames_end;

  // "meta" property
  os << ",\n"
     << " \"meta\": {\n"
     << "  \"app\": \"" << get_app_url() << "\",\n"
     << "  \"version\": \"" << get_app_version() << "\",\n";

  if (!m_textureFilename.empty())
    os << "  \"image\": \""
       << escape_for_json(base::get_file_name(m_textureFilename)).c_str()
       << "\",\n";

  os << "  \"format\": \"" << (texture->pixelFormat() == IMAGE_RGB ? "RGBA8888": "I8") << "\",\n"
     << "  \"size\": { "
     << "\"w\": " << texture->width() << ", "
     << "\"h\": " << texture->height() << " },\n"
     << "  \"scale\": \"1\"";

  // meta.frameTags
  if (m_listTags) {
    os << ",\n"
       << "  \"frameTags\": ["; // TODO rename this someday in the future

    std::set<doc::ObjectId> includedSprites;

    bool firstTag = true;
    for (auto& item : m_documents) {
      if (item.isOneImageOnly())
        continue;

      Doc* doc = item.doc;
      Sprite* sprite = doc->sprite();

      // Avoid including tags two or more times in the list (e.g. when
      // -split-layers is specified, several calls of addDocument()
      // are used for each layer, so we have to avoid iterating the
      // same sprite several times)
      if (includedSprites.find(sprite->id()) != includedSprites.end())
        continue;
      includedSprites.insert(sprite->id());

      for (Tag* tag : sprite->tags()) {
        if (firstTag)
          firstTag = false;
        else
          os << ",";

        std::string format = m_tagnameFormat;
        if (format.empty()) {
          format = "{tag}";
        }

        FilenameInfo fnInfo;
        fnInfo
          .filename(doc->filename())
          .innerTagName(tag->name());
        std::string tagname = filename_formatter(format, fnInfo);
        os << "\n   { \"name\": \"" << escape_for_json(tagname) << "\","
           << " \"from\": " << (tag->fromFrame()) << ","
           << " \"to\": " << (tag->toFrame()) << ","
           " \"direction\": \"" << escape_for_json(convert_anidir_to_string(tag->aniDir())) << "\"";
        if (tag->repeat() > 0) {
          os << ", \"repeat\": \"" << tag->repeat() << "\"";
        }
        os << tag->userData() << " }";
      }
    }
    os << "\n  ]";
  }

  // meta.layers
  if (m_listLayers) {
    LayerList metaLayers;
    for (auto& item : m_documents) {
      if (item.isOneImageOnly())
        continue;

      Doc* doc = item.doc;
      Sprite* sprite = doc->sprite();
      Layer* root = sprite->root();

      LayerList layers;
      if (item.selLayers) {
        // Select all layers (not only browseable ones)
        layers = item.selLayers->toAllLayersList();
      }
      else {
        // Select all visible layers by default
        layers = sprite->allVisibleLayers();
      }

      for (Layer* layer : layers) {
        // If this layer is inside a group, check that the group will
        // be included in the meta data too.
        Layer* group = layer->parent();
        int pos = int(metaLayers.size());
        while (group && group != root) {
          if (std::find(metaLayers.begin(), metaLayers.end(), group) == metaLayers.end()) {
            metaLayers.insert(metaLayers.begin()+pos, group);
          }
          group = group->parent();
        }
        // Insert the layer
        if (std::find(metaLayers.begin(), metaLayers.end(), layer) == metaLayers.end()) {
          metaLayers.push_back(layer);
        }
      }
    }

    bool firstLayer = true;
    os << ",\n"
       << "  \"layers\": [";
    for (Layer* layer : metaLayers) {
      if (firstLayer)
        firstLayer = false;
      else
        os << ",";
      os << "\n   { \"name\": \"" << escape_for_json(layer->name()) << "\"";

      if (layer->parent() != layer->sprite()->root())
        os << ", \"group\": \"" << escape_for_json(layer->parent()->name()) << "\"";

      if (LayerImage* layerImg = dynamic_cast<LayerImage*>(layer)) {
        os << ", \"opacity\": " << layerImg->opacity()
           << ", \"blendMode\": \"" << blend_mode_to_string(layerImg->blendMode()) << "\"";
      }
      os << layer->userData();

      // Cels
      CelList cels;
      layer->getCels(cels);
      bool someCelWithData = false;
      for (const Cel* cel : cels) {
        if (cel->zIndex() != 0 ||
            !cel->data()->userData().isEmpty()) {
          someCelWithData = true;
          break;
        }
      }

      if (someCelWithData) {
        bool firstCel = true;

        os << ", \"cels\": [";
        for (const Cel* cel : cels) {
          if (cel->zIndex() != 0 ||
              !cel->data()->userData().isEmpty()) {
            if (firstCel)
              firstCel = false;
            else
              os << ", ";

            os << "{ \"frame\": " << cel->frame();
            if (cel->zIndex() != 0) {
              os << ", \"zIndex\": " << cel->zIndex();
            }
            if (!cel->data()->userData().isEmpty()) {
              os << cel->data()->userData();
            }
            os << " }";
          }
        }
        os << "]";
      }

      os << " }";
    }
    os << "\n  ]";
  }

  // meta.slices
  if (m_listSlices) {
    os << ",\n"
       << "  \"slices\": [";

    std::set<doc::ObjectId> includedSprites;

    bool firstSlice = true;
    for (auto& item : m_documents) {
      if (item.isOneImageOnly())
        continue;

      Doc* doc = item.doc;
      Sprite* sprite = doc->sprite();

      // Avoid including slices two or more times in the list
      // (e.g. when -split-layers is specified, several calls of
      // addDocument() are used for each layer, so we have to avoid
      // iterating the same sprite several times)
      if (includedSprites.find(sprite->id()) != includedSprites.end())
        continue;
      includedSprites.insert(sprite->id());

      // TODO add possibility to export some slices

      for (Slice* slice : sprite->slices()) {
        if (firstSlice)
          firstSlice = false;
        else
          os << ",";
        os << "\n   { \"name\": \"" << escape_for_json(slice->name()) << "\""
           << slice->userData();

        // Keys
        if (!slice->empty()) {
          bool firstKey = true;

          os << ", \"keys\": [";
          for (const auto& key : *slice) {
            if (firstKey)
              firstKey = false;
            else
              os << ", ";

            const SliceKey* sliceKey = key.value();

            os << "{ \"frame\": " << key.frame() << ", "
               << "\"bounds\": {"
               << "\"x\": " << sliceKey->bounds().x << ", "
               << "\"y\": " << sliceKey->bounds().y << ", "
               << "\"w\": " << sliceKey->bounds().w << ", "
               << "\"h\": " << sliceKey->bounds().h << " }";

            if (!sliceKey->center().isEmpty()) {
              os << ", \"center\": {"
                 << "\"x\": " << sliceKey->center().x << ", "
                 << "\"y\": " << sliceKey->center().y << ", "
                 << "\"w\": " << sliceKey->center().w << ", "
                 << "\"h\": " << sliceKey->center().h << " }";
            }

            if (sliceKey->hasPivot()) {
              os << ", \"pivot\": {"
                 << "\"x\": " << sliceKey->pivot().x << ", "
                 << "\"y\": " << sliceKey->pivot().y << " }";
            }

            os << " }";
          }
          os << "]";
        }
        os << " }";
      }
    }
    os << "\n  ]";
  }

  os << "\n }\n"
     << "}\n";
}

} // namespace app
