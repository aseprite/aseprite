// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/crash/write_document.h"

#include "app/document.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/path.h"
#include "base/serialization.h"
#include "base/string.h"
#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/cel_data_io.h"
#include "doc/cel_io.h"
#include "doc/cels_range.h"
#include "doc/frame.h"
#include "doc/frame_tag.h"
#include "doc/frame_tag_io.h"
#include "doc/image_io.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/palette_io.h"
#include "doc/sprite.h"
#include "doc/string_io.h"

#include <fstream>
#include <map>

namespace app {
namespace crash {

using namespace base::serialization;
using namespace base::serialization::little_endian;
using namespace doc;

#ifdef _WIN32
  #define OFSTREAM(dir, name, fn) \
    std::ofstream name(base::from_utf8(base::join_path(dir, fn)), std::ofstream::binary);
#else
  #define OFSTREAM(dir, name, fn) \
    std::ofstream name(base::join_path(dir, fn).c_str(), std::ofstream::binary);
#endif

namespace {

typedef std::map<ObjectId, ObjectVersion> Versions;
static std::map<ObjectId, Versions> g_documentObjects;

class Writer {
public:
  Writer(const std::string& dir, app::Document* doc)
    : m_dir(dir)
    , m_doc(doc)
    , m_versions(g_documentObjects[doc->id()]) {
  }

  void saveDocument() {
    Sprite* spr = m_doc->sprite();

    saveObject(nullptr, m_doc, &Writer::writeDocumentFile);
    saveObject("spr", spr, &Writer::writeSprite);

    //////////////////////////////////////////////////////////////////////
    // Create one stream for each object

    for (Palette* pal : spr->getPalettes())
      saveObject("pal", pal, &Writer::writePalette);

    for (FrameTag* frtag : spr->frameTags())
      saveObject("frtag", frtag, &Writer::writeFrameTag);

    std::vector<Layer*> layers;
    spr->getLayersList(layers);
    for (Layer* lay : layers)
      saveObject("lay", lay, &Writer::writeLayerStructure);

    for (Cel* cel : spr->cels())
      saveObject("cel", cel, &Writer::writeCel);

    // Images (CelData)
    for (Cel* cel : spr->uniqueCels()) {
      saveObject("celdata", cel->data(), &Writer::writeCelData);
      saveObject("img", cel->image(), &Writer::writeImage);
    }
  }

private:

  void writeDocumentFile(std::ofstream& s, app::Document* doc) {
    write32(s, doc->sprite()->id());
    write_string(s, doc->filename());
  }

  void writeSprite(std::ofstream& s, Sprite* spr) {
    write8(s, spr->pixelFormat());
    write16(s, spr->width());
    write16(s, spr->height());
    write32(s, spr->transparentColor());

    // Frame durations
    write32(s, spr->totalFrames());
    for (frame_t fr = 0; fr < spr->totalFrames(); ++fr)
      write32(s, spr->frameDuration(fr));

    // IDs of all main layers
    std::vector<Layer*> layers;
    spr->getLayersList(layers);
    write32(s, layers.size());
    for (Layer* lay : layers)
      write32(s, lay->id());

    // IDs of all palettes
    write32(s, spr->getPalettes().size());
    for (Palette* pal : spr->getPalettes())
      write32(s, pal->id());
  }

  void writeLayerStructure(std::ofstream& s, Layer* lay) {
    write32(s, static_cast<int>(lay->flags())); // Flags
    write16(s, static_cast<int>(lay->type()));  // Type
    write_string(s, lay->name());

    if (lay->type() == ObjectType::LayerImage) {
      CelConstIterator it, begin = static_cast<const LayerImage*>(lay)->getCelBegin();
      CelConstIterator end = static_cast<const LayerImage*>(lay)->getCelEnd();

      // Cels
      write32(s, static_cast<const LayerImage*>(lay)->getCelsCount());
      for (it=begin; it != end; ++it) {
        const Cel* cel = *it;
        write32(s, cel->id());
      }
    }
  }

  void writeCel(std::ofstream& s, Cel* cel) {
    write_cel(s, cel);
  }

  void writeCelData(std::ofstream& s, CelData* celdata) {
    write_celdata(s, celdata);
  }

  void writeImage(std::ofstream& s, Image* img) {
    write_image(s, img);
  }

  void writePalette(std::ofstream& s, Palette* pal) {
    write_palette(s, pal);
  }

  void writeFrameTag(std::ofstream& s, FrameTag* frameTag) {
    write_frame_tag(s, frameTag);
  }

  template<typename T>
  void saveObject(const char* prefix, T* obj, void (Writer::*writeMember)(std::ofstream&, T*)) {
    if (!obj->version())
      obj->incrementVersion();

    if (m_versions[obj->id()] != obj->version()) {
      std::string fn = (prefix ? prefix + base::convert_to<std::string>(obj->id()): "doc");

      OFSTREAM(m_dir, s, fn);
      (this->*writeMember)(s, obj);
      m_versions[obj->id()] = obj->version();

      TRACE(" - %s %d saved with version %d\n",
            prefix, obj->id(), obj->version());
    }
    else {
      TRACE(" - Ignoring %s %d (version %d already saved)\n",
            prefix, obj->id(), obj->version());
    }
  }

  std::string m_dir;
  app::Document* m_doc;
  Versions& m_versions;
};

} // anonymous namespace

//////////////////////////////////////////////////////////////////////
// Public API

void write_document(const std::string& dir, app::Document* doc)
{
  Writer writer(dir, doc);
  writer.saveDocument();
}

void delete_document_internals(app::Document* doc)
{
  ASSERT(doc);
  auto it = g_documentObjects.find(doc->id());

  // The document could not be inside g_documentObjects in case it was
  // never saved by the backup process.
  if (it != g_documentObjects.end())
    g_documentObjects.erase(it);
}

} // namespace crash
} // namespace app
