// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_OPTIONAL_ALERT_H_INCLUDED
#define APP_UI_OPTIONAL_ALERT_H_INCLUDED
#pragma once

#include "app/pref/option.h"

#include <string>

namespace app {

class OptionalAlert {
public:
  static int show(Option<bool>& option, const int optionWhenDisabled, const std::string& msg);
};

} // namespace app

#endif
