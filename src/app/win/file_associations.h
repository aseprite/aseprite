// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_WIN_FILE_TYPE_ASSOCIATION_H_INCLUDED
#define APP_WIN_FILE_TYPE_ASSOCIATION_H_INCLUDED
#pragma once

#if !LAF_WINDOWS
  #error Only for Windows
#endif

#include <string>

namespace app { namespace win {

void associate_file_type_with_asepritefile_class(const std::string& extension);
void add_aseprite_to_open_with_file_type(const std::string& extension);

}} // namespace app::win

#endif
