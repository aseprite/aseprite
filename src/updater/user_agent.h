// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef UPDATER_USER_AGENT_H_INCLUDED
#define UPDATER_USER_AGENT_H_INCLUDED
#pragma once

#include "base/disable_copying.h"

#include <iosfwd>

namespace updater {

  std::string getUserAgent();

} // namespace updater

#endif  // UPDATER_USER_AGENT_H_INCLUDED
