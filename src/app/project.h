// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_PROJECT_H_INCLUDED
#define APP_PROJECT_H_INCLUDED
#pragma once

#include "app/project_observer.h"
#include "base/disable_copying.h"
#include "base/observable.h"

namespace app {

  class Project : public base::Observable<ProjectObserver> {
  public:
    Project();
    ~Project();

  private:
    DISABLE_COPYING(Project);
  };

} // namespace app

#endif
