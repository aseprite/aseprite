// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_SCRIPT_INPUT_CHAIN_H_INCLUDED
#define APP_SCRIPT_SCRIPT_INPUT_CHAIN_H_INCLUDED
#pragma once

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/ui/input_chain_element.h"

namespace app {

class ScriptInputChain : public InputChainElement {
public:
  // InputChainElement impl
  ~ScriptInputChain() override;
  void onNewInputPriority(InputChainElement* element, const ui::Message* msg) override;
  bool onCanCut(Context* ctx) override;
  bool onCanCopy(Context* ctx) override;
  bool onCanPaste(Context* ctx) override;
  bool onCanClear(Context* ctx) override;
  bool onCut(Context* ctx) override;
  bool onCopy(Context* ctx) override;
  bool onPaste(Context* ctx, const gfx::Point* position) override;
  bool onClear(Context* ctx) override;
  void onCancel(Context* ctx) override;
};

} // namespace app

#endif
