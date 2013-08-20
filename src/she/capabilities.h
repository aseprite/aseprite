// SHE library
// Copyright (C) 2012-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef SHE_CAPABILITIES_H_INCLUDED
#define SHE_CAPABILITIES_H_INCLUDED

namespace she {

  enum Capabilities {
    kMultipleDisplaysCapability = 1,
    kCanResizeDisplayCapability = 2,
    kDisplayScaleCapability = 4
  };

} // namespace she

#endif
