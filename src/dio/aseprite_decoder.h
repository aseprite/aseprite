// Aseprite Document IO Library
// Copyright (c) 2018-2020 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_ASEPRITE_DECODER_H_INCLUDED
#define DIO_ASEPRITE_DECODER_H_INCLUDED
#pragma once

#include "dio/decoder.h"
#include "doc/frame.h"
#include "doc/layer_list.h"
#include "doc/pixel_format.h"
#include "doc/slices.h"
#include "doc/tags.h"
#include "doc/tileset.h"
#include "doc/user_data.h"

#include <string>
#include <vector>

namespace doc {
  class Cel;
  class Layer;
  class Layer;
  class Mask;
  class Palette;
  class Sprite;
  class UserData;
}

namespace dio {

struct AsepriteHeader;
struct AsepriteFrameHeader;
struct AsepriteExternalFiles;

class AsepriteDecoder : public Decoder {
public:
  bool decode() override;

private:
  bool readHeader(AsepriteHeader* header);
  void readFrameHeader(AsepriteFrameHeader* frame_header);
  void readPadding(const int bytes);
  std::string readString();
  float readFloat();
  double readDouble();
  doc::Palette* readColorChunk(doc::Palette* prevPal, doc::frame_t frame);
  doc::Palette* readColor2Chunk(doc::Palette* prevPal, doc::frame_t frame);
  doc::Palette* readPaletteChunk(doc::Palette* prevPal, doc::frame_t frame);
  doc::Layer* readLayerChunk(AsepriteHeader* header, doc::Sprite* sprite, doc::Layer** previous_layer, int* current_level);
  doc::Cel* readCelChunk(doc::Sprite* sprite,
                         doc::frame_t frame,
                         doc::PixelFormat pixelFormat,
                         const AsepriteHeader* header,
                         const size_t chunk_end);
  void readCelExtraChunk(doc::Cel* cel);
  void readColorProfile(doc::Sprite* sprite);
  void readExternalFiles(AsepriteExternalFiles& extFiles);
  doc::Mask* readMaskChunk();
  void readTagsChunk(doc::Tags* tags);
  void readSlicesChunk(doc::Slices& slices);
  doc::Slice* readSliceChunk(doc::Slices& slices);
  void readUserDataChunk(doc::UserData* userData,
                         const AsepriteExternalFiles& extFiles);
  doc::Tileset* readTilesetChunk(doc::Sprite* sprite,
                                 const AsepriteHeader* header,
                                 const AsepriteExternalFiles& extFiles);
  void readPropertiesMaps(doc::UserData::PropertiesMaps& propertiesMaps,
                          const AsepriteExternalFiles& extFiles);
  const doc::UserData::Variant readPropertyValue(uint16_t type);
  void readTilesData(doc::Tileset* tileset, const AsepriteExternalFiles& extFiles);

  doc::LayerList m_allLayers;
  std::vector<uint32_t> m_tilesetFlags;
};

} // namespace dio

#endif
