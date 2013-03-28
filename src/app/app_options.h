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

#ifndef APP_APP_OPTIONS_H_INCLUDED
#define APP_APP_OPTIONS_H_INCLUDED

#include <stdexcept>
#include <string>
#include <vector>

#include "base/program_options.h"

namespace app {

class AppOptions {
public:
  AppOptions(int argc, const char* argv[]);

  bool startUI() const { return m_startUI; }
  bool startShell() const { return m_startShell; }
  bool verbose() const { return m_verbose; }

  const std::string& paletteFileName() const { return m_paletteFileName; }

  const base::ProgramOptions::ValueList& files() const {
    return m_po.values();
  }

private:
  void showHelp();
  void showVersion();

  std::string m_exeName;
  base::ProgramOptions m_po;
  bool m_startUI;
  bool m_startShell;
  bool m_verbose;
  std::string m_paletteFileName;
};

}

#endif
