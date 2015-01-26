// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_REPLACE_STRING_H_INCLUDED
#define BASE_REPLACE_STRING_H_INCLUDED
#pragma once

#include <string>

namespace base {

  void replace_string(
    std::string& subject,
    const std::string& replace_this,
    const std::string& with_that);

}

#endif
