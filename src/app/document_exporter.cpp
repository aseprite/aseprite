/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document_exporter.h"

#include "app/console.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/file/file.h"
#include "app/ui_context.h"
#include "base/path.h"
#include "base/unique_ptr.h"
#include "gfx/size.h"
#include "doc/cel.h"
#include "doc/dithering_method.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "doc/stock.h"

#include <cstdio>
#include <fstream>
#include <iostream>

using namespace doc;

namespace app {

class DocumentExporter::Sample {
public:
  Sample(Document* document, Sprite* sprite,
    FrameNumber frame, const std::string& filename) :
    m_document(document),
    m_sprite(sprite),
    m_frame(frame),
    m_filename(filename) {
  }

  Document* document() const { return m_document; }
  Sprite* sprite() const { return m_sprite; }
  FrameNumber frame() const { return m_frame; }
  std::string filename() const { return m_filename; }
  const gfx::Size& originalSize() const { return m_originalSize; }
  const gfx::Rect& trimmedBounds() const { return m_trimmedBounds; }
  const gfx::Rect& inTextureBounds() const { return m_inTextureBounds; }

  bool trimmed() const {
    return m_trimmedBounds.x > 0
      || m_trimmedBounds.y > 0
      || m_trimmedBounds.w != m_originalSize.w
      || m_trimmedBounds.h != m_originalSize.h;
  }

  void setOriginalSize(const gfx::Size& size) { m_originalSize = size; }
  void setTrimmedBounds(const gfx::Rect& bounds) { m_trimmedBounds = bounds; }
  void setInTextureBounds(const gfx::Rect& bounds) { m_inTextureBounds = bounds; }

private:
  Document* m_document;
  Sprite* m_sprite;
  FrameNumber m_frame;
  std::string m_filename;
  gfx::Size m_originalSize;
  gfx::Rect m_trimmedBounds;
  gfx::Rect m_inTextureBounds;
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
  virtual void layoutSamples(Samples& samples) = 0;
};

class DocumentExporter::SimpleLayoutSamples :
    public DocumentExporter::LayoutSamples {
public:
  void layoutSamples(Samples& samples) override {
    const Sprite* oldSprite = NULL;

    gfx::Point framePt(0, 0);
    for (Samples::iterator it=samples.begin(), end=samples.end();
         it != end; ++it) {
      const Sprite* sprite = it->sprite();
      gfx::Size size(sprite->width(), sprite->height());

      it->setOriginalSize(size);
      it->setTrimmedBounds(gfx::Rect(gfx::Point(0, 0), size));
      it->setInTextureBounds(gfx::Rect(framePt, size));

      // All frames of each sprite in one row.
      if (oldSprite != NULL && oldSprite != it->sprite()) {
        framePt.x = 0;
        framePt.y += size.h;
      }
      else {
        framePt.x += size.w;
      }

      oldSprite = it->sprite();
    }
  }
};

void DocumentExporter::exportSheet()
{
  // We output the metadata to std::cout if the user didn't specify a file.
  std::ofstream fos;
  std::streambuf* osbuf;
  if (m_dataFilename.empty())
    osbuf = std::cout.rdbuf();
  else {
    fos.open(m_dataFilename.c_str(), std::ios::out);
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
    return;
  }

  // 2) Layout those samples in a texture field.
  SimpleLayoutSamples layout;
  layout.layoutSamples(samples);

  // 3) Create and render the texture.
  base::UniquePtr<Document> textureDocument(
    createEmptyTexture(samples));

  Sprite* texture = textureDocument->sprite();
  Image* textureImage = static_cast<LayerImage*>(texture->folder()->getFirstLayer())
    ->getCel(FrameNumber(0))->image();

  renderTexture(samples, textureImage);

  // Save the metadata.
  createDataFile(samples, os, textureImage);

  // Save the image files.
  if (!m_textureFilename.empty()) {
    textureDocument->setFilename(m_textureFilename.c_str());
    save_document(UIContext::instance(), textureDocument.get());
  }
}

void DocumentExporter::captureSamples(Samples& samples)
{
  std::vector<char> buf(32);

  for (std::vector<Document*>::iterator
         it = m_documents.begin(),
         end = m_documents.end(); it != end; ++it) {
    Document* document = *it;
    Sprite* sprite = document->sprite();
    
    for (FrameNumber frame=FrameNumber(0);
         frame<sprite->totalFrames(); ++frame) {
      std::string filename = document->filename();

      if (sprite->totalFrames() > FrameNumber(1)) {
        int frameNumWidth =
          (sprite->totalFrames() < 10)? 1:
          (sprite->totalFrames() < 100)? 2:
          (sprite->totalFrames() < 1000)? 3: 4;
        std::sprintf(&buf[0], "%0*d", frameNumWidth, (int)frame);

        std::string path = base::get_file_path(filename);
        std::string title = base::get_file_title(filename);
        std::string ext = base::get_file_extension(filename);
        filename = base::join_path(path, title + &buf[0] + "." + ext);
      }

      samples.addSample(Sample(document, sprite, frame, filename));
    }
  }
}

Document* DocumentExporter::createEmptyTexture(const Samples& samples)
{
  Palette* palette = NULL;
  PixelFormat pixelFormat = IMAGE_INDEXED;
  gfx::Rect fullTextureBounds;
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
        && palette->countDiff(it->sprite()->getPalette(FrameNumber(0)), NULL, NULL) > 0) {
        pixelFormat = IMAGE_RGB;
      }
      else
        palette = it->sprite()->getPalette(FrameNumber(0));
    }

    fullTextureBounds = fullTextureBounds.createUnion(it->inTextureBounds());
  }

  base::UniquePtr<Sprite> sprite(Sprite::createBasicSprite(
      pixelFormat, fullTextureBounds.w, fullTextureBounds.h, maxColors));

  if (palette != NULL)
    sprite->setPalette(palette, false);

  base::UniquePtr<Document> document(new Document(sprite));
  sprite.release();

  return document.release();
}

