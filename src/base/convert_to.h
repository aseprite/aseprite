// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_CONVERT_TO_H_INCLUDED
#define BASE_CONVERT_TO_H_INCLUDED
#pragma once

#include "base/string.h"

namespace base {

  class Sha1;
  class Version;

  // Undefined convertion
  template<typename To, typename From>
  To convert_to(const From& from) {
    // As this function doesn't return a value, if a specialization is
    // not found, a compiler error will be thrown (which means that
    // the conversion isn't supported).

    // TODO Use a static_assert(false)
  }

  template<> int convert_to(const base::string& from);
  template<> base::string convert_to(const int& from);

  template<> Sha1 convert_to(const base::string& from);
  template<> base::string convert_to(const Sha1& from);

  template<> Version convert_to(const base::string& from);
  template<> base::string convert_to(const Version& from);

}

#endif
