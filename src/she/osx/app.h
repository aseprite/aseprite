// SHE library
// Copyright (C) 2012-2016  David Capello
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
    OSXApp();
    ~OSXApp();

    bool init();

  private:
    class Impl;
    Impl* m_impl;
  };

} // namespace she

#endif
