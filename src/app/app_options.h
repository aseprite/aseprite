// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
  enum VerboseLevel {
    kNoVerbose,
    kVerbose,
    kHighlyVerbose,
  };

  typedef base::ProgramOptions PO;
  typedef PO::Option Option;
  typedef PO::ValueList ValueList;

  AppOptions(int argc, const char* argv[]);

  bool startUI() const { return m_startUI; }
  bool startShell() const { return m_startShell; }
  VerboseLevel verboseLevel() const { return m_verboseLevel; }

  const std::string& paletteFileName() const { return m_paletteFileName; }

  const ValueList& values() const {
    return m_po.values();
  }

  // Export options
  const Option& saveAs() const { return m_saveAs; }
  const Option& scale() const { return m_scale; }
  const Option& data() const { return m_data; }
  const Option& format() const { return m_format; }
  const Option& sheet() const { return m_sheet; }
  const Option& sheetWidth() const { return m_sheetWidth; }
  const Option& sheetHeight() const { return m_sheetHeight; }
  const Option& sheetType() const { return m_sheetType; }
  const Option& sheetPack() const { return m_sheetPack; }
  const Option& splitLayers() const { return m_splitLayers; }
  const Option& layer() const { return m_layer; }
  const Option& allLayers() const { return m_allLayers; }
  const Option& frameTag() const { return m_frameTag; }
  const Option& ignoreEmpty() const { return m_ignoreEmpty; }
  const Option& borderPadding() const { return m_borderPadding; }
  const Option& shapePadding() const { return m_shapePadding; }
  const Option& innerPadding() const { return m_innerPadding; }
  const Option& trim() const { return m_trim; }
  const Option& crop() const { return m_crop; }
  const Option& filenameFormat() const { return m_filenameFormat; }
  const Option& script() const { return m_script; }
  const Option& listLayers() const { return m_listLayers; }
  const Option& listTags() const { return m_listTags; }

  bool hasExporterParams() const;

private:
  void showHelp();
  void showVersion();

  std::string m_exeName;
  base::ProgramOptions m_po;
  bool m_startUI;
  bool m_startShell;
  VerboseLevel m_verboseLevel;
  std::string m_paletteFileName;

  Option& m_palette;
  Option& m_shell;
  Option& m_batch;
  Option& m_saveAs;
  Option& m_scale;
  Option& m_data;
  Option& m_format;
  Option& m_sheet;
  Option& m_sheetWidth;
  Option& m_sheetHeight;
  Option& m_sheetType;
  Option& m_sheetPack;
  Option& m_splitLayers;
  Option& m_layer;
  Option& m_allLayers;
  Option& m_frameTag;
  Option& m_ignoreEmpty;
  Option& m_borderPadding;
  Option& m_shapePadding;
  Option& m_innerPadding;
  Option& m_trim;
  Option& m_crop;
  Option& m_filenameFormat;
  Option& m_script;
  Option& m_listLayers;
  Option& m_listTags;

  Option& m_verbose;
  Option& m_debug;
  Option& m_help;
  Option& m_version;

};

} // namespace app

#endif
