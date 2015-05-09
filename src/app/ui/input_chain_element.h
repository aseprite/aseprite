// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_INPUT_CHAIN_ELEMENT_H_INCLUDED
#define APP_INPUT_CHAIN_ELEMENT_H_INCLUDED
#pragma once

namespace app {

  class Context;

  class InputChainElement {
  public:
    virtual ~InputChainElement() { }

    // Called when a new element has priorty in the chain.
    virtual void onNewInputPriority(InputChainElement* element) = 0;

    virtual bool onCanCut(Context* ctx) = 0;
    virtual bool onCanCopy(Context* ctx) = 0;
    virtual bool onCanPaste(Context* ctx) = 0;
    virtual bool onCanClear(Context* ctx) = 0;

    virtual bool onCut(Context* ctx) = 0;
    virtual bool onCopy(Context* ctx) = 0;
    virtual bool onPaste(Context* ctx) = 0;
    virtual bool onClear(Context* ctx) = 0;
    virtual void onCancel(Context* ctx) = 0;
  };

} // namespace app

#endif
