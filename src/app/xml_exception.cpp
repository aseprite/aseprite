// Aseprite
// Copyright (C) 2023-2024  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/xml_exception.h"

#include "fmt/format.h"
#include "tinyxml2.h"

namespace app {

using namespace tinyxml2;

XmlException::XmlException(const std::string& filename, const XMLDocument* doc) noexcept
{
  try {
    setMessage(fmt::format("Error in XML file '{}' (line {})\nError {}: {}",
                           filename,
                           doc->ErrorLineNum(),
                           int(doc->ErrorID()),
                           doc->ErrorStr())
                 .c_str());
  }
  catch (...) {
    // No throw
  }
}

} // namespace app
