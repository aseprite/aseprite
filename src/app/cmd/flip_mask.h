// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_FLIP_MASK_H_INCLUDED
#define APP_CMD_FLIP_MASK_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_document.h"
#include "doc/algorithm/flip_type.h"

namespace app {
namespace cmd {
  using namespace doc;

  class FlipMask : public Cmd
                 , public WithDocument {
  public:
    FlipMask(Doc* doc, doc::algorithm::FlipType flipType);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    void swap();

    doc::algorithm::FlipType m_flipType;
  };

} // namespace cmd
} // namespace app

#endif
