// Aseprite Scripting Library
// Copyright (c) 2015-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SCRIPT_ENGINE_DELEGATE_H_INCLUDED
#define SCRIPT_ENGINE_DELEGATE_H_INCLUDED
#pragma once

namespace script {

  class EngineDelegate {
  public:
    virtual ~EngineDelegate() { }
    virtual void onConsolePrint(const char* text) = 0;
  };

  class StdoutEngineDelegate : public EngineDelegate {
  public:
    void onConsolePrint(const char* text) override;
  };

}

#endif
