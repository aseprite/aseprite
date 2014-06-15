/* Aseprite
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ini_file.h"

#include "app/resource_finder.h"
#include "base/fs.h"
#include "base/path.h"

#include <allegro/config.h>
#include <allegro/file.h>
#include <allegro/unicode.h>

namespace app {

using namespace gfx;

static std::string g_configFilename;

ConfigModule::ConfigModule()
{
  ResourceFinder rf;
  rf.includeConfFile();

  std::string config_filename;

  // Search the configuration file from first to last path
  if (rf.findFirst())
    config_filename = rf.filename();

  // If the file wasn't found, we will create configuration file
  // in the first path
  if (config_filename[0] == 0) {
    config_filename = rf.defaultFilename();

    std::string dir = base::get_file_path(config_filename);
    if (!base::is_directory(dir))
      base::make_directory(dir);
  }

  override_config_file(config_filename.c_str());
  g_configFilename = config_filename;
}

ConfigModule::~ConfigModule()
{
  //override_config_file(NULL);
  flush_config_file();
}

std::string get_config_file()
{
  return g_configFilename;
}

bool get_config_bool(const char *section, const char *name, bool value)
{
  const char *got = get_config_string(section, name, value ? "yes": "no");
  return (got &&
          (ustricmp(got, "yes") == 0 ||
           ustricmp(got, "true") == 0 ||
           ustricmp(got, "1") == 0)) ? true: false;
}

void set_config_bool(const char *section, const char *name, bool value)
{
  set_config_string(section, name, value ? "yes": "no");
}

Rect get_config_rect(const char *section, const char *name, const Rect& rect)
{
  Rect rect2(rect);
  char **argv;
  int argc;

  argv = get_config_argv(section, name, &argc);

  if (argv && argc == 4) {
    rect2.x = ustrtol(argv[0], NULL, 10);
    rect2.y = ustrtol(argv[1], NULL, 10);
    rect2.w = ustrtol(argv[2], NULL, 10);
    rect2.h = ustrtol(argv[3], NULL, 10);
  }

  return rect2;
}

void set_config_rect(const char *section, const char *name, const Rect& rect)
{
  char buf[128];
  uszprintf(buf, sizeof(buf), "%d %d %d %d",
            rect.x, rect.y, rect.w, rect.h);
  set_config_string(section, name, buf);
}

app::Color get_config_color(const char *section, const char *name, const app::Color& value)
{
  return app::Color::fromString(get_config_string(section, name, value.toString().c_str()));
}

void set_config_color(const char *section, const char *name, const app::Color& value)
{
  set_config_string(section, name, value.toString().c_str());
}

} // namespace app
