// Aseprite Code Generator
// Copyright (c) 2014-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/exception.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/split_string.h"
#include "base/string.h"
#include "gen/common.h"
#include "gen/pref_types.h"

#include <iostream>
#include <stdexcept>
#include <vector>

typedef std::vector<TiXmlElement*> XmlElements;

static void print_pref_class_def(TiXmlElement* elem, const std::string& className, const char* section, int indentSpaces)
{
  std::string indent(indentSpaces, ' ');
  std::cout
    << "\n"
    << indent << "class " << className << " : public Section {\n"
    << indent << "public:\n"
    << indent << "  " << className << "(const std::string& name);\n";

  std::cout
    << indent << "  void load();\n"
    << indent << "  void save();\n"
    << indent << "  Section* section(const char* id) override;\n"
    << indent << "  OptionBase* option(const char* id) override;\n";

  TiXmlElement* child = (elem->FirstChild() ? elem->FirstChild()->ToElement(): NULL);
  while (child) {
    if (child->Value()) {
      std::string name = child->Value();
      const char* childId = child->Attribute("id");

      if (name == "option") {
        if (!child->Attribute("type")) throw std::runtime_error("missing 'type' attr in <option>");
        if (!childId) throw std::runtime_error("missing 'id' attr in <option>");
        std::string memberName = convert_xmlid_to_cppid(childId, false);
        std::cout
          << indent << "  Option<" << child->Attribute("type") << "> " << memberName << ";\n";
      }
      else if (name == "section") {
        if (!childId) throw std::runtime_error("missing 'id' attr in <section>");
        std::string childClassName = convert_xmlid_to_cppid(childId, true);
        std::string memberName = convert_xmlid_to_cppid(childId, false);
        print_pref_class_def(child, childClassName, childId, indentSpaces+2);
        std::cout
          << indent << "  " << childClassName << " " << memberName << ";\n";
      }
    }
    child = child->NextSiblingElement();
  }

  std::cout
    << indent << "};\n";
}

static void print_pref_class_impl(TiXmlElement* elem, const std::string& prefix, const std::string& className, const char* section)
{
  std::cout
    << "\n"
    << prefix << className << "::" << className << "(const std::string& name)\n";

  if (section)
    std::cout << "  : Section(std::string(!name.empty() ? name + \".\": \"\") + \"" << section << "\")\n";
  else
    std::cout << "  : Section(name)\n";

  TiXmlElement* child = (elem->FirstChild() ? elem->FirstChild()->ToElement(): NULL);
  while (child) {
    if (child->Value()) {
      std::string name = child->Value();
      const char* childId = child->Attribute("id");

      if (name == "option") {
        if (!child->Attribute("type")) throw std::runtime_error("missing 'type' attr in <option>");
        if (!childId) throw std::runtime_error("missing 'id' attr in <option>");
        std::string memberName = convert_xmlid_to_cppid(childId, false);
        std::cout << "  , "
                  << memberName << "(this, \"" << childId << "\"";
        if (child->Attribute("default"))
          std::cout << ", " << child->Attribute("default");
        std::cout << ")\n";
      }
      else if (name == "section") {
        if (!childId) throw std::runtime_error("missing 'id' attr in <option>");
        std::string memberName = convert_xmlid_to_cppid(childId, false);
        std::cout << "  , " << memberName << "(name)\n";
      }
    }
    child = child->NextSiblingElement();
  }

  std::cout
    << "{\n"
    << "}\n";

  // Section::load()

  std::cout
    << "\n"
    << "void " << prefix << className << "::load()\n"
    << "{\n";

  child = (elem->FirstChild() ? elem->FirstChild()->ToElement(): NULL);
  while (child) {
    if (child->Value()) {
      std::string name = child->Value();
      const char* childId = child->Attribute("id");

      if (name == "option") {
        std::string memberName = convert_xmlid_to_cppid(childId, false);

        const char* migrate = child->Attribute("migrate");
        if (migrate) {
          std::vector<std::string> parts;
          base::split_string(migrate, parts, ".");

          std::cout << "  load_option_with_migration(" << memberName
                    << ", \"" << parts[0]
                    << "\", \"" << parts[1] << "\");\n";
        }
        else
          std::cout << "  load_option(" << memberName << ");\n";
      }
      else if (name == "section") {
        std::string memberName = convert_xmlid_to_cppid(childId, false);
        std::cout << "  " << memberName << ".load();\n";
      }
    }
    child = child->NextSiblingElement();
  }

  std::cout
    << "}\n"
    << "\n";

  // Section::save()

  std::cout
    << "void " << prefix << className << "::save()\n"
    << "{\n";

  child = (elem->FirstChild() ? elem->FirstChild()->ToElement(): NULL);
  while (child) {
    if (child->Value()) {
      std::string name = child->Value();
      if (name == "option") {
        std::string memberName = convert_xmlid_to_cppid(child->Attribute("id"), false);
        std::cout << "  save_option(" << memberName << ");\n";
      }
      else if (name == "section") {
        std::string memberName = convert_xmlid_to_cppid(child->Attribute("id"), false);
        std::cout << "  " << memberName << ".save();\n";
      }
    }
    child = child->NextSiblingElement();
  }

  std::cout
    << "}\n"
    << "\n";

  // Section::section(id)

  std::cout
    << "Section* " << prefix << className << "::section(const char* id)\n"
    << "{\n";

  child = (elem->FirstChild() ? elem->FirstChild()->ToElement(): NULL);
  while (child) {
    if (child->Value()) {
      std::string name = child->Value();
      const char* childId = child->Attribute("id");
      if (name == "section") {
        std::string memberName = convert_xmlid_to_cppid(childId, false);
        std::cout << "  if (std::strcmp(id, " << memberName << ".name()) == 0) return &" << memberName << ";\n";
      }
    }
    child = child->NextSiblingElement();
  }

  std::cout
    << "  return nullptr;\n"
    << "}\n"
    << "\n";

  // Section::option(id)

  std::cout
    << "OptionBase* " << prefix << className << "::option(const char* id)\n"
    << "{\n";

  child = (elem->FirstChild() ? elem->FirstChild()->ToElement(): NULL);
  while (child) {
    if (child->Value()) {
      std::string name = child->Value();
      const char* childId = child->Attribute("id");
      if (name == "option") {
        std::string memberName = convert_xmlid_to_cppid(childId, false);
        std::cout << "  if (std::strcmp(id, " << memberName << ".id()) == 0) return &" << memberName << ";\n";
      }
    }
    child = child->NextSiblingElement();
  }

  std::cout
    << "  return nullptr;\n"
    << "}\n";

  // Sub-sections

  child = (elem->FirstChild() ? elem->FirstChild()->ToElement(): NULL);
  while (child) {
    if (child->Value()) {
      std::string name = child->Value();
      if (name == "section") {
        std::string childClassName = convert_xmlid_to_cppid(child->Attribute("id"), true);
        print_pref_class_impl(child, className + "::", childClassName, child->Attribute("id"));
      }
    }
    child = child->NextSiblingElement();
  }
}

