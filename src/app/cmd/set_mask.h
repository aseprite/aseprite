// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_SET_MASK_H_INCLUDED
#define APP_CMD_SET_MASK_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_document.h"
#include "base/unique_ptr.h"

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
    SetMask(Document* doc, Mask* newMask);

    // Used to change the new mask used in the onRedo()
    void setNewMask(Mask* newMask);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override;

  private:
    void setMask(Mask* mask);

    base::UniquePtr<Mask> m_oldMask;
    base::UniquePtr<Mask> m_newMask;
  };

} // namespace cmd
} // namespace app

#endif
