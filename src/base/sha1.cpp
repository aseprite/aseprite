// ASE base library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/sha1.h"
#include "base/sha1_rfc3174.h"

#include <fstream>
#include <cassert>

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

  ifstream file(fileName.c_str(), ios::in | ios::binary);
  if (!file.good())
    return Sha1();

  SHA1Context sha;
  SHA1Reset(&sha);

  unsigned char buf[1024];
  while (file.good()) {
    file.read((char*)buf, 1024);
    streamsize len = file.gcount();
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
