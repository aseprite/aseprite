// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RES_RESOURCE_H_INCLUDED
#define APP_RES_RESOURCE_H_INCLUDED
#pragma once

namespace app {

class Resource {
public:
  virtual ~Resource() {}
  virtual const std::string& id() const = 0;
  virtual const std::string& path() const = 0;
};

} // namespace app

#endif
