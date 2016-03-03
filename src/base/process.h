// Aseprite Base Library
// Copyright (c) 2015-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_PROCESS_H_INCLUDED
#define BASE_PROCESS_H_INCLUDED
#pragma once

#include "base/ints.h"

namespace base {

  typedef uint32_t pid;

  pid get_current_process_id();

  bool is_process_running(pid pid);

} // namespace base

#endif
