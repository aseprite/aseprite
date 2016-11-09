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
#include "base/serialization.h"
#include "base/string.h"
#include "base/unique_ptr.h"
#include "doc/cancel_io.h"
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
static std::map<ObjectId, std::vector<std::string> > g_deleteFiles;

class Writer {
public:
  Writer(const std::string& dir, app::Document* doc, doc::CancelIO* cancel)
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

    for (FrameTag* frtag : spr->frameTags())
      if (!saveObject("frtag", frtag, &Writer::writeFrameTag))
        return false;

    for (Cel* cel : spr->uniqueCels()) {
      if (!saveObject("img", cel->image(), &Writer::writeImage))
        return false;

      if (!saveObject("celdata", cel->data(), &Writer::writeCelData))
        return false;
    }

    for (Cel* cel : spr->cels())
      if (!saveObject("cel", cel, &Writer::writeCel))
        return false;

    std::vector<Layer*> layers;
    spr->getLayersList(layers);
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

  bool writeDocumentFile(std::ofstream& s, app::Document* doc) {
    write32(s, doc->sprite()->id());
    write_string(s, doc->filename());
    return true;
  }

  bool writeSprite(std::ofstream& s, Sprite* spr) {
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

    // IDs of all frame tags
    write32(s, spr->frameTags().size());
    for (FrameTag* frtag : spr->frameTags())
      write32(s, frtag->id());

    return true;
  }

  bool writeLayerStructure(std::ofstream& s, Layer* lay) {
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

  bool writeFrameTag(std::ofstream& s, FrameTag* frameTag) {
    write_frame_tag(s, frameTag);
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

    TRACE(" - Saved %s #%d v%d\n", prefix, obj->id(), obj->version());
    return true;
  }

  void deleteOldVersions() {
    while (!m_deleteFiles.empty() && !isCanceled()) {
      std::string file = m_deleteFiles.back();
      m_deleteFiles.erase(m_deleteFiles.end()-1);

      try {
        TRACE(" - Deleting <%s>\n", file.c_str());
        base::delete_file(file);
      }
      catch (const std::exception&) {
        TRACE(" - Cannot delete <%s>\n", file.c_str());
      }
    }
  }

  std::string m_dir;
  app::Document* m_doc;
  ObjVersionsMap& m_objVersions;
  std::vector<std::string>& m_deleteFiles;
  doc::CancelIO* m_cancel;
};

} // anonymous namespace

//////////////////////////////////////////////////////////////////////
// Public API

bool write_document(const std::string& dir,
                    app::Document* doc,
                    doc::CancelIO* cancel)
{
  Writer writer(dir, doc, cancel);
  return writer.saveDocument();
}

void delete_document_internals(app::Document* doc)
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
