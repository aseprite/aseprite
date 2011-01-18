// ASE base library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/split_string.h"

#include <algorithm>

namespace {

  struct is_separator {
    const base::string* separators;

    is_separator(const base::string* seps) : separators(seps) {
    }

    bool operator()(base::string::value_type chr)
    {
      for (base::string::const_iterator
	     it = separators->begin(),
	     end = separators->end(); it != end; ++it) {
	if (chr == *it)
	  return true;
      }
      return false;
    }
  };

}

void base::split_string(const base::string& string,
			std::vector<base::string>& parts,
			const base::string& separators)
{
  size_t elements = 1 + std::count_if(string.begin(), string.end(), is_separator(&separators));
  parts.reserve(elements);

  size_t beg = 0, end;
  while (true) {
    end = string.find_first_of(separators, beg);
    if (end != base::string::npos) {
      parts.push_back(string.substr(beg, end - beg));
      beg = end+1;
    }
    else {
      parts.push_back(string.substr(beg));
      break;
    }
  }
}
