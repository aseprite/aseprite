// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_XML_DOCUMENT_H_INCLUDED
#define APP_XML_DOCUMENT_H_INCLUDED
#pragma once

#include "base/exception.h"

#include "tinyxml.h"

#include <memory>
#include <string>

namespace app {

  typedef std::shared_ptr<TiXmlDocument> XmlDocumentRef;

  XmlDocumentRef open_xml(const std::string& filename);
  void save_xml(XmlDocumentRef doc, const std::string& filename);

  bool bool_attr_is_true(const TiXmlElement* elem, const char* attrName);

} // namespace app

#endif
