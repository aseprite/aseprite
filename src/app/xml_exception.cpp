// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/xml_exception.h"

#include "tinyxml.h"

#include <cstdio>

namespace app {

XmlException::XmlException(const TiXmlDocument* doc) throw()
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
