// LAF OS Library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_WIN_WINDOW_DDE_H_INCLUDED
#define OS_WIN_WINDOW_DDE_H_INCLUDED
#pragma once

#include <windows.h>

namespace os {

bool handle_dde_messages(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT& result);

} // namespace os

#endif
