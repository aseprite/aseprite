// ASE base library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/exception.h"

#include <cstdio>
#include <cstdarg>

using namespace std;
using namespace base;

Exception::Exception() throw()
{
}

Exception::Exception(const char* format, ...) throw()
{
  try {
    if (!strchr(format, '%')) {
      m_msg = format;
    }
    else {
      va_list ap;
      va_start(ap, format);

      char buf[1024];		// TODO warning buffer overflow
      vsprintf(buf, format, ap);
      m_msg = buf;

      va_end(ap);
    }
  }
  catch (...) {
    // No throw
  }
}

Exception::Exception(const std::string& msg) throw()
{
  try {
    m_msg = msg;
  }
  catch (...) {
    // No throw
  }
}

Exception::~Exception() throw()
{
}

void Exception::setMessage(const char* msg) throw()
{
  try {
    m_msg = msg;
  }
  catch (...) {
    // No throw
  }
}

const char* Exception::what() const throw()
{
  return m_msg.c_str();
}
