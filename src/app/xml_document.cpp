// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/xml_document.h"

#include "app/xml_exception.h"
#include "base/file_handle.h"

#include "tinyxml.h"

namespace app {

using namespace base;

XmlDocumentRef open_xml(const std::string& filename)
{
  FileHandle file(open_file(filename, "rb"));
  if (!file)
    throw Exception("Error loading file: " + filename);

  // Try to load the XML file
  XmlDocumentRef doc(new TiXmlDocument());
  doc->SetValue(filename.c_str());
  if (!doc->LoadFile(file.get()))
    throw XmlException(doc.get());

  return doc;
}

void save_xml(XmlDocumentRef doc, const std::string& filename)
{
  FileHandle file(open_file(filename, "wb"));
  if (!file)
    throw Exception("Error loading file: " + filename);

  if (!doc->SaveFile(file.get()))
    throw XmlException(doc.get());
}

bool bool_attr_is_true(const TiXmlElement* elem, const char* attrName)
{
  const char* value = elem->Attribute(attrName);

  return (value != NULL) && (strcmp(value, "true") == 0);
}

} // namespace app
