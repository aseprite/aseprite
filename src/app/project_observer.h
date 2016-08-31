// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_PROJECT_OBSERVER_H_INCLUDED
#define APP_PROJECT_OBSERVER_H_INCLUDED
#pragma once

#include "app/project_event.h"

namespace app {

  class ProjectObserver {
  public:
    virtual ~ProjectObserver() { }
    virtual void dispose() = 0;
    virtual void onAddDocument(ProjectEvent& ev) = 0;
    virtual void onRemoveDocument(ProjectEvent& ev) = 0;
  };

} // namespace app

#endif
