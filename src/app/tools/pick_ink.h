// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_PICK_INK_H_INCLUDED
#define APP_TOOLS_PICK_INK_H_INCLUDED
#pragma once

#include "app/tools/ink.h"

namespace app { namespace tools {

class PickInk : public Ink {
public:
  enum Target { Fg, Bg };

public:
  PickInk(Target target);
  Ink* clone() override;

  Target target() const { return m_target; }

  bool isEyedropper() const override;
  void prepareInk(ToolLoop* loop) override;
  void inkHline(int x1, int y, int x2, ToolLoop* loop) override;

private:
  Target m_target;
};

}} // namespace app::tools

#endif // APP_TOOLS_PICK_INK_H_INCLUDED
