// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/log.h"

#include "base/debug.h"
#include "base/fstream_path.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

namespace {

class nullbuf : public std::streambuf {
protected:
  int_type overflow(int_type ch) override {
    return traits_type::not_eof(ch);
  }
};

class nullstream : public std::ostream {
public:
  nullstream()
    : std::basic_ios<char_type, traits_type>(&m_buf)
    , std::ostream(&m_buf) { }
private:
  nullbuf m_buf;
};

LogLevel log_level = LogLevel::NONE;
nullstream null_stream;
std::ofstream log_stream;
std::string log_filename;

bool open_log_stream()
{
  if (!log_stream.is_open()) {
    if (log_filename.empty())
      return false;

    log_stream.open(FSTREAM_PATH(log_filename));
  }
  return log_stream.is_open();
}

void log_text(const char* text)
{
  if (!open_log_stream())
    return;

  log_stream.write(text, strlen(text));
  log_stream.flush();
}

} // anonymous namespace

void base::set_log_filename(const char* filename)
{
  if (log_stream.is_open())
    log_stream.close();

  log_filename = filename;
}

void base::set_log_level(LogLevel level)
{
  log_level = level;
}

std::ostream& base::get_log_stream(LogLevel level)
{
  ASSERT(level != NONE);

  if ((log_level < level) ||
      (!log_stream.is_open() && !open_log_stream()))
    return null_stream;
  else
    return log_stream;
}

void LOG(const char* format, ...)
{
  if (log_level < INFO)
    return;

  char buf[2048];
  va_list ap;
  va_start(ap, format);
  std::vsnprintf(buf, sizeof(buf)-1, format, ap);
  log_text(buf);

#ifdef _DEBUG
  fputs(buf, stderr);
  fflush(stderr);
#endif

  va_end(ap);
}
