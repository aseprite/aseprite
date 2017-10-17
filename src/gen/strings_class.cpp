// Aseprite Code Generator
// Copyright (c) 2016-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "gen/strings_class.h"

#include "base/fs.h"
#include "base/replace_string.h"
#include "base/string.h"
#include "cfg/cfg.h"

#include <iostream>

static std::string to_cpp(std::string stringId)
{
  base::replace_string(stringId, ".", "_");
  return stringId;
}

void gen_strings_class(const std::string& inputFn)
{
  cfg::CfgFile cfg;
  cfg.load(inputFn);

  std::cout
    << "// Don't modify, generated file from " << inputFn << "\n"
    << "\n";

  std::string fnUpper = base::string_to_upper(base::get_file_title(inputFn));

  std::cout
    << "#ifndef GENERATED_" << fnUpper << "_H_INCLUDED\n"
    << "#define GENERATED_" << fnUpper << "_H_INCLUDED\n"
    << "#pragma once\n"
    << "\n"
    << "namespace app {\n"
    << "namespace gen {\n"
    << "\n"
    << "  template<typename T>\n"
    << "  class Strings {\n"
    << "  public:\n";

  std::vector<std::string> sections;
  std::vector<std::string> keys;
  cfg.getAllSections(sections);
  for (const auto& section : sections) {
    keys.clear();
    cfg.getAllKeys(section.c_str(), keys);

    std::string textId = section;
    textId.push_back('.');
    for (auto key : keys) {
      textId.append(key);

      std::cout << "    static const std::string& " << to_cpp(textId) << "() { return T::instance()->translate(\"" << textId << "\"); }\n";

      textId.erase(section.size()+1);
    }
  }

  std::cout
    << "  };\n"
    << "\n"
    << "} // namespace gen\n"
    << "} // namespace app\n"
    << "\n"
    << "#endif\n";
}
