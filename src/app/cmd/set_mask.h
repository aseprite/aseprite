// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_MASK_H_INCLUDED
#define APP_CMD_SET_MASK_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_document.h"

#include <memory>
#include <sstream>

namespace doc {
  class Mask;
}

namespace app {
namespace cmd {
  using namespace doc;

  class SetMask : public Cmd
                , public WithDocument {
  public:
    SetMask(Doc* doc, const Mask* newMask);

    // Used to change the new mask used in the onRedo()
    void setNewMask(const Mask* newMask);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override;

  private:
    void setMask(const Mask* mask);

    std::unique_ptr<Mask> m_oldMask;
    std::unique_ptr<Mask> m_newMask;
  };

} // namespace cmd
} // namespace app

#endif
