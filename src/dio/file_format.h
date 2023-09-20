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
  ERROR = -1,
  UNKNOWN = 0,

  ASE_ANIMATION,                // Aseprite File Format
  ASE_PALETTE,                  // Adobe Swatch Exchange
  ACT_PALETTE,
  BMP_IMAGE,
  COL_PALETTE,
  FLIC_ANIMATION,
  GIF_ANIMATION,
  GPL_PALETTE,
  HEX_PALETTE,
  ICO_IMAGES,
  JPEG_IMAGE,
  PAL_PALETTE,
  PCX_IMAGE,
  PNG_IMAGE,
  SVG_IMAGE,
  TARGA_IMAGE,
  WEBP_ANIMATION,
  CSS_STYLE,
  PSD_IMAGE,
  QOI_IMAGE,
};

} // namespace dio

#endif
