// Aseprite
// Copyright (c) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CONTEXT_OBSERVER_H_INCLUDED
#define APP_CONTEXT_OBSERVER_H_INCLUDED
#pragma once

namespace app {

  class Doc;
  class Site;

  class ContextObserver {
  public:
    virtual ~ContextObserver() { }
    virtual void onActiveSiteChange(const Site& site) { }
  };

} // namespace app

#endif
