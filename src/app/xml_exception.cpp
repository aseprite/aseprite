// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/xml_exception.h"

#include "fmt/format.h"
#include "tinyxml.h"

namespace app {

XmlException::XmlException(const TiXmlDocument* doc) throw()
{
  try {
    setMessage(
      fmt::format("Error in XML file '{}' (line {}, column {})\nError {}: {}",
                  doc->Value(), doc->ErrorRow(), doc->ErrorCol(),
                  doc->ErrorId(), doc->ErrorDesc()).c_str());
  }
  catch (...) {
    // No throw
  }
}

} // namespace app
