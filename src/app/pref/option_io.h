// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_PREF_OPTION_IO_H_INCLUDED
#define APP_PREF_OPTION_IO_H_INCLUDED
#pragma once

#include "app/ini_file.h"
#include "app/pref/option.h"

namespace app {

  // Load

  template<typename T>
  void load_option(Option<T>& opt) {
    if (get_config_string(opt.section(), opt.id(), nullptr))
      opt(get_config_value(opt.section(), opt.id(), opt.defaultValue()));
    else
      opt.setValueAndDefault(opt.defaultValue());
  }

  template<typename T>
  void load_option_with_migration(Option<T>& opt, const char* oldSection, const char* oldName) {
    if (get_config_string(oldSection, oldName, nullptr)) {
      opt(get_config_value(oldSection, oldName, opt.defaultValue()));
      del_config_value(oldSection, oldName);

      opt.forceDirtyFlag();
    }
    else {
      load_option<T>(opt);
    }
  }

  // Save

  template<typename T>
  void save_option(Option<T>& opt) {
    if (!opt.isDirty())
      return;

    set_config_value(opt.section(), opt.id(), opt());
    opt.cleanDirtyFlag();
  }

} // namespace app

#endif
