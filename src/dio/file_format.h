// Aseprite Document IO Library
// Copyright (c) 2021-2023 Igara Studio S.A.
// Copyright (c) 2016-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_FILE_FORMAT_H_INCLUDED
#define DIO_FILE_FORMAT_H_INCLUDED
#pragma once

namespace dio {

enum class FileFormat {
  ERROR = 0,
  UNKNOWN = 1,

  ASE_ANIMATION = 2, // Aseprite File Format
  ASE_PALETTE = 3,   // Adobe Swatch Exchange
  ACT_PALETTE = 4,
  BMP_IMAGE = 5,
  COL_PALETTE = 6,
  FLIC_ANIMATION = 7,
  GIF_ANIMATION = 8,
  GPL_PALETTE = 9,
  HEX_PALETTE = 10,
  ICO_IMAGES = 11,
  JPEG_IMAGE = 12,
  PAL_PALETTE = 13,
  PCX_IMAGE = 14,
  PNG_IMAGE = 15,
  SVG_IMAGE = 16,
  TARGA_IMAGE = 17,
  WEBP_ANIMATION = 18,
  CSS_STYLE = 19,
  PSD_IMAGE = 20,
  QOI_IMAGE = 21,

  FIRST_CUSTOM = 22,
  LAST_CUSTOM = 0x7fffffff
};

inline FileFormat register_custom_format()
{
  static int type = static_cast<int>(FileFormat::FIRST_CUSTOM);
  return static_cast<FileFormat>(type++);
}

} // namespace dio

#endif
