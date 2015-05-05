// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CONTEXT_OBSERVER_H_INCLUDED
#define DOC_CONTEXT_OBSERVER_H_INCLUDED
#pragma once

namespace doc {

  class Document;
  class Site;

  class ContextObserver {
  public:
    virtual ~ContextObserver() { }
    virtual void onActiveSiteChange(const Site& site) { }
  };

} // namespace doc

#endif
