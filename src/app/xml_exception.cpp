/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/xml_exception.h"

#include "tinyxml.h"

#include <cstdio>

namespace app {

XmlException::XmlException(TiXmlDocument* doc) throw()
{
  try {
    char buf[4096];             // TODO Overflow

    sprintf(buf, "Error in XML file '%s' (line %d, column %d)\nError %d: %s",
            doc->Value(), doc->ErrorRow(), doc->ErrorCol(),
            doc->ErrorId(), doc->ErrorDesc());

    setMessage(buf);
  }
  catch (...) {
    // No throw
  }
}

} // namespace app
