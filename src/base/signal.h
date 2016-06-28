// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_SIGNAL_H_INCLUDED
#define BASE_SIGNAL_H_INCLUDED
#pragma once

#include "obs/signal.h"

namespace base {

template<typename R>
using Signal0 = obs::signal0<R>;

template<typename R, typename A1>
using Signal1 = obs::signal1<R, A1>;

template<typename R, typename A1, typename A2>
using Signal2 = obs::signal2<R, A1, A2>;

} // namespace base

#endif
