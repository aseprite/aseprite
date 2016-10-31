// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_LOG_H_INCLUDED
#define BASE_LOG_H_INCLUDED
#pragma once

#ifdef ERROR
#undef ERROR
#endif

enum LogLevel {
  NONE    = 0, // Default log level: do not log
  FATAL   = 1, // Something failed and we CANNOT continue the execution
  ERROR   = 2, // Something failed, the UI should show this, and we can continue
  WARNING = 3, // Something failed, the UI don't need to show this, and we can continue
  INFO    = 4, // Information about some important event
  VERBOSE = 5, // Information step by step
};

#ifdef __cplusplus
#include <iosfwd>

namespace base {

  void set_log_filename(const char* filename);
  void set_log_level(LogLevel level);

  std::ostream& get_log_stream(LogLevel level);

} // namespace base

// E.g. LOG("text in information log level\n");
std::ostream& LOG(const char* format, ...);

// E.g. LOG(INFO) << "some information\n";
inline std::ostream& LOG(LogLevel level) {
  return base::get_log_stream(level);
}

#endif

#endif
