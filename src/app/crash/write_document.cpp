// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/crash/write_document.h"

#include "app/crash/doc_format.h"
#include "app/crash/internals.h"
#include "app/crash/log.h"
#include "app/doc.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "base/serialization.h"
#include "base/string.h"
#include "doc/cancel_io.h"
#include "doc/cel.h"
#include "doc/cel_data_io.h"
#include "doc/cel_io.h"
#include "doc/cels_range.h"
#include "doc/frame.h"
#include "doc/image_io.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/palette.h"
#include "doc/palette_io.h"
#include "doc/slice.h"
#include "doc/slice_io.h"
#include "doc/sprite.h"
#include "doc/string_io.h"
#include "doc/tag.h"
#include "doc/tag_io.h"
#include "doc/tileset.h"
#include "doc/tileset_io.h"
#include "doc/tilesets.h"
#include "doc/user_data_io.h"
#include "fixmath/fixmath.h"

#include <fstream>
#include <map>

namespace app {
namespace crash {

using namespace base::serialization;
using namespace base::serialization::little_endian;
using namespace doc;

namespace {

static std::map<ObjectId, ObjVersionsMap> g_docVersions;
static std::map<ObjectId, base::paths> g_deleteFiles;

class Writer {
public:
  Writer(const std::string& dir, Doc* doc, doc::CancelIO* cancel)
    : m_dir(dir)
    , m_doc(doc)
    , m_objVersions(g_docVersions[doc->id()])
    , m_deleteFiles(g_deleteFiles[doc->id()])
    , m_cancel(cancel) {
  }

  bool saveDocument() {
    Sprite* spr = m_doc->sprite();

    // Save from objects without children (e.g. images), to aggregated
    // objects (e.g. cels, layers, etc.)

    for (Palette* pal : spr->getPalettes())
      if (!saveObject("pal", pal, &Writer::writePalette))
        return false;

    if (spr->hasTilesets()) {
      for (Tileset* tset : *spr->tilesets()) {
        // The tileset can be nullptr if it was erased (as we keep
        // empty spaces in the Tilesets array)
        if (tset) {
          if (!saveObject("tset", tset, &Writer::writeTileset))
            return false;
        }
      }
    }

    for (Tag* frtag : spr->tags())
      if (!saveObject("frtag", frtag, &Writer::writeFrameTag))
        return false;

    for (Slice* slice : spr->slices())
      if (!saveObject("slice", slice, &Writer::writeSlice))
        return false;

    // Get all layers (visible, hidden, subchildren, etc.)
    LayerList layers = spr->allLayers();

    // Save original cel data (skip links)
    for (Layer* lay : layers) {
      CelList cels;
      lay->getCels(cels);

      for (Cel* cel : cels) {
        if (cel->link())        // Skip link
          continue;

        if (!saveObject("img", cel->image(), &Writer::writeImage))
          return false;

        if (!saveObject("celdata", cel->data(), &Writer::writeCelData))
          return false;
      }
    }

    // Save all cels (original and links)
    for (Layer* lay : layers) {
      CelList cels;
      lay->getCels(cels);

      for (Cel* cel : cels)
        if (!saveObject("cel", cel, &Writer::writeCel))
          return false;
    }

    // Save all layers (top level, groups, children, etc.)
    for (Layer* lay : layers)
      if (!saveObject("lay", lay, &Writer::writeLayerStructure))
        return false;

    if (!saveObject("spr", spr, &Writer::writeSprite))
      return false;

    if (!saveObject("doc", m_doc, &Writer::writeDocumentFile))
      return false;

    // Delete old files after all files are correctly saved.
    deleteOldVersions();
    return true;
  }

private:

  bool isCanceled() const {
    return (m_cancel && m_cancel->isCanceled());
  }

  bool writeDocumentFile(std::ofstream& s, Doc* doc) {
    write32(s, doc->sprite()->id());
    write_string(s, doc->filename());
    write16(s, DOC_FORMAT_VERSION_LAST);
    return true;
  }

  bool writeSprite(std::ofstream& s, Sprite* spr) {
    // Header
    write8(s, int(spr->colorMode()));
    write16(s, spr->width());
    write16(s, spr->height());
    write32(s, spr->transparentColor());
    write32(s, spr->totalFrames());

    // Frame durations
    for (frame_t fr = 0; fr < spr->totalFrames(); ++fr)
      write32(s, spr->frameDuration(fr));

    // IDs of all tilesets
    write32(s, spr->hasTilesets() ? spr->tilesets()->size(): 0);
    if (spr->hasTilesets()) {
      for (Tileset* tileset : *spr->tilesets()) {
        if (tileset)
          write32(s, tileset->id());
        else
          write32(s, 0);
      }
    }

    // IDs of all main layers
    write32(s, spr->allLayersCount());
    writeAllLayersID(s, 0, spr->root());

    // IDs of all palettes
    write32(s, spr->getPalettes().size());
    for (Palette* pal : spr->getPalettes())
      write32(s, pal->id());

    // IDs of all frame tags
    write32(s, spr->tags().size());
    for (Tag* frtag : spr->tags())
      write32(s, frtag->id());

    // IDs of all slices
    write32(s, spr->slices().size());
    for (const Slice* slice : spr->slices())
      write32(s, slice->id());

    // Color Space
    writeColorSpace(s, spr->colorSpace());

    // Grid bounds
    writeGridBounds(s, spr->gridBounds());

    // Write Sprite User Data
    write_user_data(s, spr->userData());

    return true;
  }

  bool writeGridBounds(std::ofstream& s, const gfx::Rect& grid) {
    write16(s, (int16_t)grid.x);
    write16(s, (int16_t)grid.y);
    write16(s, grid.w);
    write16(s, grid.h);
    return true;
  }

