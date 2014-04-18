// SHE library
// Copyright (C) 2012-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_CAPABILITIES_H_INCLUDED
#define SHE_CAPABILITIES_H_INCLUDED
#pragma once

namespace she {

  enum Capabilities {
    kMultipleDisplaysCapability = 1,
    kCanResizeDisplayCapability = 2,
    kDisplayScaleCapability = 4,
    kMouseEventsCapability = 8,
  };

} // namespace she

#endif
