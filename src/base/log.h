// Aseprite Base Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_LOG_H_INCLUDED
#define BASE_LOG_H_INCLUDED
#pragma once

// Define BASE_DONT_DEFINE_LOG_MACRO in case that you don't need LOG
#ifndef BASE_DONT_DEFINE_LOG_MACRO
  #define LOG base_log
#endif

void base_set_log_filename(const char* filename);
void base_log(const char* format, ...);

#endif
