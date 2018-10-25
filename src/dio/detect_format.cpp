// Aseprite Document IO Library
// Copyright (c) 2016-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "dio/detect_format.h"

#include "base/file_handle.h"
#include "base/fs.h"
#include "base/string.h"
#include "flic/flic_details.h"

#include <cstring>

#define ASE_MAGIC_NUMBER 0xA5E0
#define BMP_MAGIC_NUMBER 0x4D42 // "BM"
#define JPG_MAGIC_NUMBER 0xD8FF
#define GIF_87_STAMP     "GIF87a"
#define GIF_89_STAMP     "GIF89a"
#define PNG_MAGIC_DWORD1 0x474E5089
#define PNG_MAGIC_DWORD2 0x0A1A0A0D

namespace dio {

FileFormat detect_format(const std::string& filename)
{
  FileFormat ff = detect_format_by_file_content(filename);
  if (ff == FileFormat::UNKNOWN)
    ff = detect_format_by_file_extension(filename);
  return ff;
}

FileFormat detect_format_by_file_content_bytes(const uint8_t* buf,
                                               const int n)
{
#define IS_MAGIC_WORD(offset, word)             \
  ((buf[offset+0] == (word & 0xff)) &&          \
   (buf[offset+1] == ((word & 0xff00) >> 8)))

#define IS_MAGIC_DWORD(offset, dword)                     \
  ((buf[offset+0] == (dword & 0xff)) &&                   \
   (buf[offset+1] == ((dword & 0xff00) >> 8)) &&          \
   (buf[offset+2] == ((dword & 0xff0000) >> 16)) &&       \
   (buf[offset+3] == ((dword & 0xff000000) >> 24)))

  if (n >= 2) {
    if (n >= 6) {
      if (n >= 8) {
        if (IS_MAGIC_DWORD(0, PNG_MAGIC_DWORD1) &&
            IS_MAGIC_DWORD(4, PNG_MAGIC_DWORD2))
          return FileFormat::PNG_IMAGE;
      }

      if (std::strncmp((const char*)buf, GIF_87_STAMP, 6) == 0 ||
          std::strncmp((const char*)buf, GIF_89_STAMP, 6) == 0)
        return FileFormat::GIF_ANIMATION;

      if (IS_MAGIC_WORD(4, ASE_MAGIC_NUMBER))
        return FileFormat::ASE_ANIMATION;

      if (IS_MAGIC_WORD(4, FLI_MAGIC_NUMBER) ||
          IS_MAGIC_WORD(4, FLC_MAGIC_NUMBER))
        return FileFormat::FLIC_ANIMATION;
    }

    if (IS_MAGIC_WORD(0, BMP_MAGIC_NUMBER))
      return FileFormat::BMP_IMAGE;

    if (IS_MAGIC_WORD(0, JPG_MAGIC_NUMBER))
      return FileFormat::JPEG_IMAGE;
  }

  return FileFormat::UNKNOWN;
}

FileFormat detect_format_by_file_content(const std::string& filename)
{
  base::FileHandle handle(base::open_file(filename.c_str(), "rb"));
  if (!handle)
    return FileFormat::ERROR;

  FILE* f = handle.get();
  uint8_t buf[8];
  int n = (int)fread(buf, 1, 8, f);

  return detect_format_by_file_content_bytes(buf, n);
}

FileFormat detect_format_by_file_extension(const std::string& filename)
{
  // By extension
  const std::string ext = base::string_to_lower(base::get_file_extension(filename));

  if (ext == "ase" ||
      ext == "aseprite")
    return FileFormat::ASE_ANIMATION;

  if (ext == "bmp")
    return FileFormat::BMP_IMAGE;

  if (ext == "col")
    return FileFormat::COL_PALETTE;

  if (ext == "flc" ||
      ext == "fli")
    return FileFormat::FLIC_ANIMATION;

  if (ext == "gif")
    return FileFormat::GIF_ANIMATION;

  if (ext == "gpl")
    return FileFormat::GPL_PALETTE;

  if (ext == "hex")
    return FileFormat::HEX_PALETTE;

  if (ext == "ico")
    return FileFormat::ICO_IMAGES;

  if (ext == "jpg" ||
      ext == "jpeg")
    return FileFormat::JPEG_IMAGE;

  if (ext == "pal")
    return FileFormat::PAL_PALETTE;

  if (ext == "pcx" ||
      ext == "pcc")
    return FileFormat::PCX_IMAGE;

  if (ext == "png")
    return FileFormat::PNG_IMAGE;
  
  if (ext == "svg")
    return FileFormat::SVG_IMAGE;

  if (ext == "tga")
    return FileFormat::TARGA_IMAGE;

  if (ext == "webp")
    return FileFormat::WEBP_ANIMATION;

  return FileFormat::UNKNOWN;
}

} // namespace dio
