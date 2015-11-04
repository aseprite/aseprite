// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document_exporter.h"

#include "app/cmd/set_pixel_format.h"
#include "app/console.h"
#include "app/document.h"
#include "app/file/file.h"
#include "app/filename_formatter.h"
#include "app/ui_context.h"
#include "base/convert_to.h"
#include "base/fstream_path.h"
#include "base/path.h"
#include "base/replace_string.h"
#include "base/shared_ptr.h"
#include "base/string.h"
#include "base/unique_ptr.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/dithering_method.h"
#include "doc/frame_tag.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "gfx/packing_rects.h"
#include "gfx/size.h"
#include "render/render.h"

#include <cstdio>
#include <fstream>
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

int DocumentExporter::Item::frames() const
{
  if (frameTag) {
    int result = frameTag->toFrame() - frameTag->fromFrame() + 1;
    return MID(1, result, doc->sprite()->totalFrames());
  }
  else
    return doc->sprite()->totalFrames();
}

int DocumentExporter::Item::fromFrame() const
{
  if (frameTag)
    return MID(0, frameTag->fromFrame(), doc->sprite()->lastFrame());
  else
    return 0;
}

int DocumentExporter::Item::toFrame() const
{
  if (frameTag)
    return MID(0, frameTag->toFrame(), doc->sprite()->lastFrame());
  else
    return doc->sprite()->lastFrame();
}

class DocumentExporter::Sample {
public:
  Sample(Document* document, Sprite* sprite, Layer* layer,
    frame_t frame, const std::string& filename, int innerPadding) :
    m_document(document),
    m_sprite(sprite),
    m_layer(layer),
    m_frame(frame),
    m_filename(filename),
    m_innerPadding(innerPadding),
    m_bounds(new SampleBounds(sprite)),
    m_isDuplicated(false) {
  }

  Document* document() const { return m_document; }
  Sprite* sprite() const { return m_sprite; }
  Layer* layer() const { return m_layer; }
  frame_t frame() const { return m_frame; }
  std::string filename() const { return m_filename; }
  const gfx::Size& originalSize() const { return m_bounds->originalSize(); }
  const gfx::Rect& trimmedBounds() const { return m_bounds->trimmedBounds(); }
  const gfx::Rect& inTextureBounds() const { return m_bounds->inTextureBounds(); }

  gfx::Size requiredSize() const {
    gfx::Size size = m_bounds->trimmedBounds().getSize();
    size.w += 2*m_innerPadding;
    size.h += 2*m_innerPadding;
    return size;
  }

  bool trimmed() const {
    return m_bounds->trimmed();
  }

  void setTrimmedBounds(const gfx::Rect& bounds) { m_bounds->setTrimmedBounds(bounds); }
  void setInTextureBounds(const gfx::Rect& bounds) { m_bounds->setInTextureBounds(bounds); }

  bool isDuplicated() const { return m_isDuplicated; }
  SampleBoundsPtr sharedBounds() const { return m_bounds; }

  void setSharedBounds(const SampleBoundsPtr& bounds) {
    m_isDuplicated = true;
    m_bounds = bounds;
  }

private:
  Document* m_document;
  Sprite* m_sprite;
  Layer* m_layer;
  frame_t m_frame;
  std::string m_filename;
  int m_borderPadding;
  int m_shapePadding;
  int m_innerPadding;
  SampleBoundsPtr m_bounds;
  bool m_isDuplicated;
};

