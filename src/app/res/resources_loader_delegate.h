// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RES_RESOURCES_LOADER_DELEGATE_H_INCLUDED
#define APP_RES_RESOURCES_LOADER_DELEGATE_H_INCLUDED
#pragma once

#include <map>
#include <string>

namespace app {

class Resource;

class ResourcesLoaderDelegate {
public:
  virtual ~ResourcesLoaderDelegate() {}
  virtual void getResourcesPaths(std::map<std::string, std::string>& idAndPath) const = 0;
  virtual Resource* loadResource(const std::string& id, const std::string& path) = 0;
};

} // namespace app

#endif
