// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_FLIP_MASKED_CEL_H_INCLUDED
#define APP_CMD_FLIP_MASKED_CEL_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "doc/algorithm/flip_type.h"
#include "doc/color.h"

namespace app {
namespace cmd {
  using namespace doc;

  class FlipMaskedCel : public Cmd
                      , public WithCel {
  public:
    FlipMaskedCel(Cel* cel, doc::algorithm::FlipType flipType);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    void swap();

    doc::algorithm::FlipType m_flipType;
    color_t m_bgcolor;
  };

} // namespace cmd
} // namespace app

#endif
