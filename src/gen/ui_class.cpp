// Aseprite Code Generator
// Copyright (c) 2014-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "gen/ui_class.h"

#include "base/exception.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/string.h"
#include "gen/common.h"

#include <iostream>
#include <vector>

typedef std::vector<TiXmlElement*> XmlElements;

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
  static std::string parent;
  if (name != "item")
    parent = name;

  if (name == "box") return "ui::Box";
  if (name == "button") return "ui::Button";
  if (name == "buttonset") return "app::ButtonSet";
  if (name == "check") return "ui::CheckBox";
  if (name == "colorpicker") return "app::ColorButton";
  if (name == "combobox") return "ui::ComboBox";
  if (name == "dropdownbutton") return "app::DropDownButton";
  if (name == "entry") return "ui::Entry";
  if (name == "grid") return "ui::Grid";
  if (name == "hbox") return "ui::HBox";
  if (name == "item" && parent == "buttonset") return "app::ButtonSet::Item";
  if (name == "label") return "ui::Label";
  if (name == "link") return "ui::LinkLabel";
  if (name == "listbox") return "ui::ListBox";
  if (name == "panel") return "ui::Panel";
  if (name == "popupwindow") return "ui::PopupWindow";
  if (name == "radio") return "ui::RadioButton";
  if (name == "search") return "app::SearchEntry";
  if (name == "separator") return "ui::Separator";
  if (name == "slider") return "ui::Slider";
  if (name == "splitter") return "ui::Splitter";
  if (name == "tipwindow") return "ui::TipWindow";
  if (name == "vbox") return "ui::VBox";
  if (name == "view") return "ui::View";
  if (name == "window") return "ui::Window";
  throw base::Exception("unknown widget name: " + name);
}

void gen_ui_class(TiXmlDocument* doc,
                  const std::string& inputFn,
                  const std::string& widgetId)
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
    const char* desktop = elem->Attribute("desktop");
    if (desktop && std::string(desktop) == "true")
      std::cout
        << " : ui::Window(ui::Window::DesktopWindow)";
    else
      std::cout
        << " : ui::Window(ui::Window::WithTitleBar)";
  }

  std::cout
    << " {\n"
    << "      app::load_widget(\"" << base::get_file_name(inputFn) << "\", \"" << widgetId << "\", this);\n"
    << "      app::finder(this)\n";

  for (TiXmlElement* elem : widgets) {
    const char* id = elem->Attribute("id");
    std::string cppid = convert_xmlid_to_cppid(id, false);
    std::cout
      << "        >> \"" << id << "\" >> m_" << cppid << "\n";
  }

  std::cout
    << "      ;\n"
    << "    }\n"
    << "\n";

  for (TiXmlElement* elem : widgets) {
    std::string childType = convert_type(elem->Value());
    const char* id = elem->Attribute("id");
    std::string cppid = convert_xmlid_to_cppid(id, false);
    std::cout
      << "    " << childType << "* " << cppid << "() const { return m_" << cppid << "; }\n";
  }

  std::cout
    << "\n"
    << "  private:\n";

  for (TiXmlElement* elem : widgets) {
    std::string childType = convert_type(elem->Value());
    const char* id = elem->Attribute("id");
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
