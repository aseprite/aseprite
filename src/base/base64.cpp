// Aseprite Base Library
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/base64.h"

#include "modp_b64.h"

namespace base {

void encode_base64(const buffer& input, std::string& output)
{
  size_t size = modp_b64_encode_len(input.size());
  output.resize(size);
  size = modp_b64_encode(&output[0], (const char*)&input[0], input.size());
  if (size != MODP_B64_ERROR)
    output.erase(size, std::string::npos);
  else
    output.clear();
}

void decode_base64(const std::string& input, buffer& output)
{
  output.resize(modp_b64_decode_len(input.size()));
  size_t size = modp_b64_decode((char*)&output[0], input.c_str(), input.size());
  if (size != MODP_B64_ERROR)
    output.resize(size);
  else
    output.resize(0);
}

} // namespace base
