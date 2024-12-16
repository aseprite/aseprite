// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_CLEAR_MASK_H_INCLUDED
#define APP_CMD_CLEAR_MASK_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "app/cmd/with_image.h"
#include "app/cmd_sequence.h"
#include "doc/image_ref.h"
#include "gfx/rect.h"

#include <memory>

namespace app { namespace cmd {
using namespace doc;

class ClearMask : public Cmd,
                  public WithCel {
public:
  ClearMask(Cel* cel);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  size_t onMemSize() const override
  {
    return sizeof(*this) + m_seq.memSize() + (m_copy ? m_copy->getMemSize() : 0);
  }

private:
  void clear();
  void restore();

  CmdSequence m_seq;
  ImageRef m_copy;
  gfx::Point m_cropPos;
  color_t m_bgcolor;
};

}} // namespace app::cmd

#endif
