/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <allegro/unicode.h>
#include <stdio.h>

#include "ase_exception.h"
#include "console.h"
#include "tinyxml.h"

ase_exception::ase_exception(const char* msg, ...) throw()
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

ase_exception::ase_exception(const std::string& msg) throw()
{
  try {
    m_msg = msg;
  }
  catch (...) {
    // no throw
  }
}

ase_exception::ase_exception(TiXmlDocument* doc) throw()
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

ase_exception::~ase_exception() throw()
{
}

void ase_exception::show()
{
  Console console;
  console.printf("A problem has occurred.\n\nDetails:\n%s", what());
}

const char* ase_exception::what() const throw()
{
  return m_msg.c_str();
}
