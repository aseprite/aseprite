// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_INPUT_CHAIN_ELEMENT_H_INCLUDED
#define APP_INPUT_CHAIN_ELEMENT_H_INCLUDED
#pragma once

#include "gfx/point.h"

namespace ui {
class Message;
}

namespace app {

class Context;

class InputChainElement {
public:
  virtual ~InputChainElement() {}

  // Called when a new element has priorty in the chain.
  virtual void onNewInputPriority(InputChainElement* element, const ui::Message* msg) = 0;

  virtual bool onCanCut(Context* ctx) = 0;
  virtual bool onCanCopy(Context* ctx) = 0;
  virtual bool onCanPaste(Context* ctx) = 0;
  virtual bool onCanClear(Context* ctx) = 0;

  // These commands are executed from Context::executeCommand()
  // which catch any exception that is thrown.
  virtual bool onCut(Context* ctx) = 0;
  virtual bool onCopy(Context* ctx) = 0;
  virtual bool onPaste(Context* ctx, const gfx::Point* position) = 0;
  virtual bool onClear(Context* ctx) = 0;
  virtual void onCancel(Context* ctx) = 0;
};

} // namespace app

#endif
