// ASE base library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_SPLIT_STRING_H_INCLUDED
#define BASE_SPLIT_STRING_H_INCLUDED

#include <vector>
#include <algorithm>

namespace base {

  namespace details {

    template<class T>
    struct is_separator {
      const T* separators;
      is_separator(const T* separators) : separators(separators) { }
      bool operator()(typename T::value_type chr)
      {
	for (typename T::const_iterator
	       it = separators->begin(),
	       end = separators->end(); it != end; ++it) {
	  if (chr == *it)
	    return true;
	}
	return false;
      }
    };

  }

  template<class T>
  void split_string(const T& string, std::vector<T>& parts, const T& separators)
  {
    size_t elements = 1 + std::count_if(string.begin(), string.end(), details::is_separator<T>(&separators));
    parts.reserve(elements);

    size_t beg = 0, end;
    while (true) {
      end = string.find_first_of(separators, beg);
      if (end != T::npos) {
	parts.push_back(string.substr(beg, end - beg));
	beg = end+1;
      }
      else {
	parts.push_back(string.substr(beg));
	break;
      }
    }
  }

}

#endif
