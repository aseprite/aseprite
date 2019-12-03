// Aseprite Code Generator
// Copyright (C) 2014-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef GEN_COMMON_H_INCLUDED
#define GEN_COMMON_H_INCLUDED
#pragma once

#include <cctype>
#include <string>

inline std::string convert_xmlid_to_cppid(const std::string& xmlid, bool firstLetterUpperCase)
{
  bool firstLetter = firstLetterUpperCase;
  std::string cppid;
  for (std::size_t i=0; i<xmlid.size(); ++i) {
    if (xmlid[i] == '_') {
      firstLetter = true;
    }
    else if (firstLetter) {
      firstLetter = false;
      cppid += std::toupper(xmlid[i]);
    }
    else
      cppid += xmlid[i];
  }
  return cppid;
}

#endif
