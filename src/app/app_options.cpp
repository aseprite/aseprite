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

#include "app/app_options.h"

#include "base/path.h"

#include <iostream>

namespace app {

typedef base::ProgramOptions::Option Option;

AppOptions::AppOptions(int argc, const char* argv[])
  : m_exeName(base::get_file_name(argv[0]))
  , m_startUI(true)
  , m_startShell(false)
  , m_verbose(false)
{
  Option& palette = m_po.add("palette").requiresValue("GFXFILE").description("Use a specific palette by default");
  Option& shell = m_po.add("shell").description("Start an interactive console to execute scripts");
  Option& batch = m_po.add("batch").description("Do not start the UI");
  Option& verbose = m_po.add("verbose").description("Explain what is being done (in stderr or a log file)");
  Option& help = m_po.add("help").mnemonic('?').description("Display this help and exits");
  Option& version = m_po.add("version").description("Output version information and exit");

  try {
    m_po.parse(argc, argv);

    m_verbose = verbose.enabled();
    m_paletteFileName = palette.value();
    m_startShell = shell.enabled();

    if (help.enabled()) {
      showHelp();
      m_startUI = false;
    }
    else if (version.enabled()) {
      showVersion();
      m_startUI = false;
    }

    if (shell.enabled() || batch.enabled()) {
      m_startUI = false;
    }
  }
  catch (const std::runtime_error& parseError) {
    std::cerr << m_exeName << ": " << parseError.what() << '\n'
              << "Try \"" << m_exeName << " --help\" for more information.\n";
    m_startUI = false;
  }
}

void AppOptions::showHelp()
{
  std::cout
    << PACKAGE << " v" << VERSION << " | A pixel art program\n" << COPYRIGHT
    << "\n\nUsage:\n"
    << "  " << m_exeName << " [OPTIONS] [FILES]...\n\n"
    << "Options:\n"
    << m_po
    << "\nFind more information in " << PACKAGE
    << " web site: " << WEBSITE << "\n\n";
}

void AppOptions::showVersion()
{
  std::cout << PACKAGE << ' ' << VERSION << '\n';
}

}
