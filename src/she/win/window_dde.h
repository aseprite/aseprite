// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_WIN_WINDOW_DDE_H_INCLUDED
#define SHE_WIN_WINDOW_DDE_H_INCLUDED
#pragma once

#include <windows.h>

namespace she {

bool handle_dde_messages(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& result);

} // namespace she

#endif
