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

#ifndef VACA_APPLICATION_H
#define VACA_APPLICATION_H

#include "Vaca/base.h"
#include <vector>

namespace Vaca {

  namespace details {
    class VACA_DLL MainArgs
    {
    public:
      static void setArgs(int, char**);
    };
  }

/**
   The main class of Vaca: initializes and destroys the GUI library
   resources.

   A program that uses Vaca library must to create one instance of
   this class, or an instance of a derived class.
*/
class VACA_DLL Application
{
public:

  Application();
  virtual ~Application();

  static size_t getArgc();
  static const String& getArgv(size_t i);
  static const std::vector<String>& getArgs();

  static Application* getInstance();

private:

  friend class details::MainArgs;

  /**
     The singleton, the only instance of Application (or a class
     derived from Application) that a program can contain.
   */
  static Application* m_instance;

  static std::vector<String> m_args;

  static void setArgs(const std::vector<String>& args);

};

} // namespace Vaca

#endif // VACA_APPLICATION_H