  bool writeColorSpace(std::ofstream& s, const gfx::ColorSpaceRef& colorSpace) {
    write16(s, colorSpace->type());
    write16(s, colorSpace->flags());
    write32(s, fixmath::ftofix(colorSpace->gamma()));

    auto& rawData = colorSpace->rawData();
    write32(s, rawData.size());
    if (rawData.size() > 0)
      s.write((const char*)&rawData[0], rawData.size());

    write_string(s, colorSpace->name());
    return true;
  }

  void writeAllLayersID(std::ofstream& s, ObjectId parentId, const LayerGroup* group) {
    for (const Layer* lay : group->layers()) {
      write32(s, lay->id());
      write32(s, parentId);

      if (lay->isGroup())
        writeAllLayersID(s, lay->id(), static_cast<const LayerGroup*>(lay));
    }
  }

  bool writeLayerStructure(std::ofstream& s, Layer* lay) {
    write32(s, static_cast<int>(lay->flags())); // Flags
    write16(s, static_cast<int>(lay->type()));  // Type
    write_string(s, lay->name());

    switch (lay->type()) {

      case ObjectType::LayerImage:
      case ObjectType::LayerTilemap: {
        // Tileset index
        if (lay->type() == ObjectType::LayerTilemap)
          write32(s, static_cast<const LayerTilemap*>(lay)->tilesetIndex());

        CelConstIterator it, begin = static_cast<const LayerImage*>(lay)->getCelBegin();
        CelConstIterator end = static_cast<const LayerImage*>(lay)->getCelEnd();

        // Blend mode & opacity
        write16(s, (int)static_cast<const LayerImage*>(lay)->blendMode());
        write8(s, static_cast<const LayerImage*>(lay)->opacity());

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

    // Save user data
    write_user_data(s, lay->userData());
    return true;
  }

  bool writeCel(std::ofstream& s, Cel* cel) {
    write_cel(s, cel);
    return true;
  }

  bool writeCelData(std::ofstream& s, CelData* celdata) {
    write_celdata(s, celdata);
    return true;
  }

  bool writeImage(std::ofstream& s, Image* img) {
    return write_image(s, img, m_cancel);
  }

  bool writePalette(std::ofstream& s, Palette* pal) {
    write_palette(s, pal);
    return true;
  }

  bool writeTileset(std::ofstream& s, Tileset* tileset) {
    write_tileset(s, tileset);
    return true;
  }

  bool writeFrameTag(std::ofstream& s, Tag* frameTag) {
    write_tag(s, frameTag);
    return true;
  }

  bool writeSlice(std::ofstream& s, Slice* slice) {
    write_slice(s, slice);
    return true;
  }

  template<typename T>
  bool saveObject(const char* prefix, T* obj, bool (Writer::*writeMember)(std::ofstream&, T*)) {
    if (isCanceled())
      return false;

    if (!obj->version())
      obj->incrementVersion();

    ObjVersions& versions = m_objVersions[obj->id()];
    if (versions.newer() == obj->version())
      return true;

    std::string fn = prefix;
    fn.push_back('-');
    fn += base::convert_to<std::string>(obj->id());

    std::string fullfn = base::join_path(m_dir, fn);
    std::string oldfn = fullfn + "." + base::convert_to<std::string>(versions.older());
    fullfn += "." + base::convert_to<std::string>(obj->version());

    std::ofstream s(FSTREAM_PATH(fullfn), std::ofstream::binary);
    write32(s, 0);                // Leave a room for the magic number
    if (!(this->*writeMember)(s, obj)) // Write the object
      return false;

    // Flush all data. In this way we ensure that the magic number is
    // the last thing being written in the file.
    s.flush();

    // Write the magic number
    s.seekp(0);
    write32(s, MAGIC_NUMBER);

    // Remove the older version
    if (versions.older() && base::is_file(oldfn))
      m_deleteFiles.push_back(oldfn);

    // Rotate versions and add the latest one
    versions.rotateRevisions(obj->version());

    RECO_TRACE(" - Saved %s #%d v%d\n", prefix, obj->id(), obj->version());
    return true;
  }

  void deleteOldVersions() {
    while (!m_deleteFiles.empty() && !isCanceled()) {
      std::string file = m_deleteFiles.back();
      m_deleteFiles.erase(m_deleteFiles.end()-1);

      try {
        RECO_TRACE(" - Deleting <%s>\n", file.c_str());
        base::delete_file(file);
      }
      catch (const std::exception&) {
        RECO_TRACE(" - Cannot delete <%s>\n", file.c_str());
      }
    }
  }

  std::string m_dir;
  Doc* m_doc;
  ObjVersionsMap& m_objVersions;
  base::paths& m_deleteFiles;
  doc::CancelIO* m_cancel;
};

} // anonymous namespace

//////////////////////////////////////////////////////////////////////
// Public API

bool write_document(const std::string& dir,
                    Doc* doc,
                    doc::CancelIO* cancel)
{
  Writer writer(dir, doc, cancel);
  return writer.saveDocument();
}

void delete_document_internals(Doc* doc)
{
  ASSERT(doc);

  // The document could not be inside g_documentObjects in case it was
  // never saved by the backup process.
  {
    auto it = g_docVersions.find(doc->id());
    if (it != g_docVersions.end())
      g_docVersions.erase(it);
  }
  {
    auto it = g_deleteFiles.find(doc->id());
    if (it != g_deleteFiles.end())
      g_deleteFiles.erase(it);
  }
}

} // namespace crash
} // namespace app
