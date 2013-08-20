// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef BASE_SPLIT_STRING_H_INCLUDED
#define BASE_SPLIT_STRING_H_INCLUDED

#include "base/string.h"
#include <vector>

namespace base {

  void split_string(const base::string& string,
                    std::vector<base::string>& parts,
                    const base::string& separators);

}

#endif
