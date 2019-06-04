// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RES_PALETTES_LOADER_DELEGATE_H_INCLUDED
#define APP_RES_PALETTES_LOADER_DELEGATE_H_INCLUDED
#pragma once

#include "app/file/file_op_config.h"
#include "app/res/resources_loader_delegate.h"

namespace app {

  class PalettesLoaderDelegate : public ResourcesLoaderDelegate {
  public:
    PalettesLoaderDelegate();

    // ResourcesLoaderDelegate impl
    virtual void getResourcesPaths(std::map<std::string, std::string>& idAndPath) const override;
    virtual Resource* loadResource(const std::string& id,
                                   const std::string& path) override;

  private:
    FileOpConfig m_config;
  };

} // namespace app

#endif
