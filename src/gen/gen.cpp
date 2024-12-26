// Aseprite Code Generator
// Copyright (c) 2021-2024 Igara Studio S.A.
// Copyright (c) 2014-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/file_handle.h"
#include "base/fs.h"
#include "base/program_options.h"
#include "base/string.h"
#include "gen/check_strings.h"
#include "gen/pref_types.h"
#include "gen/strings_class.h"
#include "gen/theme_class.h"
#include "gen/ui_class.h"
#include "tinyxml2.h"

#include <iostream>
#include <memory>

using PO = base::ProgramOptions;
using namespace tinyxml2;

static void run(int argc, const char* argv[])
{
  PO po;
  PO::Option& inputOpt = po.add("input").requiresValue("<filename>");
  PO::Option& widgetId = po.add("widgetid").requiresValue("<id>");
  PO::Option& prefH = po.add("pref-h");
  PO::Option& prefCpp = po.add("pref-cpp");
  PO::Option& theme = po.add("theme");
  PO::Option& strings = po.add("strings");
  PO::Option& commandIds = po.add("command-ids");
  PO::Option& widgetsDir = po.add("widgets-dir").requiresValue("<dir>");
  PO::Option& stringsDir = po.add("strings-dir").requiresValue("<dir>");
  PO::Option& guiFile = po.add("gui-file").requiresValue("<filename>");
  po.parse(argc, argv);

  // Try to load the XML file
  std::unique_ptr<XMLDocument> doc;

  std::string inputFilename = po.value_of(inputOpt);
  if (!inputFilename.empty() && base::get_file_extension(inputFilename) == "xml") {
    base::FileHandle inputFile(base::open_file(inputFilename, "rb"));
    doc = std::make_unique<XMLDocument>();
    if (doc->LoadFile(inputFile.get()) != XML_SUCCESS) {
      std::cerr << inputFilename << ":" << doc->ErrorLineNum() << ": "
                << "error " << int(doc->ErrorID()) << ": " << doc->ErrorStr() << "\n";
      throw std::runtime_error("invalid input file");
    }
  }

  if (doc) {
    // Generate widget class
    if (po.enabled(widgetId))
      gen_ui_class(doc.get(), inputFilename, po.value_of(widgetId));
    // Generate preference header file
    else if (po.enabled(prefH))
      gen_pref_header(doc.get(), inputFilename);
    // Generate preference c++ file
    else if (po.enabled(prefCpp))
      gen_pref_impl(doc.get(), inputFilename);
    // Generate theme class
    else if (po.enabled(theme))
      gen_theme_class(doc.get(), inputFilename);
  }
  // Generate strings.ini.h file
  else if (po.enabled(strings)) {
    gen_strings_class(inputFilename);
  }
  // Generate command_ids.ini.h file
  else if (po.enabled(commandIds)) {
    gen_command_ids(inputFilename);
  }
  // Check all translation files (en.ini, es.ini, etc.)
  else if (po.enabled(widgetsDir) && po.enabled(stringsDir)) {
    check_strings(po.value_of(widgetsDir), po.value_of(stringsDir), po.value_of(guiFile));
  }
}

int main(int argc, const char* argv[])
{
  try {
    run(argc, argv);
    return 0;
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
