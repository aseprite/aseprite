// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_XML_EXCEPTION_H_INCLUDED
#define APP_XML_EXCEPTION_H_INCLUDED
#pragma once

#include "base/exception.h"

class TiXmlDocument;

namespace app {

  class XmlException : public base::Exception {
  public:
    XmlException(const TiXmlDocument* doc) throw();
  };

} // namespace app

#endif
