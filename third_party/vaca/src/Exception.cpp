// Vaca - Visual Application Components Abstraction
// Copyright (c) 2005-2009 David Capello
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in
//   the documentation and/or other materials provided with the
//   distribution.
// * Neither the name of the author nor the names of its contributors
//   may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#include "Vaca/Exception.h"
#include "Vaca/String.h"

#ifdef VACA_ON_WINDOWS
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <lmerr.h>
  #include <wininet.h>
#endif

using namespace Vaca;

/**
   Creates generic exception with an empty error message.
*/
Exception::Exception()
  : std::exception()
{
  initialize();
}

/**
   Creates an exception with the specified error message.

   @param message Error message.
*/
Exception::Exception(const String& message)
  : std::exception()
  , m_message(message)
{
  initialize();
}

/**
   Destroys the exception.
*/
Exception::~Exception() throw()
{
}

/**
   Returns a printable C-string of the error.

   It could contain more information about the error than just the
   specified "message" in the constructor.

   @win32
   The @msdn{GetLastError} is used to get more information about
   the error.
   @endwin32
*/
const char* Exception::what() const throw()
{
  return m_what.c_str();
}

/**
   Returns the message specified in the constructor or an empty string
   if the default constructor was used.
 */
const String& Exception::getMessage() const throw()
{
  return m_message;
}

void Exception::initialize()
{
#ifdef VACA_ON_WINDOWS
  HMODULE hmodule = NULL;
  DWORD flags =
    FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS;
  LPSTR msgbuf = NULL;

  m_errorCode = GetLastError();

  // is it an network-error?
  if (m_errorCode >= NERR_BASE && m_errorCode <= MAX_NERR) {
    hmodule = LoadLibraryExA("netmsg.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hmodule)
      flags |= FORMAT_MESSAGE_FROM_HMODULE;
  }
  else if (m_errorCode >= INTERNET_ERROR_BASE && m_errorCode <= INTERNET_ERROR_LAST) {
    hmodule = LoadLibraryExA("wininet.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hmodule)
      flags |= FORMAT_MESSAGE_FROM_HMODULE;
  }

  // get the error message in ASCII
  if (!FormatMessageA(flags,
		      hmodule,
		      m_errorCode,
		      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		      reinterpret_cast<LPSTR>(&msgbuf),
		      0, NULL)) {
    // error we can't load the string
    msgbuf = NULL;
  }

  if (hmodule)
    FreeLibrary(hmodule);

  m_what += convert_to<std::string>(format_string(L"%d", m_errorCode));
  m_what += " - ";
  if (msgbuf) {
    m_what += msgbuf;
    LocalFree(msgbuf);
  }
  m_what += convert_to<std::string>(m_message);
#endif
}
