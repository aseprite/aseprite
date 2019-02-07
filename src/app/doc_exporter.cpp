// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
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
#include "app/pref/preferences.h"
#include "app/restore_visible_layers.h"
#include "app/snap_to_grid.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "base/replace_string.h"
#include "base/shared_ptr.h"
#include "base/string.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/frame_tag.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/selected_frames.h"
#include "doc/selected_layers.h"
#include "doc/slice.h"
#include "doc/sprite.h"
#include "gfx/packing_rects.h"
#include "gfx/size.h"
#include "render/dithering_algorithm.h"
#include "render/ordered_dither.h"
#include "render/render.h"

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>

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

class SampleBounds {
public:
  SampleBounds(Sprite* sprite) :
    m_originalSize(sprite->width(), sprite->height()),
    m_trimmedBounds(0, 0, sprite->width(), sprite->height()),
    m_inTextureBounds(0, 0, sprite->width(), sprite->height()) {
  }

  bool trimmed() const {
    return m_trimmedBounds.x > 0
      || m_trimmedBounds.y > 0
      || m_trimmedBounds.w != m_originalSize.w
      || m_trimmedBounds.h != m_originalSize.h;
  }

  const gfx::Size& originalSize() const { return m_originalSize; }
  const gfx::Rect& trimmedBounds() const { return m_trimmedBounds; }
  const gfx::Rect& inTextureBounds() const { return m_inTextureBounds; }

  void setTrimmedBounds(const gfx::Rect& bounds) { m_trimmedBounds = bounds; }
  void setInTextureBounds(const gfx::Rect& bounds) { m_inTextureBounds = bounds; }

private:
  gfx::Size m_originalSize;
  gfx::Rect m_trimmedBounds;
  gfx::Rect m_inTextureBounds;
};

typedef base::SharedPtr<SampleBounds> SampleBoundsPtr;

DocExporter::Item::Item(Doc* doc,
                             doc::FrameTag* frameTag,
                             doc::SelectedLayers* selLayers,
                             doc::SelectedFrames* selFrames)
  : doc(doc)
  , frameTag(frameTag)
  , selLayers(selLayers ? new doc::SelectedLayers(*selLayers): nullptr)
  , selFrames(selFrames ? new doc::SelectedFrames(*selFrames): nullptr)
{
}

DocExporter::Item::Item(Item&& other)
  : doc(other.doc)
  , frameTag(other.frameTag)
  , selLayers(other.selLayers)
  , selFrames(other.selFrames)
{
  other.selLayers = nullptr;
  other.selFrames = nullptr;
}

DocExporter::Item::~Item()
{
  delete selLayers;
  delete selFrames;
}

int DocExporter::Item::frames() const
{
  if (selFrames)
    return selFrames->size();
  else if (frameTag) {
    int result = frameTag->toFrame() - frameTag->fromFrame() + 1;
    return MID(1, result, doc->sprite()->totalFrames());
  }
  else
    return doc->sprite()->totalFrames();
}

doc::SelectedFrames DocExporter::Item::getSelectedFrames() const
{
  if (selFrames)
    return *selFrames;

  doc::SelectedFrames frames;
  if (frameTag) {
    frames.insert(MID(0, frameTag->fromFrame(), doc->sprite()->lastFrame()),
                  MID(0, frameTag->toFrame(), doc->sprite()->lastFrame()));
  }
  else {
    frames.insert(0, doc->sprite()->lastFrame());
  }
  return frames;
}

class DocExporter::Sample {
public:
  Sample(Doc* document, Sprite* sprite, SelectedLayers* selLayers,
         frame_t frame, const std::string& filename, int innerPadding, bool extrude) :
    m_document(document),
    m_sprite(sprite),
    m_selLayers(selLayers),
    m_frame(frame),
    m_filename(filename),
    m_innerPadding(innerPadding),
    m_extrude(extrude),
    m_bounds(new SampleBounds(sprite)),
    m_isDuplicated(false) {
  }

  Doc* document() const { return m_document; }
  Sprite* sprite() const { return m_sprite; }
  Layer* layer() const {
    return (m_selLayers && m_selLayers->size() == 1 ? *m_selLayers->begin():
                                                      nullptr);
  }
  SelectedLayers* selectedLayers() const { return m_selLayers; }
  frame_t frame() const { return m_frame; }
  std::string filename() const { return m_filename; }
  const gfx::Size& originalSize() const { return m_bounds->originalSize(); }
  const gfx::Rect& trimmedBounds() const { return m_bounds->trimmedBounds(); }
  const gfx::Rect& inTextureBounds() const { return m_bounds->inTextureBounds(); }

