// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_RES_RESOURCES_LOADER_DELEGATE_H_INCLUDED
#define APP_RES_RESOURCES_LOADER_DELEGATE_H_INCLUDED
#pragma once

#include <string>

namespace app {

  class Resource;

  class ResourcesLoaderDelegate {
  public:
    virtual ~ResourcesLoaderDelegate() { }
    virtual std::string resourcesLocation() const = 0;
    virtual Resource* loadResource(const std::string& filename) = 0;
  };

} // namespace app

#endif
