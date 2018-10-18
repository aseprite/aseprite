// Aseprite Document IO Library
// Copyright (c) 2018 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_ASEPRITE_DECODER_H_INCLUDED
#define DIO_ASEPRITE_DECODER_H_INCLUDED
#pragma once

#include "dio/decoder.h"
#include "doc/frame.h"
#include "doc/frame_tags.h"
#include "doc/layer_list.h"
#include "doc/pixel_format.h"
#include "doc/slices.h"

#include <string>

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

class AsepriteDecoder : public Decoder {
public:
  bool decode() override;

private:
  bool readHeader(AsepriteHeader* header);
  void readFrameHeader(AsepriteFrameHeader* frame_header);
  void readPadding(const int bytes);
  std::string readString();
  doc::Palette* readColorChunk(doc::Palette* prevPal, doc::frame_t frame);
  doc::Palette* readColor2Chunk(doc::Palette* prevPal, doc::frame_t frame);
  doc::Palette* readPaletteChunk(doc::Palette* prevPal, doc::frame_t frame);
  doc::Layer* readLayerChunk(AsepriteHeader* header, doc::Sprite* sprite, doc::Layer** previous_layer, int* current_level);
  doc::Cel* readCelChunk(doc::Sprite* sprite,
                         doc::LayerList& allLayers,
                         doc::frame_t frame,
                         doc::PixelFormat pixelFormat,
                         AsepriteHeader* header,
                         size_t chunk_end);
  void readCelExtraChunk(doc::Cel* cel);
  void readColorProfile(doc::Sprite* sprite);
  doc::Mask* readMaskChunk();
  void readFrameTagsChunk(doc::FrameTags* frameTags);
  void readSlicesChunk(doc::Slices& slices);
  doc::Slice* readSliceChunk(doc::Slices& slices);
  void readUserDataChunk(doc::UserData* userData);
};

} // namespace dio

#endif
