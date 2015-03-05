// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/replace_string.h"

namespace base {

void replace_string(
  std::string& subject,
  const std::string& replace_this,
  const std::string& with_that)
{
  if (replace_this.empty())     // Do nothing case
    return;

  std::size_t i = 0;
  while (true) {
    i = subject.find(replace_this, i);
    if (i == std::string::npos)
      break;
    subject.replace(i, replace_this.size(), with_that);
    i += with_that.size();
  }
}

} // namespace base
