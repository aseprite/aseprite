// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_MSK_FILE_H_INCLUDED
#define APP_UTIL_MSK_FILE_H_INCLUDED
#pragma once

namespace doc {
class Mask;
}

namespace app {

doc::Mask* load_msk_file(const char* filename);
int save_msk_file(const doc::Mask* mask, const char* filename);

} // namespace app

#endif
