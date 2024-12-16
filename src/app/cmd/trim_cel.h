// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_TRIM_CEL_H_INCLUDED
#define APP_CMD_TRIM_CEL_H_INCLUDED
#pragma once

#include "app/cmd_sequence.h"

namespace doc {
class Cel;
}

namespace app { namespace cmd {

class TrimCel : public CmdSequence {
public:
  TrimCel(doc::Cel* cel);
};

}} // namespace app::cmd

#endif