class DocumentExporter::Samples {
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

class DocumentExporter::LayoutSamples {
public:
  virtual ~LayoutSamples() { }
  virtual void layoutSamples(Samples& samples, int borderPadding, int shapePadding, int& width, int& height) = 0;
};

class DocumentExporter::SimpleLayoutSamples :
    public DocumentExporter::LayoutSamples {
public:
  void layoutSamples(Samples& samples, int borderPadding, int shapePadding, int& width, int& height) override {
    const Sprite* oldSprite = NULL;
    const Layer* oldLayer = NULL;

    gfx::Point framePt(borderPadding, borderPadding);
    gfx::Size rowSize(0, 0);

    for (auto& sample : samples) {
      if (sample.isDuplicated())
        continue;

      const Sprite* sprite = sample.sprite();
      const Layer* layer = sample.layer();
      gfx::Size size = sample.requiredSize();

      if (oldSprite) {
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

      sample.setInTextureBounds(gfx::Rect(framePt, size));

      // Next frame position.
      framePt.x += size.w + shapePadding;
      rowSize = rowSize.createUnion(size);

      oldSprite = sprite;
      oldLayer = layer;
    }
  }
};

class DocumentExporter::BestFitLayoutSamples :
    public DocumentExporter::LayoutSamples {
public:
  void layoutSamples(Samples& samples, int borderPadding, int shapePadding, int& width, int& height) override {
    gfx::PackingRects pr;

    // TODO Add support for shape paddings

    for (auto& sample : samples) {
      if (sample.isDuplicated())
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

DocumentExporter::DocumentExporter()
 : m_dataFormat(DefaultDataFormat)
 , m_textureFormat(DefaultTextureFormat)
 , m_textureWidth(0)
 , m_textureHeight(0)
 , m_texturePack(false)
 , m_scale(1.0)
 , m_scaleMode(DefaultScaleMode)
 , m_ignoreEmptyCels(false)
 , m_borderPadding(0)
 , m_shapePadding(0)
 , m_innerPadding(0)
 , m_trimCels(false)
 , m_listFrameTags(false)
 , m_listLayers(false)
{
}

Document* DocumentExporter::exportSheet()
{
  // We output the metadata to std::cout if the user didn't specify a file.
  std::ofstream fos;
  std::streambuf* osbuf;
  if (m_dataFilename.empty())
    osbuf = std::cout.rdbuf();
  else {
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
  if (m_texturePack) {
    BestFitLayoutSamples layout;
    layout.layoutSamples(samples,
      m_borderPadding, m_shapePadding, m_textureWidth, m_textureHeight);
  }
  else {
    SimpleLayoutSamples layout;
    layout.layoutSamples(samples,
      m_borderPadding, m_shapePadding, m_textureWidth, m_textureHeight);
  }

  // 3) Create and render the texture.
  base::UniquePtr<Document> textureDocument(
    createEmptyTexture(samples));

  Sprite* texture = textureDocument->sprite();
  Image* textureImage = texture->folder()->getFirstLayer()
    ->cel(frame_t(0))->image();

  renderTexture(samples, textureImage);

  // Save the metadata.
  createDataFile(samples, os, textureImage);

  // Save the image files.
  if (!m_textureFilename.empty()) {
    textureDocument->setFilename(m_textureFilename.c_str());
    int ret = save_document(UIContext::instance(), textureDocument.get());
    if (ret == 0)
      textureDocument->markAsSaved();
  }

  return textureDocument.release();
}

void DocumentExporter::captureSamples(Samples& samples)
{
  for (auto& item : m_documents) {
    Document* doc = item.doc;
    Sprite* sprite = doc->sprite();
    Layer* layer = item.layer;
    FrameTag* frameTag = item.frameTag;
    int frames = item.frames();
    bool hasFrames = (frames > 1);
    bool hasLayer = (layer != nullptr);
    bool hasFrameTag = (frameTag && !item.temporalTag);

    std::string format = m_filenameFormat;
    if (format.empty()) {
      if (hasFrames || hasLayer | hasFrameTag) {
        format = "{title}";
        if (hasLayer   ) format += " ({layer})";
        if (hasFrameTag) format += " #{tag}";
        if (hasFrames  ) format += " {frame}";
        format += ".{extension}";
      }
      else
        format = "{name}";
    }

    frame_t frameFirst = item.fromFrame();
    frame_t frameLast = item.toFrame();
    for (frame_t frame=frameFirst; frame<=frameLast; ++frame) {
      FrameTag* innerTag = (frameTag ? frameTag: sprite->frameTags().innerTag(frame));
      FrameTag* outerTag = sprite->frameTags().outerTag(frame);
      FilenameInfo fnInfo;
      fnInfo
        .filename(doc->filename())
        .layerName(layer ? layer->name(): "")
        .innerTagName(innerTag ? innerTag->name(): "")
        .outerTagName(outerTag ? outerTag->name(): "")
        .frame((frames > 1) ? frame-frameFirst: frame_t(-1));

      std::string filename = filename_formatter(format, fnInfo);

      Sample sample(doc, sprite, layer, frame, filename, m_innerPadding);
      Cel* cel = nullptr;
      Cel* link = nullptr;
      bool done = false;

      if (layer && layer->isImage())
        cel = layer->cel(frame);

      if (cel)
        link = cel->link();

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

        base::UniquePtr<Image> sampleRender(
          Image::create(sprite->pixelFormat(),
            sprite->width(),
            sprite->height(),
            m_sampleRenderBuf));

        sampleRender->setMaskColor(sprite->transparentColor());
        clear_image(sampleRender, sprite->transparentColor());
        renderSample(sample, sampleRender, 0, 0);

        gfx::Rect frameBounds;
        doc::color_t refColor = 0;

        if (m_trimCels) {
          if ((layer &&
               layer->isBackground()) ||
              (!layer &&
               sprite->backgroundLayer() &&
               sprite->backgroundLayer()->isVisible())) {
            refColor = get_pixel(sampleRender, 0, 0);
          }
          else {
            refColor = sprite->transparentColor();
          }
        }
        else if (m_ignoreEmptyCels)
          refColor = sprite->transparentColor();

        if (!algorithm::shrink_bounds(sampleRender, frameBounds, refColor)) {
          // If shrink_bounds() returns false, it's because the whole
          // image is transparent (equal to the mask color).
          continue;
        }

        if (m_trimCels)
          sample.setTrimmedBounds(frameBounds);
      }

      samples.addSample(sample);
    }
  }
}

Document* DocumentExporter::createEmptyTexture(const Samples& samples)
{
  Palette* palette = NULL;
  PixelFormat pixelFormat = IMAGE_INDEXED;
  gfx::Rect fullTextureBounds(0, 0, m_textureWidth, m_textureHeight);
  int maxColors = 256;

  for (Samples::const_iterator
         it = samples.begin(),
         end = samples.end(); it != end; ++it) {
    // We try to render an indexed image. But if we find a sprite with
    // two or more palettes, or two of the sprites have different
    // palettes, we've to use RGB format.
    if (pixelFormat == IMAGE_INDEXED) {
      if (it->sprite()->pixelFormat() != IMAGE_INDEXED) {
        pixelFormat = IMAGE_RGB;
      }
      else if (it->sprite()->getPalettes().size() > 1) {
        pixelFormat = IMAGE_RGB;
      }
      else if (palette != NULL
        && palette->countDiff(it->sprite()->palette(frame_t(0)), NULL, NULL) > 0) {
        pixelFormat = IMAGE_RGB;
      }
      else
        palette = it->sprite()->palette(frame_t(0));
    }

    gfx::Rect sampleBounds = it->inTextureBounds();

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
  // the DocumentExporter::LayoutSamples() impl).
  if (m_textureWidth == 0) fullTextureBounds.w += m_borderPadding;
  if (m_textureHeight == 0) fullTextureBounds.h += m_borderPadding;

  base::UniquePtr<Sprite> sprite(
    Sprite::createBasicSprite(
      pixelFormat,
      fullTextureBounds.x+fullTextureBounds.w,
      fullTextureBounds.y+fullTextureBounds.h, maxColors));

  if (palette != NULL)
    sprite->setPalette(palette, false);

  base::UniquePtr<Document> document(new Document(sprite));
  sprite.release();

  return document.release();
}

void DocumentExporter::renderTexture(const Samples& samples, Image* textureImage)
{
  textureImage->clear(0);

  for (const auto& sample : samples) {
    if (sample.isDuplicated())
      continue;

    // Make the sprite compatible with the texture so the render()
    // works correctly.
    if (sample.sprite()->pixelFormat() != textureImage->pixelFormat()) {
      cmd::SetPixelFormat(
        sample.sprite(),
        textureImage->pixelFormat(),
        DitheringMethod::NONE).execute(UIContext::instance());
    }

    renderSample(sample, textureImage,
      sample.inTextureBounds().x+m_innerPadding,
      sample.inTextureBounds().y+m_innerPadding);
  }
}

void DocumentExporter::createDataFile(const Samples& samples, std::ostream& os, Image* textureImage)
{
  std::string frames_begin;
  std::string frames_end;
  bool filename_as_key = false;
  bool filename_as_attr = false;

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
       << "\"x\": " << frameBounds.x << ", "
       << "\"y\": " << frameBounds.y << ", "
       << "\"w\": " << frameBounds.w << ", "
       << "\"h\": " << frameBounds.h << " },\n"
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
     << "  \"scale\": \"" << m_scale << "\"";

  // meta.frameTags
  if (m_listFrameTags) {
    os << ",\n"
       << "  \"frameTags\": [";

    bool firstTag = true;
    for (auto& item : m_documents) {
      Document* doc = item.doc;
      Sprite* sprite = doc->sprite();

      for (FrameTag* tag : sprite->frameTags()) {
        if (firstTag)
          firstTag = false;
        else
          os << ",";
        os << "\n   { \"name\": \"" << escape_for_json(tag->name()) << "\","
           << " \"from\": " << tag->fromFrame() << ","
           << " \"to\": " << tag->toFrame() << " }";
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
      Document* doc = item.doc;
      Sprite* sprite = doc->sprite();

      std::vector<Layer*> layers;
      sprite->getLayersList(layers);

      for (Layer* layer : layers) {
        if (firstLayer)
          firstLayer = false;
        else
          os << ",";
        os << "\n   { \"name\": \"" << escape_for_json(layer->name()) << "\" }";
      }
    }
    os << "\n  ]";
  }

  os << "\n }\n"
     << "}\n";
}

void DocumentExporter::renderSample(const Sample& sample, doc::Image* dst, int x, int y)
{
  render::Render render;
  gfx::Clip clip(x, y, sample.trimmedBounds());

  if (sample.layer()) {
    render.renderLayer(dst, sample.layer(), sample.frame(), clip);
  }
  else {
    render.renderSprite(dst, sample.sprite(), sample.frame(), clip);
  }
}

} // namespace app
