// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/split_string.h"

#include <algorithm>

namespace {

  struct is_separator {
    const std::string* separators;

    is_separator(const std::string* seps) : separators(seps) {
    }

    bool operator()(std::string::value_type chr)
    {
      for (std::string::const_iterator
             it = separators->begin(),
             end = separators->end(); it != end; ++it) {
        if (chr == *it)
          return true;
      }
      return false;
    }
  };

}

void base::split_string(const std::string& string,
                        std::vector<std::string>& parts,
                        const std::string& separators)
{
  std::size_t elements = 1 + std::count_if(string.begin(), string.end(), is_separator(&separators));
  parts.reserve(elements);

  std::size_t beg = 0, end;
  while (true) {
    end = string.find_first_of(separators, beg);
    if (end != std::string::npos) {
      parts.push_back(string.substr(beg, end - beg));
      beg = end+1;
    }
    else {
      parts.push_back(string.substr(beg));
      break;
    }
  }
}
