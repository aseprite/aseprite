// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/crash/document_io.h"

#include "app/document.h"
#include "base/convert_to.h"
#include "base/path.h"
#include "base/serialization.h"
#include "base/string.h"
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
  #define OFSTREAM(name, fn) \
    std::ofstream name(base::from_utf8(base::join_path(path, fn)));
#else
  #define OFSTREAM(name, fn) \
    std::ofstream name(base::join_path(path, fn));
#endif

namespace {

typedef std::map<ObjectId, ObjectVersion> Versions;
static std::map<ObjectId, Versions> g_documentObjects;

void write_sprite(std::ofstream& s, Sprite* spr)
{
  write32(s, spr->id());
  write32(s, spr->version());
  write32(s, spr->folder()->version());
  write32(s, spr->pixelFormat());
  write32(s, spr->width());
  write32(s, spr->height());
  write32(s, spr->transparentColor());

  // Frame durations
  write32(s, spr->totalFrames());
  for (frame_t fr = 0; fr < spr->totalFrames(); ++fr)
    write32(s, spr->frameDuration(fr));
}

void write_layer_structure(std::ofstream& s, Layer* lay)
{
  write32(s, lay->id());
  write_string(s, lay->name());

  write32(s, static_cast<int>(lay->flags())); // Flags
  write16(s, static_cast<int>(lay->type()));  // Type

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

template<typename T, typename WriteFunc>
void save_object(T* obj, const char* prefix, WriteFunc write_func, Versions& versions, const std::string& path)
{
  if (!obj->version())
    obj->incrementVersion();

  if (versions[obj->id()] != obj->version()) {
    OFSTREAM(s, prefix + base::convert_to<std::string>(obj->id()));
    write_func(s, obj);
    versions[obj->id()] = obj->version();

    TRACE(" - %s %d saved with version %d\n",
      prefix, obj->id(), obj->version());
  }
  else {
    TRACE(" - Ignoring %s %d (version %d already saved)\n",
      prefix, obj->id(), obj->version());
  }
}

} // anonymous namespace

void write_document(const std::string& path, app::Document* doc)
{
  Versions& versions = g_documentObjects[doc->id()];

  Sprite* spr = doc->sprite();
  save_object(spr, "spr", write_sprite, versions, path);

  //////////////////////////////////////////////////////////////////////
  // Create one stream for each object

  for (Palette* pal : spr->getPalettes())
    save_object(pal, "pal", write_palette, versions, path);

  for (FrameTag* frtag : spr->frameTags())
    save_object(frtag, "frtag", write_frame_tag, versions, path);

  std::vector<Layer*> layers;
  spr->getLayersList(layers);
  for (Layer* lay : layers)
    save_object(lay, "lay", write_layer_structure, versions, path);

  for (Cel* cel : spr->cels())
    save_object(cel, "cel", write_cel, versions, path);

  // Images (CelData)
  for (Cel* cel : spr->uniqueCels()) {
    save_object(cel->data(), "celdata", write_celdata, versions, path);
    save_object(cel->image(), "img", write_image, versions, path);
  }
}

} // namespace crash
} // namespace app
