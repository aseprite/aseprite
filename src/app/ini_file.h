/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#ifndef APP_INI_FILE_H_INCLUDED
#define APP_INI_FILE_H_INCLUDED

#include <allegro/config.h>
#include "gfx/rect.h"
#include "app/color.h"

namespace app {

  class ConfigModule {
  public:
    ConfigModule();
    ~ConfigModule();
  };

  bool get_config_bool(const char *section, const char *name, bool value);
  void set_config_bool(const char *section, const char *name, bool value);

  gfx::Rect get_config_rect(const char *section, const char *name, const gfx::Rect& rect);
  void set_config_rect(const char *section, const char *name, const gfx::Rect& rect);

  app::Color get_config_color(const char *section, const char *name, const app::Color& value);
  void set_config_color(const char *section, const char *name, const app::Color& value);

} // namespace app

#endif
