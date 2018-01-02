// Aseprite Document IO Library
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "dio/decode_file.h"

#include "dio/aseprite_decoder.h"
#include "dio/decoder.h"
#include "dio/detect_format.h"
#include "dio/file_interface.h"
#include "doc/document.h"

#include <cassert>

namespace dio {

bool decode_file(DecodeDelegate* delegate,
                 FileInterface* f)
{
  assert(delegate);
  assert(f);

  std::vector<uint8_t> buf(8, 0);
  size_t n = f->readBytes(&buf[0], 8);
  FileFormat format = detect_format_by_file_content_bytes(&buf[0], n);
  f->seek(0);                // Rewind

  Decoder* decoder = nullptr;

  switch (format) {
    case FileFormat::ASE_ANIMATION: decoder = new AsepriteDecoder; break;
  }

  bool result = false;

  if (decoder) {
    decoder->initialize(delegate, f);
    result = decoder->decode();
    delete decoder;
  }

  return result;
}

} // namespace dio
