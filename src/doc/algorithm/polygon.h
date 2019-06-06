// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ALGORITHM_POLYGON_H_INCLUDED
#define DOC_ALGORITHM_POLYGON_H_INCLUDED
#pragma once

#include "doc/algorithm/hline.h"
#include "gfx/fwd.h"

#include <vector>

namespace doc {
  namespace algorithm {

    void polygon(int vertices, const int* points, void* data, AlgoHLine proc);
    bool createUnion(std::vector<int>& pairs, const int x, int& ints);
  }
}

#endif
