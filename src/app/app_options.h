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

#ifndef APP_APP_OPTIONS_H_INCLUDED
#define APP_APP_OPTIONS_H_INCLUDED
#pragma once

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

  const base::ProgramOptions::ValueList& values() const {
    return m_po.values();
  }

  // Export options
  const std::string& dataFormat() const { return m_dataFormat; }
  const std::string& data() const { return m_data; }
  const std::string& textureFormat() const { return m_textureFormat; }
  const std::string& sheet() const { return m_sheet; }
  const double scale() const { return m_scale; }
  const std::string& scaleMode() const { return m_scaleMode; }

  bool hasExporterParams() {
    return
      !m_dataFormat.empty() ||
      !m_data.empty() ||
      !m_textureFormat.empty() ||
      !m_sheet.empty();
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

  std::string m_dataFormat;
  std::string m_data;
  std::string m_textureFormat;
  std::string m_sheet;
  double m_scale;
  std::string m_scaleMode;
};

} // namespace app

#endif
