// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef UPDATER_USER_AGENT_H_INCLUDED
#define UPDATER_USER_AGENT_H_INCLUDED
#pragma once

#include <string>

namespace updater {

std::string getFullOSString();
std::string getUserAgent();

} // namespace updater

#endif // UPDATER_USER_AGENT_H_INCLUDED
