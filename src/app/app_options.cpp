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

#include <cstdlib>
#include <iostream>

namespace app {

typedef base::ProgramOptions::Option Option;

AppOptions::AppOptions(int argc, const char* argv[])
  : m_exeName(base::get_file_name(argv[0]))
  , m_startUI(true)
  , m_startShell(false)
  , m_verbose(false)
  , m_scale(1.0)
{
  Option& palette = m_po.add("palette").requiresValue("<filename>").description("Use a specific palette by default");
  Option& shell = m_po.add("shell").description("Start an interactive console to execute scripts");
  Option& batch = m_po.add("batch").description("Do not start the UI");
  // Option& dataFormat = m_po.add("format").requiresValue("<name>").description("Select the format for the sprite sheet data");
  Option& data = m_po.add("data").requiresValue("<filename>").description("File to store the sprite sheet metadata (.json file)");
  //Option& textureFormat = m_po.add("texture-format").requiresValue("<name>").description("Output texture format.");
  Option& sheet = m_po.add("sheet").requiresValue("<filename>").description("Image file to save the texture (.png)");
  //Option& scale = m_po.add("scale").requiresValue("<float>").description("");
  //Option& scaleMode = m_po.add("scale-mode").requiresValue("<mode>").description("Export the first given document to a JSON object");
  //Option& splitLayers = m_po.add("split-layers").description("Specifies that each layer of the given file should be saved as a different image in the sheet.");
  //Option& rotsprite = m_po.add("rotsprite").requiresValue("<angle1,angle2,...>").description("Specifies different angles to export the given image.");
  //Option& merge = m_po.add("merge").requiresValue("<datafiles>").description("Merge several sprite sheets in one.");
  Option& verbose = m_po.add("verbose").description("Explain what is being done (in stderr or a log file)");
  Option& help = m_po.add("help").mnemonic('?').description("Display this help and exits");
  Option& version = m_po.add("version").description("Output version information and exit");

  try {
    m_po.parse(argc, argv);

    m_verbose = m_po.enabled(verbose);
    m_paletteFileName = m_po.value_of(palette);
    m_startShell = m_po.enabled(shell);
    // m_dataFormat = m_po.value_of(dataFormat);
    m_data = m_po.value_of(data);
    // m_textureFormat = m_po.value_of(textureFormat);
    m_sheet = m_po.value_of(sheet);
    // if (scale.enabled())
    //   m_scale = std::strtod(m_po.value_of(scale).c_str(), NULL);
    // m_scaleMode = m_po.value_of(scaleMode);

    if (m_po.enabled(help)) {
      showHelp();
      m_startUI = false;
    }
    else if (m_po.enabled(version)) {
      showVersion();
      m_startUI = false;
    }

    if (m_po.enabled(shell) || m_po.enabled(batch)) {
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
