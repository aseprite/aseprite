// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_ALLEG4_DISPLAY_EVENTS_H_INCLUDED
#define SHE_ALLEG4_DISPLAY_EVENTS_H_INCLUDED
#pragma once

namespace she {

  void display_events_init();
  void display_generate_events();

  // Returns true if a resize event was received but the "screen"
  // variable wasn't recreated yet (i.e. acknowledge_resize() wasn't
  // called yet)
  bool is_display_resize_awaiting();

} // namespace she

#endif
