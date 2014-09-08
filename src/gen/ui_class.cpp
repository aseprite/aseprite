// Aseprite Code Generator
// Copyright (c) 2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/exception.h"
#include "base/file_handle.h"
#include "base/path.h"
#include "base/program_options.h"
#include "base/string.h"
#include "gen/ui_class.h"

#include <cctype>
#include <iostream>

typedef base::ProgramOptions PO;
typedef std::vector<TiXmlElement*> XmlElements;

static std::string convert_xmlid_to_cppid(const std::string& xmlid, bool firstLetterUpperCase)
{
  bool firstLetter = firstLetterUpperCase;
  std::string cppid;
  for (size_t i=0; i<xmlid.size(); ++i) {
    if (xmlid[i] == '_') {
      firstLetter = true;
    }
    else if (firstLetter) {
      firstLetter = false;
      cppid += std::toupper(xmlid[i]);
    }
    else
      cppid += xmlid[i];
  }
  return cppid;
}

static TiXmlElement* find_element_by_id(TiXmlElement* elem, const std::string& thisId)
{
  const char* id = elem->Attribute("id");
  if (id && id == thisId)
    return elem;

  TiXmlElement* child = elem->FirstChildElement();
  while (child) {
    TiXmlElement* match = find_element_by_id(child, thisId);
    if (match)
      return match;

    child = child->NextSiblingElement();
  }

  return NULL;
}

static void collect_widgets_with_ids(TiXmlElement* elem, XmlElements& widgets)
{
  TiXmlElement* child = elem->FirstChildElement();
  while (child) {
    const char* id = child->Attribute("id");
    if (id)
      widgets.push_back(child);
    collect_widgets_with_ids(child, widgets);
    child = child->NextSiblingElement();
  }
}

static std::string convert_type(const std::string& name)
{
  if (name == "box") return "ui::Box";
  if (name == "button") return "ui::Button";
  if (name == "buttonset") return "app::ButtonSet";
  if (name == "check") return "ui::CheckBox";
  if (name == "combobox") return "ui::ComboBox";
  if (name == "entry") return "ui::Entry";
  if (name == "hbox") return "ui::HBox";
  if (name == "label") return "ui::Label";
  if (name == "link") return "ui::LinkLabel";
  if (name == "listbox") return "ui::ListBox";
  if (name == "panel") return "ui::Panel";
  if (name == "radio") return "ui::RadioButton";
  if (name == "slider") return "ui::Slider";
  if (name == "vbox") return "ui::VBox";
  if (name == "view") return "ui::View";
  if (name == "window") return "ui::Window";
  throw base::Exception("unknown widget name: " + name);
}

void gen_ui_class(TiXmlDocument* doc, const std::string& inputFn, const std::string& widgetId)
{
  std::cout
    << "// Don't modify, generated file from " << inputFn << "\n"
    << "\n";

  TiXmlHandle handle(doc);
  TiXmlElement* elem = handle.FirstChild("gui").ToElement();
  elem = find_element_by_id(elem, widgetId);
  if (!elem) {
    std::cout << "#error Widget not found: " << widgetId << "\n";
    return;
  }

  XmlElements widgets;
  collect_widgets_with_ids(elem, widgets);

  std::string className = convert_xmlid_to_cppid(widgetId, true);
  std::string fnUpper = base::string_to_upper(base::get_file_title(inputFn));
  std::string widgetType = convert_type(elem->Value());

  std::cout
    << "#ifndef GENERATED_" << fnUpper << "_H_INCLUDED\n"
    << "#define GENERATED_" << fnUpper << "_H_INCLUDED\n"
    << "#pragma once\n"
    << "\n"
    << "#include \"app/find_widget.h\"\n"
    << "#include \"app/load_widget.h\"\n"
    << "#include \"ui/ui.h\"\n"
    << "\n"
    << "namespace app {\n"
    << "namespace gen {\n"
    << "\n"
    << "  class " << className << " : public " << widgetType << " {\n"
    << "  public:\n"
    << "    " << className << "()";

  // Special ctor for base class
  if (widgetType == "ui::Window") {
    std::cout
      << " : ui::Window(ui::Window::WithTitleBar)";
  }

  std::cout
    << " {\n"
    << "      app::load_widget(\"" << base::get_file_name(inputFn) << "\", \"" << widgetId << "\", this);\n"
    << "      app::finder(this)\n";

  for (XmlElements::iterator it=widgets.begin(), end=widgets.end();
       it != end; ++it) {
    const char* id = (*it)->Attribute("id");
    std::string cppid = convert_xmlid_to_cppid(id, false);
    std::cout
      << "        >> \"" << id << "\" >> m_" << cppid << "\n";
  }

  std::cout
    << "      ;\n"
    << "    }\n"
    << "\n";

  for (XmlElements::iterator it=widgets.begin(), end=widgets.end();
       it != end; ++it) {
    std::string childType = convert_type((*it)->Value());
    const char* id = (*it)->Attribute("id");
    std::string cppid = convert_xmlid_to_cppid(id, false);
    std::cout
      << "    " << childType << "* " << cppid << "() { return m_" << cppid << "; }\n";
  }

  std::cout
    << "\n"
    << "  private:\n";

  for (XmlElements::iterator it=widgets.begin(), end=widgets.end();
       it != end; ++it) {
    std::string childType = convert_type((*it)->Value());
    const char* id = (*it)->Attribute("id");
    std::string cppid = convert_xmlid_to_cppid(id, false);
    std::cout
      << "    " << childType << "* m_" << cppid << ";\n";
  }

  std::cout
    << "  };\n"
    << "\n"
    << "} // namespace gen\n"
    << "} // namespace app\n"
    << "\n"
    << "#endif\n";
}
