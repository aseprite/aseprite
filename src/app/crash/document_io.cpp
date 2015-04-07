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

void write_document(const std::string& path, app::Document* doc)
{
  Sprite* spr = doc->sprite();
  std::vector<Layer*> layers;

  {
    OFSTREAM(os, "spr-" + base::convert_to<std::string>(spr->id()));

    write32(os, spr->id());
    write8(os, spr->pixelFormat());
    write32(os, spr->width());
    write32(os, spr->height());
    write32(os, spr->transparentColor());

    // Frame durations
    write32(os, spr->totalFrames());
    for (frame_t fr = 0; fr < spr->totalFrames(); ++fr)
      write32(os, spr->frameDuration(fr));

    // Palettes
    write32(os, spr->getPalettes().size());
    for (Palette* pal : spr->getPalettes())
      write32(os, pal->id());

    // Frame tags
    write32(os, spr->frameTags().size());
    for (FrameTag* tag : spr->frameTags())
      write32(os, tag->id());

    // Layers
    spr->getLayersList(layers);
    write32(os, layers.size());
    for (Layer* lay : layers)
      write32(os, lay->id());

    // Cels
    int celCount = 0;
    for (Cel* cel : spr->cels())
      ++celCount;
    write32(os, celCount);
    for (Cel* cel : spr->cels())
      write32(os, cel->id());

    // Images (CelData)
    int uniqueCelCount = 0;
    for (Cel* cel : spr->uniqueCels())
      ++uniqueCelCount;
    write32(os, uniqueCelCount);
    for (Cel* cel : spr->uniqueCels())
      write32(os, cel->data()->id());
  }

  //////////////////////////////////////////////////////////////////////
  // Create one stream for each object

  for (Palette* pal : spr->getPalettes()) {
    OFSTREAM(subos, "pal-" + base::convert_to<std::string>(pal->id()));
    write_palette(subos, pal);
  }

  for (FrameTag* tag : spr->frameTags()) {
    OFSTREAM(subos, "frtag-" + base::convert_to<std::string>(tag->id()));
    write_frame_tag(subos, tag);
  }

  for (Layer* lay : layers) {
    OFSTREAM(os, "lay-" + base::convert_to<std::string>(lay->id()));
    write32(os, lay->id());
    write_string(os, lay->name());

    write32(os, static_cast<int>(lay->flags())); // Flags
    write16(os, static_cast<int>(lay->type()));  // Type

    if (lay->type() == ObjectType::LayerImage) {
      CelConstIterator it, begin = static_cast<const LayerImage*>(lay)->getCelBegin();
      CelConstIterator end = static_cast<const LayerImage*>(lay)->getCelEnd();

      // Cels
      write32(os, static_cast<const LayerImage*>(lay)->getCelsCount());
      for (it=begin; it != end; ++it) {
        const Cel* cel = *it;
        write32(os, cel->id());
      }
    }
  }

  for (Cel* cel : spr->cels()) {
    OFSTREAM(os, "cel-" + base::convert_to<std::string>(cel->id()));
    write_cel(os, cel);
  }

  // Images (CelData)
  for (Cel* cel : spr->uniqueCels()) {
    OFSTREAM(os, "img-" + base::convert_to<std::string>(cel->data()->id()));
    write_celdata(os, cel->data());
    write_image(os, cel->image());
  }
}

} // namespace crash
} // namespace app
