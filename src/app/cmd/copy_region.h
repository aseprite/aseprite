// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_COPY_REGION_H_INCLUDED
#define APP_CMD_COPY_REGION_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_image.h"
#include "gfx/region.h"

#include <sstream>

namespace app {
namespace cmd {
  using namespace doc;

  class CopyRegion : public Cmd
                   , public WithImage {
  public:
    CopyRegion(Image* dst, const Image* src,
      const gfx::Region& region, int src_dx, int src_dy);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        (size_t)const_cast<std::stringstream*>(&m_stream)->tellp();
    }

  private:
    void swap();

    gfx::Region m_region;
    std::stringstream m_stream;
  };

} // namespace cmd
} // namespace app

#endif
