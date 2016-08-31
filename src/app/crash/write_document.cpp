// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/crash/write_document.h"

#include "app/crash/internals.h"
#include "app/document.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/fstream_path.h"
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

namespace {

static std::map<ObjectId, ObjVersionsMap> g_docVersions;

class Writer {
public:
  Writer(const std::string& dir, app::Document* doc)
    : m_dir(dir)
    , m_doc(doc)
    , m_objVersions(g_docVersions[doc->id()]) {
  }

  void saveDocument() {
    Sprite* spr = m_doc->sprite();

    // Save from objects without children (e.g. images), to aggregated
    // objects (e.g. cels, layers, etc.)

    for (Palette* pal : spr->getPalettes())
      saveObject("pal", pal, &Writer::writePalette);

    for (FrameTag* frtag : spr->frameTags())
      saveObject("frtag", frtag, &Writer::writeFrameTag);

    for (Cel* cel : spr->uniqueCels()) {
      saveObject("img", cel->image(), &Writer::writeImage);
      saveObject("celdata", cel->data(), &Writer::writeCelData);
    }

    for (Cel* cel : spr->cels())
      saveObject("cel", cel, &Writer::writeCel);

    // Save all layers (top level, groups, children, etc.)
    LayerList layers = spr->allLayers();
    for (Layer* lay : layers)
      saveObject("lay", lay, &Writer::writeLayerStructure);

    saveObject("spr", spr, &Writer::writeSprite);
    saveObject("doc", m_doc, &Writer::writeDocumentFile);
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
    write32(s, spr->allLayersCount());
    writeAllLayersID(s, 0, spr->root());

    // IDs of all palettes
    write32(s, spr->getPalettes().size());
    for (Palette* pal : spr->getPalettes())
      write32(s, pal->id());

    // IDs of all frame tags
    write32(s, spr->frameTags().size());
    for (FrameTag* frtag : spr->frameTags())
      write32(s, frtag->id());
  }

  void writeAllLayersID(std::ofstream& s, ObjectId parentId, const LayerGroup* group) {
    for (const Layer* lay : group->layers()) {
      write32(s, lay->id());
      write32(s, parentId);

      if (lay->isGroup())
        writeAllLayersID(s, lay->id(), static_cast<const LayerGroup*>(lay));
    }
  }

  void writeLayerStructure(std::ofstream& s, Layer* lay) {
    write32(s, static_cast<int>(lay->flags())); // Flags
    write16(s, static_cast<int>(lay->type()));  // Type
    write_string(s, lay->name());

    switch (lay->type()) {

      case ObjectType::LayerImage: {
        CelConstIterator it, begin = static_cast<const LayerImage*>(lay)->getCelBegin();
        CelConstIterator end = static_cast<const LayerImage*>(lay)->getCelEnd();

        // Cels
        write32(s, static_cast<const LayerImage*>(lay)->getCelsCount());
        for (it=begin; it != end; ++it) {
          const Cel* cel = *it;
          write32(s, cel->id());
        }
        break;
      }

      case ObjectType::LayerGroup:
        // Do nothing (the layer parent/children structure is saved in
        // writeSprite/writeAllLayersID() functions)
        break;
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

    ObjVersions& versions = m_objVersions[obj->id()];
    if (versions.newer() == obj->version())
      return;

    std::string fn = prefix;
    fn.push_back('-');
    fn += base::convert_to<std::string>(obj->id());

    std::string fullfn = base::join_path(m_dir, fn);
    std::string oldfn = fullfn + "." + base::convert_to<std::string>(versions.older());
    fullfn += "." + base::convert_to<std::string>(obj->version());

    std::ofstream s(FSTREAM_PATH(fullfn), std::ofstream::binary);
    write32(s, 0);                // Leave a room for the magic number
    (this->*writeMember)(s, obj); // Write the object

    // Flush all data. In this way we ensure that the magic number is
    // the last thing being written in the file.
    s.flush();

    // Write the magic number
    s.seekp(0);
    write32(s, MAGIC_NUMBER);

    // Remove the older version
    try {
      if (versions.older() && base::is_file(oldfn))
        base::delete_file(oldfn);
    }
    catch (const std::exception&) {
      TRACE(" - Cannot delete %s #%d v%d\n", prefix, obj->id(), versions.older());
    }

    // Rotate versions and add the latest one
    versions.rotateRevisions(obj->version());

    TRACE(" - Saved %s #%d v%d\n", prefix, obj->id(), obj->version());
  }

  std::string m_dir;
  app::Document* m_doc;
  ObjVersionsMap& m_objVersions;
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
  auto it = g_docVersions.find(doc->id());

  // The document could not be inside g_documentObjects in case it was
  // never saved by the backup process.
  if (it != g_docVersions.end())
    g_docVersions.erase(it);
}

} // namespace crash
} // namespace app
