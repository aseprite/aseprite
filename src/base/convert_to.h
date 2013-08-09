// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef BASE_CONVERT_TO_H_INCLUDED
#define BASE_CONVERT_TO_H_INCLUDED

#include "base/string.h"

namespace base {

  class Sha1;
  class Version;

  // Undefined convertion
  template<typename To, typename From>
  To convert_to(const From& from) {
    enum { not_supported = 1/(1 == 0) }; // static_assert(false)
  }

  template<> int convert_to(const base::string& from);
  template<> base::string convert_to(const int& from);

  template<> Sha1 convert_to(const base::string& from);
  template<> base::string convert_to(const Sha1& from);

  template<> Version convert_to(const base::string& from);
  template<> base::string convert_to(const Version& from);

}

#endif
