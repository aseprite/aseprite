// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_ERRNO_STRING_H_INCLUDED
#define BASE_ERRNO_STRING_H_INCLUDED
#pragma once

#include <string>

namespace base {

std::string get_errno_string(int errnum);

} // namespace base

#endif
