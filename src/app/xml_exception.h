// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_XML_EXCEPTION_H_INCLUDED
#define APP_XML_EXCEPTION_H_INCLUDED
#pragma once

#include "base/exception.h"

namespace tinyxml2 {
class XMLDocument;
}

namespace app {

class XmlException : public base::Exception {
public:
  XmlException(const std::string& filename, const tinyxml2::XMLDocument* doc) noexcept;
};

} // namespace app

#endif
