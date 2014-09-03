// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_INTERNALS_H_INCLUDED
#define SHE_INTERNALS_H_INCLUDED
#pragma once

namespace she {

  class Display;
  class Event;

  extern Display* unique_display;

  void queue_event(const Event& ev);

} // namespace she

#endif
