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

#include "app/shell.h"

#include "scripting/engine.h"

#include <iostream>
#include <string>

namespace app {

Shell::Shell()
{
}

Shell::~Shell()
{
}

void Shell::run(scripting::Engine& engine)
{
  std::cout << "Welcome to " PACKAGE " v" VERSION " interactive console" << std::endl;
  std::string line;
  while (std::getline(std::cin, line)) {
    engine.eval(line);
  }
  std::cout << "Done\n";
}

} // namespace app
