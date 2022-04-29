// Aseprite
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CLI_APP_OPTIONS_H_INCLUDED
#define APP_CLI_APP_OPTIONS_H_INCLUDED
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

  const std::string& exeName() const { return m_exeName; }
  const base::ProgramOptions& programOptions() const { return m_po; }

  bool startUI() const { return m_startUI; }
  bool startShell() const { return m_startShell; }
  bool previewCLI() const { return m_previewCLI; }
  bool showHelp() const { return m_showHelp; }
  bool showVersion() const { return m_showVersion; }
  VerboseLevel verboseLevel() const { return m_verboseLevel; }

  const ValueList& values() const {
    return m_po.values();
  }

  // Export options
  const Option& saveAs() const { return m_saveAs; }
  const Option& palette() const { return m_palette; }
  const Option& scale() const { return m_scale; }
  const Option& ditheringAlgorithm() const { return m_ditheringAlgorithm; }
  const Option& ditheringMatrix() const { return m_ditheringMatrix; }
  const Option& colorMode() const { return m_colorMode; }
  const Option& shrinkTo() const { return m_shrinkTo; }
  const Option& data() const { return m_data; }
  const Option& format() const { return m_format; }
  const Option& sheet() const { return m_sheet; }
  const Option& sheetType() const { return m_sheetType; }
  const Option& sheetPack() const { return m_sheetPack; }
  const Option& sheetWidth() const { return m_sheetWidth; }
  const Option& sheetHeight() const { return m_sheetHeight; }
  const Option& sheetColumns() const { return m_sheetColumns; }
  const Option& sheetRows() const { return m_sheetRows; }
  const Option& splitLayers() const { return m_splitLayers; }
  const Option& splitTags() const { return m_splitTags; }
  const Option& splitSlices() const { return m_splitSlices; }
  const Option& splitGrid() const { return m_splitGrid; }
  const Option& layer() const { return m_layer; }
  const Option& allLayers() const { return m_allLayers; }
  const Option& ignoreLayer() const { return m_ignoreLayer; }
  const Option& tag() const { return m_tag; }
  const Option& frameRange() const { return m_frameRange; }
  const Option& ignoreEmpty() const { return m_ignoreEmpty; }
  const Option& mergeDuplicates() const { return m_mergeDuplicates; }
  const Option& borderPadding() const { return m_borderPadding; }
  const Option& shapePadding() const { return m_shapePadding; }
  const Option& innerPadding() const { return m_innerPadding; }
  const Option& trim() const { return m_trim; }
  const Option& trimSprite() const { return m_trimSprite; }
  const Option& trimByGrid() const { return m_trimByGrid; }
  const Option& extrude() const { return m_extrude; }
  const Option& crop() const { return m_crop; }
  const Option& slice() const { return m_slice; }
  const Option& filenameFormat() const { return m_filenameFormat; }
  const Option& tagnameFormat() const { return m_tagnameFormat; }
#ifdef ENABLE_SCRIPTING
  const Option& script() const { return m_script; }
  const Option& scriptParam() const { return m_scriptParam; }
#endif
  const Option& listLayers() const { return m_listLayers; }
  const Option& listTags() const { return m_listTags; }
  const Option& listSlices() const { return m_listSlices; }
  const Option& oneFrame() const { return m_oneFrame; }
  const Option& exportTileset() const { return m_exportTileset; }

  bool hasExporterParams() const;
#ifdef _WIN32
  bool disableWintab() const;
#endif

private:
  AppOptions(const AppOptions& that);

  std::string m_exeName;
  base::ProgramOptions m_po;
  bool m_startUI;
  bool m_startShell;
  bool m_previewCLI;
  bool m_showHelp;
  bool m_showVersion;
  VerboseLevel m_verboseLevel;

#ifdef ENABLE_SCRIPTING
  Option& m_shell;
#endif
  Option& m_batch;
  Option& m_preview;
  Option& m_saveAs;
  Option& m_palette;
  Option& m_scale;
  Option& m_ditheringAlgorithm;
  Option& m_ditheringMatrix;
  Option& m_colorMode;
  Option& m_shrinkTo;
  Option& m_data;
  Option& m_format;
  Option& m_sheet;
  Option& m_sheetType;
  Option& m_sheetPack;
  Option& m_sheetWidth;
  Option& m_sheetHeight;
  Option& m_sheetColumns;
  Option& m_sheetRows;
  Option& m_splitLayers;
  Option& m_splitTags;
  Option& m_splitSlices;
  Option& m_splitGrid;
  Option& m_layer;
  Option& m_allLayers;
  Option& m_ignoreLayer;
  Option& m_tag;
  Option& m_frameRange;
  Option& m_ignoreEmpty;
  Option& m_mergeDuplicates;
  Option& m_borderPadding;
  Option& m_shapePadding;
  Option& m_innerPadding;
  Option& m_trim;
  Option& m_trimSprite;
  Option& m_trimByGrid;
  Option& m_extrude;
  Option& m_crop;
  Option& m_slice;
  Option& m_filenameFormat;
  Option& m_tagnameFormat;
#ifdef ENABLE_SCRIPTING
  Option& m_script;
  Option& m_scriptParam;
#endif
  Option& m_listLayers;
  Option& m_listTags;
  Option& m_listSlices;
  Option& m_oneFrame;
  Option& m_exportTileset;

  Option& m_verbose;
  Option& m_debug;
#ifdef _WIN32
  Option& m_disableWintab;
#endif
  Option& m_help;
  Option& m_version;

};

} // namespace app

#endif
