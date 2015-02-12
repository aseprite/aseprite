// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
