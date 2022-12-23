// Aseprite
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cli/app_options.h"

#include "base/fs.h"

#include <iostream>

namespace app {

AppOptions::AppOptions(int argc, const char* argv[])
  : m_exeName(base::get_file_name(argv[0]))
  , m_startUI(true)
  , m_startShell(false)
  , m_previewCLI(false)
  , m_showHelp(false)
  , m_showVersion(false)
  , m_verboseLevel(kNoVerbose)
#ifdef ENABLE_SCRIPTING
  , m_shell(m_po.add("shell").description("Start an interactive console to execute scripts"))
#endif
  , m_batch(m_po.add("batch").mnemonic('b').description("Do not start the UI"))
  , m_preview(m_po.add("preview").mnemonic('p').description("Do not execute actions, just print what will be\ndone"))
  , m_saveAs(m_po.add("save-as").requiresValue("<filename>").description("Save the last given sprite with other format"))
  , m_palette(m_po.add("palette").requiresValue("<filename>").description("Change the palette of the last given sprite"))
  , m_scale(m_po.add("scale").requiresValue("<factor>").description("Resize all previously opened sprites"))
  , m_ditheringAlgorithm(m_po.add("dithering-algorithm").requiresValue("<algorithm>").description("Dithering algorithm used in --color-mode\nto convert images from RGB to Indexed\n  none\n  ordered\n  old"))
  , m_ditheringMatrix(m_po.add("dithering-matrix").requiresValue("<id>").description("Matrix used in ordered dithering algorithm\n  bayer2x2\n  bayer4x4\n  bayer8x8\n  filename.png"))
  , m_colorMode(m_po.add("color-mode").requiresValue("<mode>").description("Change color mode of all previously\nopened sprites:\n  rgb\n  grayscale\n  indexed"))
  , m_shrinkTo(m_po.add("shrink-to").requiresValue("width,height").description("Shrink each sprite if it is\nlarger than width or height"))
  , m_data(m_po.add("data").requiresValue("<filename.json>").description("File to store the sprite sheet metadata"))
  , m_format(m_po.add("format").requiresValue("<format>").description("Format to export the data file\n(json-hash, json-array)"))
  , m_sheet(m_po.add("sheet").requiresValue("<filename.png>").description("Image file to save the texture"))
  , m_sheetType(m_po.add("sheet-type").requiresValue("<type>").description("Algorithm to create the sprite sheet:\n  horizontal\n  vertical\n  rows\n  columns\n  packed"))
  , m_sheetPack(m_po.add("sheet-pack").description("Same as -sheet-type packed"))
  , m_sheetWidth(m_po.add("sheet-width").requiresValue("<pixels>").description("Sprite sheet width"))
  , m_sheetHeight(m_po.add("sheet-height").requiresValue("<pixels>").description("Sprite sheet height"))
  , m_sheetColumns(m_po.add("sheet-columns").requiresValue("<columns>").description("Fixed # of columns for -sheet-type rows"))
  , m_sheetRows(m_po.add("sheet-rows").requiresValue("<rows>").description("Fixed # of rows for -sheet-type columns"))
  , m_splitLayers(m_po.add("split-layers").description("Save each visible layer of sprites\nas separated images in the sheet\n"))
  , m_splitTags(m_po.add("split-tags").description("Save each tag as a separated file"))
  , m_splitSlices(m_po.add("split-slices").description("Save each slice as a separated file"))
  , m_splitGrid(m_po.add("split-grid").description("Save each grid tile as a separated file"))
  , m_layer(m_po.add("layer").alias("import-layer").requiresValue("<name>").description("Include just the given layer in the sheet\nor save as operation"))
  , m_allLayers(m_po.add("all-layers").description("Make all layers visible\nBy default hidden layers will be ignored"))
  , m_ignoreLayer(m_po.add("ignore-layer").requiresValue("<name>").description("Exclude the given layer in the sheet\nor save as operation"))
  , m_tag(m_po.add("tag").alias("frame-tag").requiresValue("<name>").description("Include tagged frames in the sheet"))
  , m_frameRange(m_po.add("frame-range").requiresValue("from,to").description("Only export frames in the [from,to] range"))
  , m_ignoreEmpty(m_po.add("ignore-empty").description("Do not export empty frames/cels"))
  , m_mergeDuplicates(m_po.add("merge-duplicates").description("Merge all duplicate frames into one in the sprite sheet"))
  , m_borderPadding(m_po.add("border-padding").requiresValue("<value>").description("Add padding on the texture borders"))
  , m_shapePadding(m_po.add("shape-padding").requiresValue("<value>").description("Add padding between frames"))
  , m_innerPadding(m_po.add("inner-padding").requiresValue("<value>").description("Add padding inside each frame"))
  , m_trim(m_po.add("trim").description("Trim whole sprite for --save-as\nor individual frames for --sheet"))
  , m_trimSprite(m_po.add("trim-sprite").description("Trim the whole sprite (for --save-as and --sheet)"))
  , m_trimByGrid(m_po.add("trim-by-grid").description("Trim all images by its correspondent grid boundaries before exporting"))
  , m_extrude(m_po.add("extrude").description("Extrude all images duplicating all edges one pixel"))
  , m_crop(m_po.add("crop").requiresValue("x,y,width,height").description("Crop all the images to the given rectangle"))
  , m_slice(m_po.add("slice").requiresValue("<name>").description("Crop the sprite to the given slice area"))
  , m_filenameFormat(m_po.add("filename-format").requiresValue("<fmt>").description("Special format to generate filenames"))
  , m_tagnameFormat(m_po.add("tagname-format").requiresValue("<fmt>").description("Special format to generate tagnames in JSON data"))
#ifdef ENABLE_SCRIPTING
  , m_script(m_po.add("script").requiresValue("<filename>").description("Execute a specific script"))
  , m_scriptParam(m_po.add("script-param").requiresValue("name=value").description("Parameter for a script executed from the\nCLI that you can access with app.params"))
#endif
  , m_listLayers(m_po.add("list-layers").description("List layers of the next given sprite\nor include layers in JSON data"))
  , m_listTags(m_po.add("list-tags").description("List tags of the next given sprite\nor include frame tags in JSON data"))
  , m_listSlices(m_po.add("list-slices").description("List slices of the next given sprite\nor include slices in JSON data"))
  , m_oneFrame(m_po.add("oneframe").description("Load just the first frame"))
  , m_exportTileset(m_po.add("export-tileset").description("Export only tilesets from visible tilemap layers"))
  , m_verbose(m_po.add("verbose").mnemonic('v').description("Explain what is being done"))
  , m_debug(m_po.add("debug").description("Extreme verbose mode and\ncopy log to desktop"))
#ifdef _WIN32
  , m_disableWintab(m_po.add("disable-wintab").description("Don't load wintab32.dll library"))
#endif
  , m_help(m_po.add("help").mnemonic('?').description("Display this help and exits"))
  , m_version(m_po.add("version").description("Output version information and exit"))
{
  try {
    m_po.parse(argc, argv);

    if (m_po.enabled(m_debug))
      m_verboseLevel = kHighlyVerbose;
    else if (m_po.enabled(m_verbose))
      m_verboseLevel = kVerbose;

#ifdef ENABLE_SCRIPTING
    m_startShell = m_po.enabled(m_shell);
#endif
    m_previewCLI = m_po.enabled(m_preview);
    m_showHelp = m_po.enabled(m_help);
    m_showVersion = m_po.enabled(m_version);

    if (m_startShell ||
        m_showHelp ||
        m_showVersion ||
        m_po.enabled(m_batch)) {
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

#ifdef _WIN32
bool AppOptions::disableWintab() const
{
  return m_po.enabled(m_disableWintab);
}
#endif

}
