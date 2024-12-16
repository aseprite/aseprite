// Aseprite Code Generator
// Copyright (c) 2024 Igara Studio S.A.
// Copyright (c) 2016-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "gen/strings_class.h"

#include "base/fs.h"
#include "base/replace_string.h"
#include "base/string.h"
#include "cfg/cfg.h"

#include <algorithm>
#include <cctype>
#include <iostream>

static std::string to_cpp(std::string stringId)
{
  base::replace_string(stringId, ".", "_");
  return stringId;
}

static size_t count_fmt_args(const std::string& format)
{
  size_t n = 0;
  for (size_t i = 0; i < format.size(); ++i) {
    if (format[i] == '{') {
      if (format[i + 1] == '}') {
        ++n;
      }
      else if (std::isdigit(format[i + 1]) && format[i + 2] == '}') {
        n = std::max<size_t>(n, (format[i + 1] - '0') + 1);
      }
    }
  }
  return n;
}

// Strings with raw "{something}" that are not a fmt format string.
//
// TODO This is a super hacky way to filter out these strings, find
//      another way.
static bool force_simple_string(const std::string& stringId)
{
  return (stringId == "data_filename_format_tooltip" || stringId == "data_tagname_format_tooltip");
}

void gen_strings_class(const std::string& inputFn)
{
  cfg::CfgFile cfg;
  cfg.load(inputFn);

  std::cout << "// Don't modify, generated file from " << inputFn << "\n"
            << "\n";

  std::cout << "#ifndef GENERATED_STRINGS_INI_H_INCLUDED\n"
            << "#define GENERATED_STRINGS_INI_H_INCLUDED\n"
            << "#pragma once\n"
            << "\n"
            << "namespace app {\n"
            << "namespace gen {\n"
            << "\n"
            << "  template<typename T>\n"
            << "  class Strings {\n"
            << "  public:\n"
            << "    class ID {\n"
            << "    public:\n";

  std::vector<std::string> sections;
  std::vector<std::string> keys;
  cfg.getAllSections(sections);
  for (const auto& section : sections) {
    keys.clear();
    cfg.getAllKeys(section.c_str(), keys);

    std::string textId = section;
    textId.push_back('.');
    for (const auto& key : keys) {
      textId.append(key);
      std::cout << "      static constexpr const char* " << to_cpp(textId) << " = \"" << textId
                << "\";\n";
      textId.erase(section.size() + 1);
    }
  }

  std::cout << "    };\n"
            << "\n";

  for (const auto& section : sections) {
    keys.clear();
    cfg.getAllKeys(section.c_str(), keys);

    std::string textId = section;
    textId.push_back('.');
    for (const auto& key : keys) {
      textId.append(key);

      std::string value = cfg.getValue(section.c_str(), key.c_str(), "");

      std::string cppId = to_cpp(textId);
      size_t nargs = count_fmt_args(value);

      // Create just a function to get the translated string (it
      // doesn't have arguments).
      if (nargs == 0 || force_simple_string(cppId)) {
        std::cout << "    static const std::string& " << cppId
                  << "() { return T::Translate(ID::" << cppId << "); }\n";
      }
      // Create a function to format the translated string with a
      // specific number of arguments (the good part is that we can
      // check the number of required arguments at compile-time).
      else {
        std::cout << "    template<";
        for (int i = 1; i <= nargs; ++i) {
          std::cout << "typename T" << i;
          if (i < nargs)
            std::cout << ", ";
        }
        std::cout << ">\n";
        std::cout << "    static std::string " << cppId << "(";
        for (int i = 1; i <= nargs; ++i) {
          std::cout << "T" << i << "&& arg" << i;
          if (i < nargs)
            std::cout << ", ";
        }
        std::cout << ") { return T::Format(ID::" << cppId << ", ";
        for (int i = 1; i <= nargs; ++i) {
          std::cout << "arg" << i;
          if (i < nargs)
            std::cout << ", ";
        }
        std::cout << "); }\n";
      }

      textId.erase(section.size() + 1);
    }
  }

  std::cout << "  };\n"
            << "\n"
            << "} // namespace gen\n"
            << "} // namespace app\n"
            << "\n"
            << "#endif\n";
}

void gen_command_ids(const std::string& inputFn)
{
  cfg::CfgFile cfg;
  cfg.load(inputFn);

  std::cout << "// Don't modify, generated file from " << inputFn << "\n"
            << "\n";

  std::cout << "#ifndef GENERATED_COMMAND_IDS_H_INCLUDED\n"
            << "#define GENERATED_COMMAND_IDS_H_INCLUDED\n"
            << "#pragma once\n"
            << "\n"
            << "namespace app {\n"
            << "namespace gen {\n"
            << "\n"
            << "  class CommandId {\n"
            << "  public:\n";

  std::vector<std::string> keys;
  cfg.getAllKeys("commands", keys);
  for (auto key : keys) {
    if (key.find('_') != std::string::npos)
      continue;

    std::cout << "    static const char* " << key << "() { return \"" << key << "\"; }\n";
  }

  std::cout << "  };\n"
            << "\n"
            << "} // namespace gen\n"
            << "} // namespace app\n"
            << "\n"
            << "#endif\n";
}
