// Aseprite Document IO Library
// Copyright (C) 2018-2026  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_ASEPRITE_ENCODER_H_INCLUDED
#define DIO_ASEPRITE_ENCODER_H_INCLUDED
#pragma once

#include "base/uuid.h"
#include "dio/aseprite_common.h"
#include "dio/encoder.h"
#include "doc/layer.h"
#include "doc/tileset.h"

#include <string>

namespace doc {
class Mask;
class Palette;
class Slice;
class Slices;
class Sprite;
class Tags;
class Tilesets;
} // namespace doc

namespace dio {

class AsepriteEncoder : public Encoder {
public:
  bool encode() override;

private:
  class ChunkWriter {
  public:
    ChunkWriter(AsepriteEncoder* encoder, AsepriteFrameHeader* frame_header, int type)
      : m_encoder(encoder)
    {
      m_encoder->writeChunkStart(frame_header, type, &m_chunk);
    }
    ~ChunkWriter() { m_encoder->writeChunkEnd(&m_chunk); }

  private:
    AsepriteEncoder* m_encoder;
    AsepriteChunk m_chunk;
  };

  void prepareHeader(AsepriteHeader* header);
  void writeHeader(const AsepriteHeader* header);
  void writeHeaderFileSize(AsepriteHeader* header);

  void prepareFrameHeader(AsepriteFrameHeader* frame_header);
  void writeFrameHeader(AsepriteFrameHeader* frame_header);

  void writeLayers(const AsepriteHeader* header,
                   AsepriteFrameHeader* frame_header,
                   const AsepriteExternalFiles& ext_files,
                   const doc::Layer* layer,
                   int child_index);
  doc::layer_t writeCels(AsepriteFrameHeader* frame_header,
                         const AsepriteExternalFiles& ext_files,
                         const doc::Sprite* sprite,
                         const doc::Layer* layer,
                         doc::layer_t layer_index,
                         const doc::frame_t frame);

  void writePadding(size_t bytes);
  void writeString(const std::string& string);
  void writeFloat(const float value);
  void writeDouble(const double value);
  void writePoint(const gfx::Point& point);
  void writeSize(const gfx::Size& size);
  void writeUuid(const base::Uuid& uuid);

  void writeChunkStart(AsepriteFrameHeader* frame_header, int type, AsepriteChunk* chunk);
  void writeChunkEnd(AsepriteChunk* chunk);

  void writeColor2Chunk(AsepriteFrameHeader* frame_header, const doc::Palette* pal);
  void writePaletteChunk(AsepriteFrameHeader* frame_header,
                         const doc::Palette* pal,
                         int from,
                         int to);
  void writeLayerChunk(const AsepriteHeader* header,
                       AsepriteFrameHeader* frame_header,
                       const doc::Layer* layer,
                       int child_level);
  void writeCelChunk(AsepriteFrameHeader* frame_header,
                     const doc::Cel* cel,
                     const doc::LayerImage* layer,
                     const doc::layer_t layer_index);
  void writeCelExtraChunk(AsepriteFrameHeader* frame_header, const doc::Cel* cel);

  void writeColorProfile(AsepriteFrameHeader* frame_header, const doc::Sprite* sprite);

  void writeMaskChunk(AsepriteFrameHeader* frame_header, doc::Mask* mask);
  void writeTagsChunk(AsepriteFrameHeader* frame_header,
                      const doc::Tags* tags,
                      const doc::frame_t fromFrame,
                      const doc::frame_t toFrame);
  void writeSliceChunks(AsepriteFrameHeader* frame_header,
                        const AsepriteExternalFiles& ext_files,
                        const doc::Slices& slices,
                        const doc::frame_t fromFrame,
                        const doc::frame_t toFrame);
  void writeSliceChunk(AsepriteFrameHeader* frame_header,
                       doc::Slice* slice,
                       const doc::frame_t fromFrame,
                       const doc::frame_t toFrame);
  void writeUserDataChunk(AsepriteFrameHeader* frame_header,
                          const AsepriteExternalFiles& ext_files,
                          const doc::UserData* userData);

  void writeExternalFilesChunk(AsepriteFrameHeader* frame_header,
                               AsepriteExternalFiles& ext_files,
                               const doc::Sprite* sprite);

  void writeTilesetChunks(AsepriteFrameHeader* frame_header,
                          const AsepriteExternalFiles& ext_files,
                          const doc::Tilesets* tilesets);
  void writeTilesetChunk(AsepriteFrameHeader* frame_header,
                         const AsepriteExternalFiles& ext_files,
                         const doc::Tileset* tileset,
                         const doc::tileset_index si);

  void writePropertyValue(const doc::UserData::Variant& value);
  void writePropertiesMaps(const AsepriteExternalFiles& ext_files,
                           size_t nmaps,
                           const doc::UserData::PropertiesMaps& propertiesMaps);
};

} // namespace dio

#endif
