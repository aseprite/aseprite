// Aseprite Document Library
// Copyright (c) 2024 Igara Studio S.A.
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_USER_DATA_IO_H_INCLUDED
#define DOC_USER_DATA_IO_H_INCLUDED
#pragma once

#include "doc/serial_format.h"

#include <iosfwd>

namespace doc {

class UserData;

void write_user_data(std::ostream& os, const UserData& userData);

UserData read_user_data(std::istream& is, SerialFormat serial = SerialFormat::LastVer);

} // namespace doc

#endif
