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
  typedef base::ProgramOptions PO;
  typedef PO::Option Option;
  typedef PO::ValueList ValueList;

  AppOptions(int argc, const char* argv[]);

  bool startUI() const { return m_startUI; }
  bool startShell() const { return m_startShell; }
  bool verbose() const { return m_verboseEnabled; }

  const std::string& paletteFileName() const { return m_paletteFileName; }

  const ValueList& values() const {
    return m_po.values();
  }

  // Export options
  const Option& saveAs() const { return m_saveAs; }
  const Option& scale() const { return m_scale; }
  const Option& data() const { return m_data; }
  const Option& sheet() const { return m_sheet; }
  const Option& sheetWidth() const { return m_sheetWidth; }
  const Option& sheetHeight() const { return m_sheetHeight; }
  const Option& sheetPack() const { return m_sheetPack; }
  const Option& splitLayers() const { return m_splitLayers; }
  const Option& importLayer() const { return m_importLayer; }
  const Option& ignoreEmpty() const { return m_ignoreEmpty; }

  bool hasExporterParams() const;

private:
  void showHelp();
  void showVersion();

  std::string m_exeName;
  base::ProgramOptions m_po;
  bool m_startUI;
  bool m_startShell;
  bool m_verboseEnabled;
  std::string m_paletteFileName;

  Option& m_palette;
  Option& m_shell;
  Option& m_batch;
  Option& m_saveAs;
  Option& m_scale;
  Option& m_data;
  Option& m_sheet;
  Option& m_sheetWidth;
  Option& m_sheetHeight;
  Option& m_sheetPack;
  Option& m_splitLayers;
  Option& m_importLayer;
  Option& m_ignoreEmpty;

  Option& m_verbose;
  Option& m_help;
  Option& m_version;

};

} // namespace app

#endif
