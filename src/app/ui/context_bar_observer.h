// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CONTEXT_BAR_OBSERVER_H_INCLUDED
#define APP_CONTEXT_BAR_OBSERVER_H_INCLUDED
#pragma once

namespace app {

class ContextBarObserver {
public:
  enum DropAction { DropPixels, CancelDrag };

  virtual ~ContextBarObserver() {}
  virtual void onDropPixels(DropAction action) {}
};

} // namespace app

#endif
