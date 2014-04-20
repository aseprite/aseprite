// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/trim_string.h"
#include <cctype>

void base::trim_string(const std::string& input, std::string& output)
{
  int i, j;

  for (i=0; i<(int)input.size(); ++i)
    if (!std::isspace(input.at(i)))
      break;

  for (j=(int)input.size()-1; j>i; --j)
    if (!std::isspace(input.at(j)))
      break;

  if (i < j)
    output = input.substr(i, j - i + 1);
  else
    output = "";
}
