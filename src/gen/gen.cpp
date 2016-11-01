// Aseprite Code Generator
// Copyright (c) 2014-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/file_handle.h"
#include "base/fs.h"
#include "base/program_options.h"
#include "base/string.h"
#include "gen/pref_types.h"
#include "gen/skin_class.h"
#include "gen/ui_class.h"
#include "tinyxml.h"

#include <iostream>

typedef base::ProgramOptions PO;

static void run(int argc, const char* argv[])
{
  PO po;
  PO::Option& inputOpt = po.add("input").requiresValue("<filename>");
  PO::Option& widgetId = po.add("widgetid").requiresValue("<filename>");
  PO::Option& prefH = po.add("pref-h");
  PO::Option& prefCpp = po.add("pref-cpp");
  PO::Option& skin = po.add("skin");
  po.parse(argc, argv);

  // Try to load the XML file
  TiXmlDocument* doc = NULL;

  std::string inputFilename = po.value_of(inputOpt);
  if (!inputFilename.empty()) {
    base::FileHandle inputFile(base::open_file(inputFilename, "rb"));
    doc = new TiXmlDocument();
    doc->SetValue(inputFilename.c_str());
    if (!doc->LoadFile(inputFile.get())) {
      std::cerr << doc->Value() << ":"
                << doc->ErrorRow() << ":"
                << doc->ErrorCol() << ": "
                << "error " << doc->ErrorId() << ": "
                << doc->ErrorDesc() << "\n";

      throw std::runtime_error("invalid input file");
    }
  }

  if (doc) {
    if (po.enabled(widgetId))
      gen_ui_class(doc, inputFilename, po.value_of(widgetId));
    else if (po.enabled(prefH))
      gen_pref_header(doc, inputFilename);
    else if (po.enabled(prefCpp))
      gen_pref_impl(doc, inputFilename);
    else if (po.enabled(skin))
      gen_skin_class(doc, inputFilename);
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
