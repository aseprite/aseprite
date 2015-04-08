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
    std::ofstream name(base::join_path(dir, fn), std::ofstream::binary);
#endif

namespace {

typedef std::map<ObjectId, ObjectVersion> Versions;
static std::map<ObjectId, Versions> g_documentObjects;

template<typename T, typename WriteFunc>
void save_object(const char* prefix, T* obj, WriteFunc write_func, Versions& versions, const std::string& dir)
{
  if (!obj->version())
    obj->incrementVersion();

  if (versions[obj->id()] != obj->version()) {
    OFSTREAM(dir, s, prefix + base::convert_to<std::string>(obj->id()));
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

void write_sprite(std::ofstream& s, Sprite* spr)
{
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

void write_layer_structure(std::ofstream& s, Layer* lay)
{
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

} // anonymous namespace

//////////////////////////////////////////////////////////////////////
// Public API

void write_document(const std::string& dir, app::Document* doc)
{
  Versions& versions = g_documentObjects[doc->id()];
  Sprite* spr = doc->sprite();

  // Create a "doc" file with the main sprite ID
  if (!base::is_file(base::join_path(dir, "doc"))) {
    OFSTREAM(dir, s, "doc");
    write32(s, spr->id());
  }

  save_object("spr", spr, write_sprite, versions, dir);

  //////////////////////////////////////////////////////////////////////
  // Create one stream for each object

  for (Palette* pal : spr->getPalettes())
    save_object("pal", pal, write_palette, versions, dir);

  for (FrameTag* frtag : spr->frameTags())
    save_object("frtag", frtag, write_frame_tag, versions, dir);

  std::vector<Layer*> layers;
  spr->getLayersList(layers);
  for (Layer* lay : layers)
    save_object("lay", lay, write_layer_structure, versions, dir);

  for (Cel* cel : spr->cels())
    save_object("cel", cel, write_cel, versions, dir);

  // Images (CelData)
  for (Cel* cel : spr->uniqueCels()) {
    save_object("celdata", cel->data(), write_celdata, versions, dir);
    save_object("img", cel->image(), write_image, versions, dir);
  }
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
