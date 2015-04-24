// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/convert_to.h"
#include "base/sha1.h"
#include <cstdio>
#include <cstdlib>

namespace base {

template<> int convert_to(const std::string& from)
{
  return std::strtol(from.c_str(), NULL, 10);
}

template<> std::string convert_to(const int& from)
{
  char buf[32];
  std::sprintf(buf, "%d", from);
  return buf;
}

template<> uint32_t convert_to(const std::string& from)
{
  return std::strtoul(from.c_str(), NULL, 10);
}

template<> std::string convert_to(const uint32_t& from)
{
  char buf[32];
  std::sprintf(buf, "%u", from);
  return buf;
}

template<> double convert_to(const std::string& from)
{
  return std::strtod(from.c_str(), NULL);
}

template<> std::string convert_to(const double& from)
{
  char buf[32];
  std::sprintf(buf, "%g", from);
  return buf;
}

template<> Sha1 convert_to(const std::string& from)
{
  std::vector<uint8_t> digest(Sha1::HashSize);

  for (size_t i=0; i<Sha1::HashSize; ++i) {
    if (i*2+1 >= from.size())
      break;

    digest[i] = convert_to<int>(from.substr(i*2, 2));
  }

  return Sha1(digest);
}

template<> std::string convert_to(const Sha1& from)
{
  char buf[3];
  std::string res;
  res.reserve(2*Sha1::HashSize);

  for(int c=0; c<Sha1::HashSize; ++c) {
    sprintf(buf, "%02x", from[c]);
    res += buf;
  }

  return res;
}

} // namespace base
