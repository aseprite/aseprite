// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_USER_DATA_IO_H_INCLUDED
#define DOC_USER_DATA_IO_H_INCLUDED
#pragma once

#include "app/crash/doc_format.h"

#include <iosfwd>

namespace doc {

  class UserData;

  void write_user_data(std::ostream& os, const UserData& userData);
  UserData read_user_data(std::istream& is, const int docFormatVer = DOC_FORMAT_VERSION_LAST);

} // namespace doc

#endif
