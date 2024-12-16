// Aseprite View Library
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef VIEW_UTILS_H_INCLUDED
#define VIEW_UTILS_H_INCLUDED
#pragma once

#include "view/range.h"

namespace view {

class TimelineAdapter;

VirtualRange to_virtual_range(const TimelineAdapter* adapter, const RealRange& realRange);

RealRange to_real_range(const TimelineAdapter* adapter, const VirtualRange& virtualRange);

} // namespace view

#endif // VIEW_RANGE_H_INCLUDED
