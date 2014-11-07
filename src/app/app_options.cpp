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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app_options.h"

#include "base/path.h"

#include <cstdlib>
#include <iostream>

namespace app {

typedef base::ProgramOptions::Option Option;

AppOptions::AppOptions(int argc, const char* argv[])
  : m_exeName(base::get_file_name(argv[0]))
  , m_startUI(true)
  , m_startShell(false)
  , m_verboseEnabled(false)
  , m_palette(m_po.add("palette").requiresValue("<filename>").description("Use a specific palette by default"))
  , m_shell(m_po.add("shell").description("Start an interactive console to execute scripts"))
  , m_batch(m_po.add("batch").description("Do not start the UI"))
  , m_saveAs(m_po.add("save-as").requiresValue("<filename>").description("Save the last given document with other format"))
  , m_scale(m_po.add("scale").requiresValue("<factor>").description("Resize all previous opened documents"))
  , m_data(m_po.add("data").requiresValue("<filename.json>").description("File to store the sprite sheet metadata"))
  , m_sheet(m_po.add("sheet").requiresValue("<filename.png>").description("Image file to save the texture"))
  , m_sheetWidth(m_po.add("sheet-width").requiresValue("<pixels>").description("Sprite sheet width"))
  , m_sheetHeight(m_po.add("sheet-height").requiresValue("<pixels>").description("Sprite sheet height"))
  , m_sheetPack(m_po.add("sheet-pack").description("Use a packing algorithm to avoid waste of space\nin the texture"))
  , m_splitLayers(m_po.add("split-layers").description("Import each layer of the next given sprite as\na separated image in the sheet"))
  , m_importLayer(m_po.add("import-layer").requiresValue("<name>").description("Import just one layer of the next given sprite"))
  , m_verbose(m_po.add("verbose").description("Explain what is being done"))
  , m_help(m_po.add("help").mnemonic('?').description("Display this help and exits"))
  , m_version(m_po.add("version").description("Output version information and exit"))
{
  try {
    m_po.parse(argc, argv);

    m_verboseEnabled = m_po.enabled(m_verbose);
    m_paletteFileName = m_po.value_of(m_palette);
    m_startShell = m_po.enabled(m_shell);

    if (m_po.enabled(m_help)) {
      showHelp();
      m_startUI = false;
    }
    else if (m_po.enabled(m_version)) {
      showVersion();
      m_startUI = false;
    }

    if (m_po.enabled(m_shell) || m_po.enabled(m_batch)) {
      m_startUI = false;
    }
  }
  catch (const std::runtime_error& parseError) {
    std::cerr << m_exeName << ": " << parseError.what() << '\n'
              << "Try \"" << m_exeName << " --help\" for more information.\n";
    m_startUI = false;
  }
}

bool AppOptions::hasExporterParams() const
{
  return
    m_po.enabled(m_data) ||
    m_po.enabled(m_sheet);
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
