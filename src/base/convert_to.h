// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_CONVERT_TO_H_INCLUDED
#define BASE_CONVERT_TO_H_INCLUDED
#pragma once

#include "base/base.h"
#include <string>

namespace base {

  class Sha1;

  // Undefined convertion
  template<typename To, typename From>
  To convert_to(const From& from) {
    static_assert(false && sizeof(To), "Invalid conversion");
  }

  template<> int convert_to(const std::string& from);
  template<> std::string convert_to(const int& from);

  template<> uint32_t convert_to(const std::string& from);
  template<> std::string convert_to(const uint32_t& from);

  template<> double convert_to(const std::string& from);
  template<> std::string convert_to(const double& from);

  template<> Sha1 convert_to(const std::string& from);
  template<> std::string convert_to(const Sha1& from);

}

#endif
