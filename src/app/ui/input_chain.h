// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_INPUT_CHAIN_H_INCLUDED
#define APP_INPUT_CHAIN_H_INCLUDED
#pragma once

#include <vector>

namespace ui {
  class Message;
}

namespace app {

  class Context;
  class InputChainElement;

  // The chain of objects (in order) that want to receive
  // input/commands from the user (e.g. ColorBar, Timeline, and
  // Workspace/DocView). When each of these elements receive the
  // user focus, they call InputChain::prioritize().
  class InputChain {
  public:
    void prioritize(InputChainElement* element,
                    const ui::Message* msg);

    bool canCut(Context* ctx);
    bool canCopy(Context* ctx);
    bool canPaste(Context* ctx);
    bool canClear(Context* ctx);

    void cut(Context* ctx);
    void copy(Context* ctx);
    void paste(Context* ctx);
    void clear(Context* ctx);
    void cancel(Context* ctx);

  private:
    std::vector<InputChainElement*> m_elements;
  };

} // namespace app

#endif
