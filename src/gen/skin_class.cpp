// Aseprite Code Generator
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "gen/skin_class.h"

#include "base/string.h"
#include "gen/common.h"

#include <iostream>
#include <vector>

void gen_skin_class(TiXmlDocument* doc, const std::string& inputFn)
{
  std::vector<std::string> colors;

  TiXmlHandle handle(doc);
  TiXmlElement* elem = handle
    .FirstChild("skin")
    .FirstChild("colors")
    .FirstChild("color").ToElement();
  while (elem) {
    const char* id = elem->Attribute("id");
    colors.push_back(id);
    elem = elem->NextSiblingElement();
  }

  std::cout
    << "// Don't modify, generated file from " << inputFn << "\n"
    << "\n"
    << "#ifndef GENERATED_SKIN_H_INCLUDED\n"
    << "#define GENERATED_SKIN_H_INCLUDED\n"
    << "#pragma once\n"
    << "\n"
    << "namespace app {\n"
    << "namespace gen {\n"
    << "\n"
    << "  template<typename T>\n"
    << "  class SkinFile {\n"
    << "  public:\n"
    << "\n"
    << "    class Colors {\n"
    << "      template<typename T> friend class SkinFile;\n"
    << "    public:\n";

  for (auto color : colors) {
    std::string id = convert_xmlid_to_cppid(color, false);
    std::cout
      << "      gfx::Color " << id << "() const { return m_" << id << "; }\n";
  }

  std::cout
    << "    private:\n";

  for (auto color : colors) {
    std::string id = convert_xmlid_to_cppid(color, false);
    std::cout
      << "      gfx::Color m_" << id << ";\n";
  }

  std::cout
    << "    };\n";

  std::cout
    << "\n"
    << "    Colors colors;\n"
    << "\n"
    << "  protected:\n"
    << "    void updateInternals() {\n";

  for (auto color : colors) {
    std::string id = convert_xmlid_to_cppid(color, false);
    std::cout << "      colors.m_" << id
              << " = colorById(\"" << color << "\");\n";
  }

  std::cout
    << "    }\n"
    << "\n"
    << "  private:\n"
    << "    gfx::Color colorById(const std::string& id) {\n"
    << "      return static_cast<T*>(this)->getColorById(id);\n"
    << "    }\n";

  std::cout
    << "  };\n"
    << "\n"
    << "} // namespace gen\n"
    << "} // namespace app\n"
    << "\n"
    << "#endif\n";
}
