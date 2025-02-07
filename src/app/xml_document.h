// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_XML_DOCUMENT_H_INCLUDED
#define APP_XML_DOCUMENT_H_INCLUDED
#pragma once

#include "base/exception.h"

#include "tinyxml2.h"

#include <memory>
#include <string>

namespace app {

using XMLDocumentRef = std::unique_ptr<tinyxml2::XMLDocument>;

XMLDocumentRef open_xml(const std::string& filename);
void save_xml(tinyxml2::XMLDocument* doc, const std::string& filename);

bool bool_attr(const tinyxml2::XMLElement* elem, const char* attrName, bool defaultVal);

} // namespace app

#endif
