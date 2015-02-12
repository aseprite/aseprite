// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_RES_PALETTES_LOADER_DELEGATE_H_INCLUDED
#define APP_RES_PALETTES_LOADER_DELEGATE_H_INCLUDED
#pragma once

#include "app/res/resources_loader_delegate.h"

namespace app {

  class PalettesLoaderDelegate : public ResourcesLoaderDelegate {
  public:
    // ResourcesLoaderDelegate impl
    virtual std::string resourcesLocation() const override;
    virtual Resource* loadResource(const std::string& filename) override;
  };

} // namespace app

#endif
