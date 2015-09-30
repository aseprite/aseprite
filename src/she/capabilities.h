// SHE library
// Copyright (C) 2012-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_CAPABILITIES_H_INCLUDED
#define SHE_CAPABILITIES_H_INCLUDED
#pragma once

namespace she {

  enum class Capabilities {
    MultipleDisplays = 1,
    CanResizeDisplay = 2,
    DisplayScale = 4,
  };

} // namespace she

#endif
