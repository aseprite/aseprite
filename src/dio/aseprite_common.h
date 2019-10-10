// Aseprite Document IO Library
// Copyright (c) 2018-2019 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_ASEPRITE_COMMON_H_INCLUDED
#define DIO_ASEPRITE_COMMON_H_INCLUDED
#pragma once

#define ASE_FILE_MAGIC                      0xA5E0
#define ASE_FILE_FRAME_MAGIC                0xF1FA

#define ASE_FILE_FLAG_LAYER_WITH_OPACITY    1

#define ASE_FILE_CHUNK_FLI_COLOR2           4
#define ASE_FILE_CHUNK_FLI_COLOR            11
#define ASE_FILE_CHUNK_LAYER                0x2004
#define ASE_FILE_CHUNK_CEL                  0x2005
#define ASE_FILE_CHUNK_CEL_EXTRA            0x2006
#define ASE_FILE_CHUNK_COLOR_PROFILE        0x2007
#define ASE_FILE_CHUNK_MASK                 0x2016
#define ASE_FILE_CHUNK_PATH                 0x2017
#define ASE_FILE_CHUNK_TAGS                 0x2018
#define ASE_FILE_CHUNK_PALETTE              0x2019
#define ASE_FILE_CHUNK_USER_DATA            0x2020
#define ASE_FILE_CHUNK_SLICES               0x2021 // Deprecated chunk (used on dev versions only between v1.2-beta7 and v1.2-beta8)
#define ASE_FILE_CHUNK_SLICE                0x2022
#define ASE_FILE_CHUNK_TILESET              0x2023

#define ASE_FILE_LAYER_IMAGE                0
#define ASE_FILE_LAYER_GROUP                1

#define ASE_FILE_RAW_CEL                    0
#define ASE_FILE_LINK_CEL                   1
#define ASE_FILE_COMPRESSED_CEL             2

#define ASE_FILE_NO_COLOR_PROFILE           0
#define ASE_FILE_SRGB_COLOR_PROFILE         1
#define ASE_FILE_ICC_COLOR_PROFILE          2

#define ASE_COLOR_PROFILE_FLAG_GAMMA        1

#define ASE_PALETTE_FLAG_HAS_NAME           1

#define ASE_USER_DATA_FLAG_HAS_TEXT         1
#define ASE_USER_DATA_FLAG_HAS_COLOR        2

#define ASE_CEL_EXTRA_FLAG_PRECISE_BOUNDS   1

#define ASE_SLICE_FLAG_HAS_CENTER_BOUNDS    1
#define ASE_SLICE_FLAG_HAS_PIVOT_POINT      2

namespace dio {

struct AsepriteHeader {
  long pos;                 // TODO used by the encoder in app project

  uint32_t size;
  uint16_t magic;
  uint16_t frames;
  uint16_t width;
  uint16_t height;
  uint16_t depth;
  uint32_t flags;
  uint16_t speed;       // Deprecated, use "duration" of AsepriteFrameHeader
  uint32_t next;
  uint32_t frit;
  uint8_t transparent_index;
  uint8_t ignore[3];
  uint16_t ncolors;
  uint8_t pixel_width;
  uint8_t pixel_height;
  int16_t grid_x;
  int16_t grid_y;
  uint16_t grid_width;
  uint16_t grid_height;
};

struct AsepriteFrameHeader {
  uint32_t size;
  uint16_t magic;
  uint32_t chunks;
  uint16_t duration;
};

struct AsepriteChunk {
  int type;
  int start;
};

} // namespace dio

#endif