void gen_pref_header(TiXmlDocument* doc, const std::string& inputFn)
{
  std::cout
    << "// Don't modify, generated file from " << inputFn << "\n"
    << "\n";

  std::cout
    << "#ifndef GENERATED_PREF_TYPES_H_INCLUDED\n"
    << "#define GENERATED_PREF_TYPES_H_INCLUDED\n"
    << "#pragma once\n"
    << "\n"
    << "#include <string>\n"
    << "\n"
    << "namespace app {\n"
    << "namespace gen {\n";

  TiXmlHandle handle(doc);
  TiXmlElement* elem = handle
    .FirstChild("preferences")
    .FirstChild("types")
    .FirstChild("enum").ToElement();
  while (elem) {
    if (!elem->Attribute("id")) throw std::runtime_error("missing 'id' attr in <enum>");
    std::cout
      << "\n"
      << "  enum class " << elem->Attribute("id") << " {\n";

    TiXmlElement* child = elem->FirstChildElement("value");
    while (child) {
      if (!child->Attribute("id")) throw std::runtime_error("missing 'id' attr in <value>");
      if (!child->Attribute("value")) throw std::runtime_error("missing 'value' attr in <value>");

      std::cout << "    " << child->Attribute("id") << " = "
                << child->Attribute("value") << ",\n";
      child = child->NextSiblingElement("value");
    }

    std::cout
      << "  };\n";

    elem = elem->NextSiblingElement("enum");
  }

  elem = handle
    .FirstChild("preferences")
    .FirstChild("global").ToElement();
  if (elem)
    print_pref_class_def(elem, "GlobalPref", NULL, 2);

  elem = handle
    .FirstChild("preferences")
    .FirstChild("tool").ToElement();
  if (elem)
    print_pref_class_def(elem, "ToolPref", NULL, 2);

  elem = handle
    .FirstChild("preferences")
    .FirstChild("document").ToElement();
  if (elem)
    print_pref_class_def(elem, "DocPref", NULL, 2);

  std::cout
    << "\n"
    << "} // namespace gen\n"
    << "} // namespace app\n"
    << "\n"
    << "#endif\n";
}

void gen_pref_impl(TiXmlDocument* doc, const std::string& inputFn)
{
  std::cout
    << "// Don't modify, generated file from " << inputFn << "\n"
    << "\n"
    << "#ifdef HAVE_CONFIG_H\n"
    << "#include \"config.h\"\n"
    << "#endif\n"
    << "\n"
    << "#include \"app/pref/option_io.h\"\n"
    << "#include \"app/pref/preferences.h\"\n"
    << "\n"
    << "#include <cstring>\n"
    << "\n"
    << "namespace app {\n"
    << "namespace gen {\n";

  TiXmlHandle handle(doc);
  TiXmlElement* elem = handle
    .FirstChild("preferences")
    .FirstChild("global").ToElement();
  if (elem)
    print_pref_class_impl(elem, "", "GlobalPref", NULL);

  elem = handle
    .FirstChild("preferences")
    .FirstChild("tool").ToElement();
  if (elem)
    print_pref_class_impl(elem, "", "ToolPref", NULL);

  elem = handle
    .FirstChild("preferences")
    .FirstChild("document").ToElement();
  if (elem)
    print_pref_class_impl(elem, "", "DocPref", NULL);

  std::cout
    << "\n"
    << "} // namespace gen\n"
    << "} // namespace app\n";
}
