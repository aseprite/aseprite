// Aseprite Code Generator
// Copyright (c) 2014-2018 David Capello
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
#include <set>
#include <vector>

typedef std::vector<TiXmlElement*> XmlElements;

namespace {

struct Item {
  std::string xmlid;
  std::string cppid;
  std::string type;
  std::string incl;
  const Item& typeIncl(const char* type,
                       const char* incl) {
    this->type = type;
    this->incl = incl;
    return *this;
  }
};

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

static Item convert_to_item(TiXmlElement* elem)
{
  static std::string parent;
  const std::string name = elem->Value();
  if (name != "item")
    parent = name;

  Item item;
  if (elem->Attribute("id")) {
    item.xmlid = elem->Attribute("id");
    item.cppid = convert_xmlid_to_cppid(item.xmlid, false);
  }

  if (name == "box")
    return item.typeIncl("ui::Box",
                         "ui/box.h");
  if (name == "button")
    return item.typeIncl("ui::Button",
                         "ui/button.h");
  if (name == "buttonset")
    return item.typeIncl("app::ButtonSet",
                         "app/ui/button_set.h");
  if (name == "check") {
    if (elem->Attribute("pref") != nullptr)
      return item.typeIncl("BoolPrefWidget<ui::CheckBox>",
                           "app/ui/pref_widget.h");
    else
      return item.typeIncl("ui::CheckBox",
                           "ui/button.h");
  }
  if (name == "colorpicker")
    return item.typeIncl("app::ColorButton",
                         "app/ui/color_button.h");
  if (name == "combobox")
    return item.typeIncl("ui::ComboBox",
                         "ui/combobox.h");
  if (name == "dropdownbutton")
    return item.typeIncl("app::DropDownButton",
                         "app/ui/drop_down_button.h");
  if (name == "entry")
    return item.typeIncl("ui::Entry",
                         "ui/entry.h");
  if (name == "expr")
    return item.typeIncl("app::ExprEntry",
                         "app/ui/expr_entry.h");
  if (name == "grid")
    return item.typeIncl("ui::Grid",
                         "ui/grid.h");
  if (name == "hbox")
    return item.typeIncl("ui::HBox",
                         "ui/box.h");
  if (name == "item" &&
      parent == "buttonset")
    return item.typeIncl("app::ButtonSet::Item",
                         "app/ui/button_set.h");
  if (name == "label")
    return item.typeIncl("ui::Label",
                         "ui/label.h");
  if (name == "link")
    return item.typeIncl("ui::LinkLabel",
                         "ui/link_label.h");
  if (name == "listbox")
    return item.typeIncl("ui::ListBox",
                         "ui/listbox.h");
  if (name == "panel")
    return item.typeIncl("ui::Panel",
                         "ui/panel.h");
  if (name == "popupwindow")
    return item.typeIncl("ui::PopupWindow",
                         "ui/popup_window.h");
  if (name == "radio")
    return item.typeIncl("ui::RadioButton",
                         "ui/button.h");
  if (name == "search")
    return item.typeIncl("app::SearchEntry",
                         "app/ui/search_entry.h");
  if (name == "separator")
    return item.typeIncl("ui::Separator",
                         "ui/separator.h");
  if (name == "slider")
    return item.typeIncl("ui::Slider",
                         "ui/slider.h");
  if (name == "splitter")
    return item.typeIncl("ui::Splitter",
                         "ui/splitter.h");
  if (name == "tipwindow")
    return item.typeIncl("ui::TipWindow",
                         "ui/tooltips.h");
  if (name == "vbox")
    return item.typeIncl("ui::VBox",
                         "ui/box.h");
  if (name == "view")
    return item.typeIncl("ui::View",
                         "ui/view.h");
  if (name == "window")
    return item.typeIncl("ui::Window",
                         "ui/window.h");

  throw base::Exception("Unknown widget name: " + name);
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

  std::vector<Item> items;
  {
    XmlElements xmlWidgets;
    collect_widgets_with_ids(elem, xmlWidgets);
    for (TiXmlElement* elem : xmlWidgets) {
      const char* id = elem->Attribute("id");
      if (!id)
        continue;
      items.push_back(convert_to_item(elem));
    }
  }

  std::string className = convert_xmlid_to_cppid(widgetId, true);
  std::string fnUpper = base::string_to_upper(base::get_file_title(inputFn));
  Item mainWidget = convert_to_item(elem);

  std::set<std::string> headerFiles;
  headerFiles.insert("app/find_widget.h");
  headerFiles.insert("app/load_widget.h");
  headerFiles.insert(mainWidget.incl);
  for (const Item& item : items)
    headerFiles.insert(item.incl);

  std::cout
    << "#ifndef GENERATED_" << fnUpper << "_H_INCLUDED\n"
    << "#define GENERATED_" << fnUpper << "_H_INCLUDED\n"
    << "#pragma once\n"
    << "\n";
  for (const auto& incl : headerFiles)
    std::cout << "#include \"" << incl << "\"\n";
  std::cout
    << "\n"
    << "namespace app {\n"
    << "namespace gen {\n"
    << "\n"
    << "  class " << className << " : public " << mainWidget.type << " {\n"
    << "  public:\n"
    << "    " << className << "()";

  // Special ctor for base class
  if (mainWidget.type == "ui::Window") {
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

  for (const Item& item : items) {
    std::cout
      << "        >> \"" << item.xmlid << "\" >> m_" << item.cppid << "\n";
  }

  std::cout
    << "      ;\n"
    << "    }\n"
    << "\n";

  for (const Item& item : items) {
    std::cout
      << "    " << item.type << "* " << item.cppid << "() const { return m_" << item.cppid << "; }\n";
  }

  std::cout
    << "\n"
    << "  private:\n";

  for (const Item& item : items) {
    std::cout
      << "    " << item.type << "* m_" << item.cppid << ";\n";
  }

  std::cout
    << "  };\n"
    << "\n"
    << "} // namespace gen\n"
    << "} // namespace app\n"
    << "\n"
    << "#endif\n";
}
