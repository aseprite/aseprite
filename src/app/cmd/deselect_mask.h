// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_DESELECT_MASK_H_INCLUDED
#define APP_CMD_DESELECT_MASK_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_document.h"
#include "base/unique_ptr.h"

namespace doc {
  class Mask;
}

namespace app {
namespace cmd {
  using namespace doc;

  class DeselectMask : public Cmd
                     , public WithDocument {
  public:
    DeselectMask(Document* doc);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override;

  private:
    base::UniquePtr<Mask> m_oldMask;
  };

} // namespace cmd
} // namespace app

#endif
