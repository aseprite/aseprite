// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/fstream_path.h"
#include "base/sha1.h"
#include "base/sha1_rfc3174.h"

#include <cassert>
#include <fstream>

namespace base {

Sha1::Sha1()
  : m_digest(20, 0)
{
}

Sha1::Sha1(const std::vector<uint8_t>& digest)
  : m_digest(digest)
{
  assert(digest.size() == HashSize);
}

// Calculates the SHA1 of the given file.
Sha1 Sha1::calculateFromFile(const std::string& fileName)
{
  using namespace std;

  ifstream file(FSTREAM_PATH(fileName), ios::in | ios::binary);
  if (!file.good())
    return Sha1();

  SHA1Context sha;
  SHA1Reset(&sha);

  unsigned char buf[1024];
  while (file.good()) {
    file.read((char*)buf, 1024);
    unsigned int len = (unsigned int)file.gcount();
    if (len > 0)
      SHA1Input(&sha, buf, len);
  }

  vector<uint8_t> digest(HashSize);
  SHA1Result(&sha, &digest[0]);

  return Sha1(digest);
}

bool Sha1::operator==(const Sha1& other) const
{
  return m_digest == other.m_digest;
}

bool Sha1::operator!=(const Sha1& other) const
{
  return m_digest != other.m_digest;
}

} // namespace base
