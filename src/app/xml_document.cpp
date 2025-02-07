// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/xml_document.h"

#include "app/xml_exception.h"
#include "base/file_handle.h"

#include "tinyxml2.h"

namespace app {

using namespace base;
using namespace tinyxml2;

XMLDocumentRef open_xml(const std::string& filename)
{
  FileHandle file(open_file(filename, "rb"));
  if (!file)
    throw Exception("Error loading file: " + filename);

  // Try to load the XML file
  auto doc = std::make_unique<XMLDocument>();
  if (doc->LoadFile(file.get()) != XML_SUCCESS)
    throw XmlException(filename, doc.get());

  return doc;
}

void save_xml(XMLDocument* doc, const std::string& filename)
{
  FileHandle file(open_file(filename, "wb"));
  if (!file) {
    // TODO add information about why the file couldn't be opened (errno?, win32?)
    throw Exception("Error loading file: " + filename);
  }

  if (doc->SaveFile(file.get()) != XML_SUCCESS)
    throw XmlException(filename, doc);
}

bool bool_attr(const XMLElement* elem, const char* attrName, bool defaultVal)
{
  const char* value = elem->Attribute(attrName);

  return value == NULL ? defaultVal : strcmp(value, "true") == 0;
}

} // namespace app
