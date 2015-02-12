// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_RES_RESOURCE_H_INCLUDED
#define APP_RES_RESOURCE_H_INCLUDED
#pragma once

namespace app {

  class Resource {
  public:
    virtual ~Resource() { }
    virtual const std::string& name() const = 0;
  };

} // namespace app

#endif
