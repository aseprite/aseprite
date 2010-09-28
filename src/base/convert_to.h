// ASE base library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_CONVERT_TO_H_INCLUDED
#define BASE_CONVERT_TO_H_INCLUDED

#include "base/string.h"

namespace base {

  // Undefined convertion
  template<typename To, typename From>
  To convert_to(const From& from) {
    enum { not_supported = 1/(1 == 0) }; // static_assert(false)
  }

  template<> int convert_to(const base::string& from);
  template<> base::string convert_to(const int& from);

}

#endif
