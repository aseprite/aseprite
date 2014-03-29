// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef BASE_EXCEPTION_H_INCLUDED
#define BASE_EXCEPTION_H_INCLUDED
#pragma once

#include <exception>
#include <string>

namespace base {

  class Exception : public std::exception {
  public:
    Exception() throw();
    Exception(const char* format, ...) throw();
    Exception(const std::string& msg) throw();
    virtual ~Exception() throw();

    const char* what() const throw();

  protected:
    void setMessage(const char* msg) throw();

  private:
    std::string m_msg;
  };

}

#endif
