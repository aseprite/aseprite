// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/crash/read_document.h"

#include "app/console.h"
#include "app/document.h"
#include "base/convert_to.h"
#include "base/exception.h"
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
#include "doc/subobjects_io.h"

#include <fstream>
#include <map>

namespace app {
namespace crash {

using namespace base::serialization;
using namespace base::serialization::little_endian;
using namespace doc;

#ifdef _WIN32
  #define IFSTREAM(dir, name, fn) \
    std::ifstream name(base::from_utf8(base::join_path(dir, fn)), std::ifstream::binary);
#else
  #define IFSTREAM(dir, name, fn) \
    std::ifstream name(base::join_path(dir, fn).c_str(), std::ifstream::binary);
#endif

namespace {

class Reader : public SubObjectsIO {
public:
  Reader(const std::string& dir)
    : m_sprite(nullptr)
    , m_dir(dir) {
  }

  Sprite* loadSprite(ObjectId sprId) {
    return loadObject<Sprite*>("spr", sprId, &Reader::readSprite);
  }

  ImageRef getImageRef(ObjectId imageId) {
    if (m_images.find(imageId) != m_images.end())
      return m_images[imageId];

    ImageRef image(loadObject<Image*>("img", imageId, &Reader::readImage));
    return m_images[imageId] = image;
  }

  CelDataRef getCelDataRef(ObjectId celdataId) {
    if (m_celdatas.find(celdataId) != m_celdatas.end())
      return m_celdatas[celdataId];

    CelDataRef celData(loadObject<CelData*>("celdata", celdataId, &Reader::readCelData));
    return m_celdatas[celdataId] = celData;
  }

private:
  template<typename T>
  T loadObject(const char* prefix, ObjectId id, T (Reader::*readMember)(std::ifstream&)) {
    TRACE(" - Restoring %s%d\n", prefix, id);

    IFSTREAM(m_dir, s, prefix + base::convert_to<std::string>(id));

    T obj = nullptr;
    if (s.good())
      obj = (this->*readMember)(s);

    if (obj) {
      TRACE(" - %s%d restored successfully\n", prefix, id);
      return obj;
    }
    else {
      TRACE(" - %s%d was not restored\n", prefix, id);
      Console().printf("Error loading object %s%d\n", prefix, id);
      return nullptr;
    }
  }

  Sprite* readSprite(std::ifstream& s) {
    PixelFormat format = (PixelFormat)read8(s);
    int w = read16(s);
    int h = read16(s);
    color_t transparentColor = read32(s);

    if (format != IMAGE_RGB &&
        format != IMAGE_INDEXED &&
        format != IMAGE_GRAYSCALE) {
      Console().printf("Invalid sprite format #%d\n", (int)format);
      return nullptr;
    }

    if (w < 1 || h < 1 || w > 0xfffff || h > 0xfffff) {
      Console().printf("Invalid sprite dimension %dx%d\n", w, h);
      return nullptr;
    }

    base::UniquePtr<Sprite> spr(new Sprite(format, w, h, 256));
    m_sprite = spr.get();
    spr->setTransparentColor(transparentColor);

    // Frame durations
    frame_t nframes = read32(s);
    if (nframes >= 1) {
      spr->setTotalFrames(nframes);
      for (frame_t fr=0; fr<nframes; ++fr) {
        int msecs = read32(s);
        spr->setFrameDuration(fr, msecs);
      }
    }
    else {
      Console().printf("Invalid number of frames #%d\n", nframes);
    }

    // Read layers
    int nlayers = read32(s);
    if (nlayers >= 1 && nlayers < 0xfffff) {
      for (int i = 0; i < nlayers; ++i) {
        ObjectId layId = read32(s);
        Layer* lay = loadObject<Layer*>("lay", layId, &Reader::readLayer);
        if (lay)
          spr->folder()->addLayer(lay);
      }
    }
    else {
      Console().printf("Invalid number of layers #%d\n", nlayers);
    }

    // Read palettes
    int npalettes = read32(s);
    if (npalettes >= 1 && npalettes < 0xfffff) {
      for (int i = 0; i < npalettes; ++i) {
        ObjectId palId = read32(s);
        Palette* pal = loadObject<Palette*>("pal", palId, &Reader::readPalette);
        if (pal)
          spr->setPalette(pal, true);
      }
    }

    return spr.release();
  }

  Layer* readLayer(std::ifstream& s) {
    LayerFlags flags = (LayerFlags)read32(s);
    ObjectType type = (ObjectType)read16(s);
    ASSERT(type == ObjectType::LayerImage);

    std::string name = read_string(s);

    if (type == ObjectType::LayerImage) {
      base::UniquePtr<LayerImage> lay(new LayerImage(m_sprite));
      lay->setName(name);
      lay->setFlags(flags);

      // Cels
      int ncels = read32(s);
      for (int i=0; i<ncels; ++i) {
        ObjectId celId = read32(s);
        Cel* cel = loadObject<Cel*>("cel", celId, &Reader::readCel);
        if (cel)
          lay->addCel(cel);
      }
      return lay.release();
    }
    else {
      Console().printf("Unable to load layer named '%s', type #%d\n",
        name.c_str(), (int)type);
      return nullptr;
    }
  }

  Cel* readCel(std::ifstream& s) {
    return read_cel(s, this, false);
  }

  CelData* readCelData(std::ifstream& s) {
    return read_celdata(s, this, false);
  }

  Image* readImage(std::ifstream& s) {
    return read_image(s, false);
  }

  Palette* readPalette(std::ifstream& s) {
    return read_palette(s);
  }

  Sprite* m_sprite;    // Used to pass the sprite in LayerImage() ctor
  std::string m_dir;
  std::map<ObjectId, ImageRef> m_images;
  std::map<ObjectId, CelDataRef> m_celdatas;
};

} // anonymous namespace

//////////////////////////////////////////////////////////////////////
// Public API

bool read_document_info(const std::string& dir, DocumentInfo& info)
{
  ObjectId sprId;

  if (base::is_file(base::join_path(dir, "doc"))) {
    IFSTREAM(dir, s, "doc");
    sprId = read32(s);
    info.filename = read_string(s);
  }
  else
    return false;

  IFSTREAM(dir, s, "spr" + base::convert_to<std::string>(sprId));
  info.format = (PixelFormat)read8(s);
  info.width = read16(s);
  info.height = read16(s);
  read32(s);                    // Ignore transparent color
  info.frames = read32(s);
  return true;
}

app::Document* read_document(const std::string& dir)
{
  if (!base::is_file(base::join_path(dir, "doc")))
    return nullptr;

  IFSTREAM(dir, s, "doc");
  ObjectId sprId = read32(s);
  std::string filename = read_string(s);

  Reader reader(dir);
  Sprite* spr = reader.loadSprite(sprId);
  if (spr) {
    app::Document* doc = new app::Document(spr);
    doc->setFilename(filename);
    doc->impossibleToBackToSavedState();
    return doc;
  }
  else {
    Console().printf("Unable to load sprite #%d\n", sprId);
    return nullptr;
  }
}

} // namespace crash
} // namespace app
