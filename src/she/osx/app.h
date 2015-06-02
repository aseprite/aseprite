// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_OSX_APP_H_INCLUDED
#define SHE_OSX_APP_H_INCLUDED
#pragma once

namespace base {
  class thread;
}

namespace she {

  class OSXApp {
  public:
    static OSXApp* instance() { return g_instance; }

    OSXApp();
    ~OSXApp();

    int run(int argc, char* argv[]);
    void joinUserThread();

    void stopUIEventLoop();

  private:
    static OSXApp* g_instance;
    base::thread* m_userThread;
  };

} // namespace she

#endif
