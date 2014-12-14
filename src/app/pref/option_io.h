/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_PREF_OPTION_IO_H_INCLUDED
#define APP_PREF_OPTION_IO_H_INCLUDED
#pragma once

#include "app/ini_file.h"
#include "app/pref/option.h"

namespace app {

  // Load

  template<typename T>
  void load_option(Option<T>& opt) {
    opt(get_config_value(opt.section(), opt.id(), opt.defaultValue()));
  }

  template<typename T>
  void load_option_with_migration(Option<T>& opt, const char* oldSection, const char* oldName) {
    if (get_config_string(oldSection, oldName, NULL)) {
      opt(get_config_value(oldSection, oldName, opt.defaultValue()));
      del_config_value(oldSection, oldName);

      opt.forceDirtyFlag();
    }
    else
      opt(get_config_value(opt.section(), opt.id(), opt.defaultValue()));
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