  gfx::Size requiredSize() const {
    // if extrude option is enabled, an extra pixel is needed for each side
    // left+right borders and top+bottom borders
    int extraExtrudePixels = m_extrude ? 2 : 0;
    gfx::Size size = m_bounds->trimmedBounds().size();
    size.w += 2*m_innerPadding + extraExtrudePixels;
    size.h += 2*m_innerPadding + extraExtrudePixels;
    return size;
  }

  bool trimmed() const {
    return m_bounds->trimmed();
  }

  void setTrimmedBounds(const gfx::Rect& bounds) { m_bounds->setTrimmedBounds(bounds); }
  void setInTextureBounds(const gfx::Rect& bounds) { m_bounds->setInTextureBounds(bounds); }

  bool isDuplicated() const { return m_isDuplicated; }
  bool isEmpty() const { return m_bounds->trimmedBounds().isEmpty(); }
  SampleBoundsPtr sharedBounds() const { return m_bounds; }

  void setSharedBounds(const SampleBoundsPtr& bounds) {
    m_isDuplicated = true;
    m_bounds = bounds;
  }

private:
  Doc* m_document;
  Sprite* m_sprite;
  SelectedLayers* m_selLayers;
  frame_t m_frame;
  std::string m_filename;
  int m_innerPadding;
  bool m_extrude;
  SampleBoundsPtr m_bounds;
  bool m_isDuplicated;
};

class DocExporter::Samples {
public:
  typedef std::list<Sample> List;
  typedef List::iterator iterator;
  typedef List::const_iterator const_iterator;

  bool empty() const { return m_samples.empty(); }

