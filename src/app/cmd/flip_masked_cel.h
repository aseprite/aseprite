// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_FLIP_MASKED_CEL_H_INCLUDED
#define APP_CMD_FLIP_MASKED_CEL_H_INCLUDED
#pragma once

#include "app/cmd_sequence.h"
#include "doc/algorithm/flip_type.h"
#include "doc/color.h"

namespace doc {
class Cel;
}

namespace app { namespace cmd {
using namespace doc;

class FlipMaskedCel : public CmdSequence {
public:
  FlipMaskedCel(Cel* cel, doc::algorithm::FlipType flipType);
};

}} // namespace app::cmd

#endif
