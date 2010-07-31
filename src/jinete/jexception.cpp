/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <allegro.h>
#include <stdio.h>

#include "jinete/jexception.h"
#include "tinyxml.h"

jexception::jexception(const char* msg, ...) throw()
{
  try {
    if (!ustrchr(msg, '%')) {
      m_msg = msg;
    }
    else {
      va_list ap;
      va_start(ap, msg);

      char buf[1024];		// TODO warning buffer overflow
      uvsprintf(buf, msg, ap);
      m_msg = buf;

      va_end(ap);
    }
  }
  catch (...) {
    // no throw
  }
}

jexception::jexception(const std::string& msg) throw()
{
  try {
    m_msg = msg;
  }
  catch (...) {
    // no throw
  }
}

jexception::jexception(TiXmlDocument* doc) throw()
{
  try {
    char buf[1024];
    usprintf(buf, "Error in XML file '%s' (line %d, column %d)\nError %d: %s",
	     doc->Value(), doc->ErrorRow(), doc->ErrorCol(),
	     doc->ErrorId(), doc->ErrorDesc());

    m_msg = buf;
  }
  catch (...) {
    // no throw
  }
}

jexception::~jexception() throw()
{
}

void jexception::show()
{
  allegro_message("A problem has occurred.\n\nDetails:\n%s", what());
}

const char* jexception::what() const throw()
{
  return m_msg.c_str();
}