  void addSample(const Sample& sample) {
    m_samples.push_back(sample);
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
  virtual void layoutSamples(Samples& samples, int borderPadding, int shapePadding, int& width, int& height) = 0;
};

class DocExporter::SimpleLayoutSamples :
    public DocExporter::LayoutSamples {
public:
  SimpleLayoutSamples(SpriteSheetType type)
    : m_type(type) {
  }

  void layoutSamples(Samples& samples, int borderPadding, int shapePadding, int& width, int& height) override {
    const Sprite* oldSprite = NULL;
    const Layer* oldLayer = NULL;

    gfx::Point framePt(borderPadding, borderPadding);
    gfx::Size rowSize(0, 0);

    for (auto& sample : samples) {
      if (sample.isDuplicated())
        continue;

      if (sample.isEmpty()) {
        sample.setInTextureBounds(gfx::Rect(0, 0, 0, 0));
        continue;
      }

      const Sprite* sprite = sample.sprite();
      const Layer* layer = sample.layer();
      gfx::Size size = sample.requiredSize();

      if (oldSprite) {
        if (m_type == SpriteSheetType::Columns) {
          // If the user didn't specify a height for the texture, we
          // put each sprite/layer in a different column.
          if (height == 0) {
            // New sprite or layer, go to next column.
            if (oldSprite != sprite || oldLayer != layer) {
              framePt.x += rowSize.w + shapePadding;
              framePt.y = borderPadding;
              rowSize = size;
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
        else {
          // If the user didn't specify a width for the texture, we put
          // each sprite/layer in a different row.
          if (width == 0) {
            // New sprite or layer, go to next row.
            if (oldSprite != sprite || oldLayer != layer) {
              framePt.x = borderPadding;
              framePt.y += rowSize.h + shapePadding;
              rowSize = size;
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
      }

      sample.setInTextureBounds(gfx::Rect(framePt, size));

      // Next frame position.
      if (m_type == SpriteSheetType::Columns) {
        framePt.y += size.h + shapePadding;
      }
      else {
        framePt.x += size.w + shapePadding;
      }

      rowSize = rowSize.createUnion(size);

      oldSprite = sprite;
      oldLayer = layer;
    }
  }

private:
  SpriteSheetType m_type;
};

class DocExporter::BestFitLayoutSamples :
    public DocExporter::LayoutSamples {
public:
  void layoutSamples(Samples& samples, int borderPadding, int shapePadding, int& width, int& height) override {
    gfx::PackingRects pr;

    // TODO Add support for shape paddings

    for (auto& sample : samples) {
      if (sample.isDuplicated() ||
          sample.isEmpty())
        continue;

      pr.add(sample.requiredSize());
    }

    if (width == 0 || height == 0) {
      gfx::Size sz = pr.bestFit();
      width = sz.w;
      height = sz.h;
    }
    else
      pr.pack(gfx::Size(width, height));

    auto it = samples.begin();
    for (auto& rc : pr) {
      if (it->isDuplicated())
        continue;

      ASSERT(it != samples.end());
      it->setInTextureBounds(rc);
      ++it;
    }
  }
};

DocExporter::DocExporter()
 : m_dataFormat(DefaultDataFormat)
 , m_textureWidth(0)
 , m_textureHeight(0)
 , m_sheetType(SpriteSheetType::None)
 , m_ignoreEmptyCels(false)
 , m_borderPadding(0)
 , m_shapePadding(0)
 , m_innerPadding(0)
 , m_trimCels(false)
 , m_trimByGrid(false)
 , m_extrude(false)
 , m_listFrameTags(false)
 , m_listLayers(false)
 , m_listSlices(false)
{
}

Doc* DocExporter::exportSheet(Context* ctx)
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
  captureSamples(samples);
  if (samples.empty()) {
    Console console;
    console.printf("No documents to export");
    return nullptr;
  }

  // 2) Layout those samples in a texture field.
  layoutSamples(samples);

  // 3) Create and render the texture.
  std::unique_ptr<Doc> textureDocument(
    createEmptyTexture(samples));

  Sprite* texture = textureDocument->sprite();
  Image* textureImage = texture->root()->firstLayer()
    ->cel(frame_t(0))->image();

  renderTexture(ctx, samples, textureImage);

  // Save the metadata.
  if (osbuf)
    createDataFile(samples, os, textureImage);

  // Save the image files.
  if (!m_textureFilename.empty()) {
    textureDocument->setFilename(m_textureFilename.c_str());
    int ret = save_document(ctx, textureDocument.get());
    if (ret == 0)
      textureDocument->markAsSaved();
  }

  return textureDocument.release();
}

gfx::Size DocExporter::calculateSheetSize()
{
  Samples samples;
  captureSamples(samples);
  layoutSamples(samples);
  return calculateSheetSize(samples);
}

void DocExporter::captureSamples(Samples& samples)
{
  for (auto& item : m_documents) {
    Doc* doc = item.doc;
    Sprite* sprite = doc->sprite();
    Layer* layer = (item.selLayers && item.selLayers->size() == 1 ?
                    *item.selLayers->begin(): nullptr);
    FrameTag* frameTag = item.frameTag;
    int frames = item.frames();

    std::string format = m_filenameFormat;
    if (format.empty()) {
      format = get_default_filename_format_for_sheet(
        doc->filename(),
        (frames > 1),                   // Has frames
        (layer != nullptr),             // Has layer
        (frameTag != nullptr));         // Has frame tag
    }

    frame_t outputFrame = 0;
    for (frame_t frame : item.getSelectedFrames()) {
      FrameTag* innerTag = (frameTag ? frameTag: sprite->frameTags().innerTag(frame));
      FrameTag* outerTag = sprite->frameTags().outerTag(frame);
      FilenameInfo fnInfo;
      fnInfo
        .filename(doc->filename())
        .layerName(layer ? layer->name(): "")
        .groupName(layer && layer->parent() != sprite->root() ? layer->parent()->name(): "")
        .innerTagName(innerTag ? innerTag->name(): "")
        .outerTagName(outerTag ? outerTag->name(): "")
        .frame(outputFrame)
        .tagFrame(innerTag ? frame - innerTag->fromFrame():
                             outputFrame);
      ++outputFrame;

      std::string filename = filename_formatter(format, fnInfo);

      Sample sample(doc, sprite, item.selLayers, frame, filename, m_innerPadding, m_extrude);
      Cel* cel = nullptr;
      Cel* link = nullptr;
      bool done = false;

      if (layer && layer->isImage()) {
        cel = layer->cel(frame);
        if (cel)
          link = cel->link();
      }

      // Re-use linked samples
      if (link) {
        for (const Sample& other : samples) {
          if (other.sprite() == sprite &&
              other.layer() == layer &&
              other.frame() == link->frame()) {
            ASSERT(!other.isDuplicated());

            sample.setSharedBounds(other.sharedBounds());
            done = true;
            break;
          }
        }
        // "done" variable can be false here, e.g. when we export a
        // frame tag and the first linked cel is outside the tag range.
        ASSERT(done || (!done && frameTag));
      }

      if (!done && (m_ignoreEmptyCels || m_trimCels)) {
        // Ignore empty cels
        if (layer && layer->isImage() && !cel)
          continue;

        std::unique_ptr<Image> sampleRender(
          Image::create(sprite->pixelFormat(),
            sprite->width(),
            sprite->height(),
            m_sampleRenderBuf));

        sampleRender->setMaskColor(sprite->transparentColor());
        clear_image(sampleRender.get(), sprite->transparentColor());
        renderSample(sample, sampleRender.get(), 0, 0, false);

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

        if (!algorithm::shrink_bounds(sampleRender.get(), frameBounds, refColor)) {
          // If shrink_bounds() returns false, it's because the whole
          // image is transparent (equal to the mask color).

          // Should we ignore this empty frame? (i.e. don't include
          // the frame in the sprite sheet)
          if (m_ignoreEmptyCels) {
            for (FrameTag* tag : sprite->frameTags()) {
              auto& delta = m_tagDelta[tag->id()];

              if (frame < tag->fromFrame()) --delta.first;
              if (frame <= tag->toFrame()) --delta.second;
            }
            continue;
          }

          // Create an empty entry for this completely trimmed frame
          // anyway to get its duration in the list of frames.
          sample.setTrimmedBounds(frameBounds = gfx::Rect(0, 0, 0, 0));
        }

        if (m_trimCels) {
          if (m_trimByGrid) {
            auto& docPref = Preferences::instance().document(doc);
            gfx::Point posTopLeft =
              snap_to_grid(docPref.grid.bounds(),
                           frameBounds.origin(),
                           PreferSnapTo::FloorGrid);
            gfx::Point posBottomRight =
              snap_to_grid(docPref.grid.bounds(),
                           frameBounds.point2(),
                           PreferSnapTo::CeilGrid);
            frameBounds = gfx::Rect(posTopLeft, posBottomRight);
          }
          sample.setTrimmedBounds(frameBounds);
        }
      }

      samples.addSample(sample);
    }
  }
}

void DocExporter::layoutSamples(Samples& samples)
{
  switch (m_sheetType) {
    case SpriteSheetType::Packed: {
      BestFitLayoutSamples layout;
      layout.layoutSamples(
        samples, m_borderPadding, m_shapePadding,
        m_textureWidth, m_textureHeight);
      break;
    }
    default: {
      SimpleLayoutSamples layout(m_sheetType);
      layout.layoutSamples(
        samples, m_borderPadding, m_shapePadding,
        m_textureWidth, m_textureHeight);
      break;
    }
  }
}

gfx::Size DocExporter::calculateSheetSize(const Samples& samples) const
{
  gfx::Rect fullTextureBounds(0, 0, m_textureWidth, m_textureHeight);

  for (const auto& sample : samples) {
    if (sample.isDuplicated() ||
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

  return gfx::Size(fullTextureBounds.x+fullTextureBounds.w,
                   fullTextureBounds.y+fullTextureBounds.h);
}

Doc* DocExporter::createEmptyTexture(const Samples& samples) const
{
  ColorMode colorMode = ColorMode::INDEXED;
  Palette* palette = nullptr;
  int maxColors = 256;
  gfx::ColorSpacePtr colorSpace;

  for (const auto& sample : samples) {
    if (sample.isDuplicated() ||
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
      else if (palette != NULL
        && palette->countDiff(sample.sprite()->palette(frame_t(0)), NULL, NULL) > 0) {
        colorMode = ColorMode::RGB;
      }
      else
        palette = sample.sprite()->palette(frame_t(0));
    }
  }

  gfx::Size textureSize = calculateSheetSize(samples);

  std::unique_ptr<Sprite> sprite(
    Sprite::createBasicSprite(
      ImageSpec(colorMode, textureSize.w, textureSize.h, 0,
                colorSpace ? colorSpace: gfx::ColorSpace::MakeNone()),
      maxColors));

  if (palette != NULL)
    sprite->setPalette(palette, false);

  std::unique_ptr<Doc> document(new Doc(sprite.get()));
  sprite.release();

  return document.release();
}

void DocExporter::renderTexture(Context* ctx, const Samples& samples, Image* textureImage) const
{
  textureImage->clear(0);

  for (const auto& sample : samples) {
    if (sample.isDuplicated() ||
        sample.isEmpty())
      continue;

    // Make the sprite compatible with the texture so the render()
    // works correctly.
    if (sample.sprite()->pixelFormat() != textureImage->pixelFormat()) {
      cmd::SetPixelFormat(
        sample.sprite(),
        textureImage->pixelFormat(),
        render::DitheringAlgorithm::None,
        render::DitheringMatrix(),
        nullptr)                // TODO add a delegate to show progress
        .execute(ctx);
    }

    renderSample(sample, textureImage,
      sample.inTextureBounds().x+m_innerPadding,
      sample.inTextureBounds().y+m_innerPadding,
      m_extrude);
  }
}

void DocExporter::createDataFile(const Samples& samples, std::ostream& os, Image* textureImage)
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
    case JsonHashDataFormat:
      frames_begin = "{";
      frames_end = "}";
      filename_as_key = true;
      filename_as_attr = false;
      break;
    case JsonArrayDataFormat:
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
     << "  \"app\": \"" << WEBSITE << "\",\n"
     << "  \"version\": \"" << VERSION << "\",\n";

  if (!m_textureFilename.empty())
    os << "  \"image\": \"" << escape_for_json(m_textureFilename).c_str() << "\",\n";

  os << "  \"format\": \"" << (textureImage->pixelFormat() == IMAGE_RGB ? "RGBA8888": "I8") << "\",\n"
     << "  \"size\": { "
     << "\"w\": " << textureImage->width() << ", "
     << "\"h\": " << textureImage->height() << " },\n"
     << "  \"scale\": \"1\"";

  // meta.frameTags
  if (m_listFrameTags) {
    os << ",\n"
       << "  \"frameTags\": [";

    bool firstTag = true;
    for (auto& item : m_documents) {
      Doc* doc = item.doc;
      Sprite* sprite = doc->sprite();

      for (FrameTag* tag : sprite->frameTags()) {
        if (firstTag)
          firstTag = false;
        else
          os << ",";

        std::pair<int, int> delta(0, 0);
        if (!m_tagDelta.empty())
          delta = m_tagDelta[tag->id()];

        os << "\n   { \"name\": \"" << escape_for_json(tag->name()) << "\","
           << " \"from\": " << (tag->fromFrame()+delta.first) << ","
           << " \"to\": " << (tag->toFrame()+delta.second) << ","
           << " \"direction\": \"" << escape_for_json(convert_anidir_to_string(tag->aniDir())) << "\" }";
      }
    }
    os << "\n  ]";
  }

  // meta.layers
  if (m_listLayers) {
    os << ",\n"
       << "  \"layers\": [";

    bool firstLayer = true;
    for (auto& item : m_documents) {
      Doc* doc = item.doc;
      Sprite* sprite = doc->sprite();
      LayerList layers;

      if (item.selLayers)
        layers = item.selLayers->toLayerList();
      else
        layers = sprite->allVisibleLayers();

      for (Layer* layer : layers) {
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
          if (!cel->data()->userData().isEmpty()) {
            someCelWithData = true;
            break;
          }
        }

        if (someCelWithData) {
          bool firstCel = true;

          os << ", \"cels\": [";
          for (const Cel* cel : cels) {
            if (!cel->data()->userData().isEmpty()) {
              if (firstCel)
                firstCel = false;
              else
                os << ", ";

              os << "{ \"frame\": " << cel->frame()
                 << cel->data()->userData()
                 << " }";
            }
          }
          os << "]";
        }

        os << " }";
      }
    }
    os << "\n  ]";
  }

  // meta.slices
  if (m_listSlices) {
    os << ",\n"
       << "  \"slices\": [";

    bool firstSlice = true;
    for (auto& item : m_documents) {
      Doc* doc = item.doc;
      Sprite* sprite = doc->sprite();

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

void DocExporter::renderSample(const Sample& sample, doc::Image* dst, int x, int y, bool extrude) const
{
  RestoreVisibleLayers layersVisibility;
  if (sample.selectedLayers())
    layersVisibility.showSelectedLayers(sample.sprite(),
                                        *sample.selectedLayers());

  render::Render render;
  if (extrude) {
    const gfx::Rect& trim = sample.trimmedBounds();

    // Displaced position onto the destination texture
    int dx[] = {0, 1, trim.w+1};
    int dy[] = {0, 1, trim.h+1};

    // Starting point of the area to be copied from the original image
    // taking into account the size of the trimmed sprite
    int srcx[] = {trim.x, trim.x, trim.x2()-1};
    int srcy[] = {trim.y, trim.y, trim.y2()-1};

    // Size of the area to be copied from original image, starting at
    // the point (srcx[i], srxy[j])
    int szx[] = {1, trim.w, 1};
    int szy[] = {1, trim.h, 1};

    // Render a 9-patch image extruding the sample one pixel on each
    // side.
    for(int j=0; j<3; ++j) {
      for(int i=0; i<3; ++i) {
        gfx::Clip clip(x+dx[i], y+dy[j], gfx::RectT<int>(srcx[i], srcy[j], szx[i], szy[j]));
        render.renderSprite(dst, sample.sprite(), sample.frame(), clip);
      }
    }
  }
  else {
    gfx::Clip clip(x, y, sample.trimmedBounds());
    render.renderSprite(dst, sample.sprite(), sample.frame(), clip);
  }
}

} // namespace app
