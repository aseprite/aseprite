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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/launcher.h"

#include "base/exception.h"
#include "base/launcher.h"
#include "ui/alert.h"

namespace app {
namespace launcher {

void open_url(const std::string& url)
{
  open_file(url);
}

void open_file(const std::string& file)
{
  if (!base::launcher::open_file(file))
    ui::Alert::show("Problem<<Cannot open file:<<%s||&Close", file.c_str());
}

void open_folder(const std::string& file)
{
  if (base::launcher::support_open_folder()) {
    if (!base::launcher::open_folder(file))
      ui::Alert::show("Problem<<Cannot open folder:<<%s||&Close", file.c_str());
  }
  else
    ui::Alert::show("Problem<<This command is not supported on your platform||&Close");
}

} // namespace launcher
} // namespace app
