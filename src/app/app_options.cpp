// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
  , m_verboseLevel(kNoVerbose)
  , m_palette(m_po.add("palette").requiresValue("<filename>").description("Use a specific palette by default"))
  , m_shell(m_po.add("shell").description("Start an interactive console to execute scripts"))
  , m_batch(m_po.add("batch").mnemonic('b').description("Do not start the UI"))
  , m_saveAs(m_po.add("save-as").requiresValue("<filename>").description("Save the last given document with other format"))
  , m_scale(m_po.add("scale").requiresValue("<factor>").description("Resize all previous opened documents"))
  , m_data(m_po.add("data").requiresValue("<filename.json>").description("File to store the sprite sheet metadata"))
  , m_format(m_po.add("format").requiresValue("<format>").description("Format to export the data file (json-hash, json-array)"))
  , m_sheet(m_po.add("sheet").requiresValue("<filename.png>").description("Image file to save the texture"))
  , m_sheetWidth(m_po.add("sheet-width").requiresValue("<pixels>").description("Sprite sheet width"))
  , m_sheetHeight(m_po.add("sheet-height").requiresValue("<pixels>").description("Sprite sheet height"))
  , m_sheetType(m_po.add("sheet-type").requiresValue("<type>").description("Algorithm to create the sprite sheet:\n  horizontal\n  vertical\n  rows\n  columns\n  packed"))
  , m_sheetPack(m_po.add("sheet-pack").description("Same as --sheet-type packed"))
  , m_splitLayers(m_po.add("split-layers").description("Import each layer of the next given sprite as\na separated image in the sheet"))
  , m_layer(m_po.add("layer").alias("import-layer").requiresValue("<name>").description("Include just the given layer in the sheet"))
  , m_allLayers(m_po.add("all-layers").description("Make all layers visible\nBy default hidden layers will be ignored"))
  , m_frameTag(m_po.add("frame-tag").requiresValue("<name>").description("Include tagged frames in the sheet"))
  , m_ignoreEmpty(m_po.add("ignore-empty").description("Do not export empty frames/cels"))
  , m_borderPadding(m_po.add("border-padding").requiresValue("<value>").description("Add padding on the texture borders"))
  , m_shapePadding(m_po.add("shape-padding").requiresValue("<value>").description("Add padding between frames"))
  , m_innerPadding(m_po.add("inner-padding").requiresValue("<value>").description("Add padding inside each frame"))
  , m_trim(m_po.add("trim").description("Trim all images before exporting"))
  , m_crop(m_po.add("crop").requiresValue("x,y,width,height").description("Crop all the images to the given rectangle"))
  , m_filenameFormat(m_po.add("filename-format").requiresValue("<fmt>").description("Special format to generate filenames"))
  , m_script(m_po.add("script").requiresValue("<filename>").description("Execute a specific script"))
  , m_listLayers(m_po.add("list-layers").description("List layers of the next given sprite\nor include layers in JSON data"))
  , m_listTags(m_po.add("list-tags").description("List tags of the next given sprite sprite\nor include frame tags in JSON data"))
  , m_verbose(m_po.add("verbose").mnemonic('v').description("Explain what is being done"))
  , m_debug(m_po.add("debug").description("Extreme verbose mode and\ncopy log to desktop"))
  , m_help(m_po.add("help").mnemonic('?').description("Display this help and exits"))
  , m_version(m_po.add("version").description("Output version information and exit"))
{
  try {
    m_po.parse(argc, argv);

    if (m_po.enabled(m_debug))
      m_verboseLevel = kHighlyVerbose;
    else if (m_po.enabled(m_verbose))
      m_verboseLevel = kVerbose;

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
