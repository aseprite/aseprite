// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/user_data_io.h"

#include "base/serialization.h"
#include "doc/string_io.h"
#include "doc/user_data.h"

#include <iostream>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

void write_user_data(std::ostream& os, const UserData& userData)
{
  write_string(os, userData.text());
}

UserData read_user_data(std::istream& is)
{
  UserData userData;
  userData.setText(read_string(is));
  return userData;
}

}
