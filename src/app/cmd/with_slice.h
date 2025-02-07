// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_WITH_SLICE_H_INCLUDED
#define APP_CMD_WITH_SLICE_H_INCLUDED
#pragma once

#include "doc/object_id.h"

namespace doc {
class Slice;
}

namespace app { namespace cmd {
using namespace doc;

class WithSlice {
public:
  WithSlice(Slice* slice);
  Slice* slice();

private:
  ObjectId m_sliceId;
};

}} // namespace app::cmd

#endif
