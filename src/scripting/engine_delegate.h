// Aseprite Scripting Library
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SCRIPTING_ENGINE_DELEGATE_H_INCLUDED
#define SCRIPTING_ENGINE_DELEGATE_H_INCLUDED
#pragma once

namespace scripting {

  class EngineDelegate {
  public:
    virtual ~EngineDelegate() { }
    virtual void onConsolePrint(const char* text) = 0;
  };

}

#endif
