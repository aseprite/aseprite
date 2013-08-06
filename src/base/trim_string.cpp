// ASEPRITE base library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/trim_string.h"
#include <cctype>

void base::trim_string(const base::string& input, base::string& output)
{
  size_t i, j;

  for (i=0; i<input.size(); ++i)
    if (!std::isspace(input.at(i)))
      break;

  for (j=input.size()-1; j>i; --j)
    if (!std::isspace(input.at(j)))
      break;

  if (i < j)
    output = input.substr(i, j - i + 1);
  else
    output = "";
}