void DocumentExporter::renderTexture(const Samples& samples, Image* textureImage)
{
  textureImage->clear(0);

  for (Samples::const_iterator
         it = samples.begin(),
         end = samples.end(); it != end; ++it) {
    // Make the sprite compatible with the texture so the render()
    // works correctly.
    if (it->sprite()->pixelFormat() != textureImage->pixelFormat()) {
      DocumentApi docApi(it->document(), NULL); // DocumentApi without undo
      docApi.setPixelFormat(it->sprite(), textureImage->pixelFormat(),
        DITHERING_NONE);
    }

    it->sprite()->render(textureImage,
      it->inTextureBounds().x - it->trimmedBounds().x,
      it->inTextureBounds().y - it->trimmedBounds().y,
      it->frame());
  }
}

void DocumentExporter::createDataFile(const Samples& samples, std::ostream& os, Image* textureImage)
{
  os << "{ \"frames\": {\n";
  for (Samples::const_iterator
         it = samples.begin(),
         end = samples.end(); it != end; ) {
    gfx::Size srcSize = it->originalSize();
    gfx::Rect spriteSourceBounds = it->trimmedBounds();
    gfx::Rect frameBounds = it->inTextureBounds();

    os << "   \"" << it->filename() << "\": {\n"
       << "    \"frame\": { "
       << "\"x\": " << frameBounds.x << ", "
       << "\"y\": " << frameBounds.y << ", "
       << "\"w\": " << frameBounds.w << ", "
       << "\"h\": " << frameBounds.h << " },\n"
       << "    \"rotated\": false,\n"
       << "    \"trimmed\": " << (it->trimmed() ? "true": "false") << ",\n"
       << "    \"spriteSourceSize\": { "
       << "\"x\": " << spriteSourceBounds.x << ", "
       << "\"y\": " << spriteSourceBounds.y << ", "
       << "\"w\": " << spriteSourceBounds.w << ", "
       << "\"h\": " << spriteSourceBounds.h << " },\n"
       << "    \"sourceSize\": { "
       << "\"w\": " << srcSize.w << ", "
       << "\"h\": " << srcSize.h << " },\n"
       << "    \"duration\": " << it->sprite()->getFrameDuration(it->frame()) << "\n"
       << "   }";

    if (++it != samples.end())
      os << ",\n";
    else
      os << "\n";
  }

  os << " },\n"
     << " \"meta\": {\n"
     << "  \"app\": \"" << WEBSITE << "\",\n"
     << "  \"version\": \"" << VERSION << "\",\n";
  if (!m_textureFilename.empty())
    os << "  \"image\": \"" << m_textureFilename.c_str() << "\",\n";
  os << "  \"format\": \"" << (textureImage->pixelFormat() == IMAGE_RGB ? "RGBA8888": "I8") << "\",\n"
     << "  \"size\": { "
     << "\"w\": " << textureImage->width() << ", "
     << "\"h\": " << textureImage->height() << " },\n"
     << "  \"scale\": \"" << m_scale << "\"\n"
     << " }\n"
     << "}\n";
}

} // namespace app
