// Aseprite Code Generator
// Copyright (c) 2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/file_handle.h"
#include "base/path.h"
#include "base/program_options.h"
#include "base/string.h"
#include "gen/ui_class.h"
#include "tinyxml.h"

#include <iostream>

typedef base::ProgramOptions PO;

static void run(int argc, const char* argv[])
{
  PO po;
  PO::Option& inputOpt = po.add("input").requiresValue("<filename>");
  PO::Option& widgetId = po.add("widgetid").requiresValue("<filename>");
  po.parse(argc, argv);

  // Try to load the XML file
  TiXmlDocument* doc = NULL;

  std::string inputFilename = po.value_of(inputOpt);
  if (!inputFilename.empty()) {
    base::FileHandle inputFile(base::open_file(inputFilename, "rb"));
    doc = new TiXmlDocument();
    doc->SetValue(inputFilename.c_str());
    if (!doc->LoadFile(inputFile))
      throw std::runtime_error("invalid input file");
  }

  if (doc && po.enabled(widgetId))
    gen_ui_class(doc, inputFilename, po.value_of(widgetId));
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
