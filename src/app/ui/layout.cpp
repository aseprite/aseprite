// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/layout.h"

#include "app/app.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/dock.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui/toolbar.h"
#include "app/ui/workspace.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "base/convert_to.h"
#include "ui/widget.h"

#include <cstring>
#include <sstream>

namespace app {

static void save_dock_layout(TiXmlElement& elem, const Dock* dock)
{
  for (const auto child : dock->children()) {
    const int side = dock->whichSideChildIsDocked(child);
    const gfx::Size size = dock->getUserDefinedSizeAtSide(side);

    std::string sideStr;
    switch (side) {
      case ui::LEFT:   sideStr = "left"; break;
      case ui::RIGHT:  sideStr = "right"; break;
      case ui::TOP:    sideStr = "top"; break;
      case ui::BOTTOM: sideStr = "bottom"; break;
      case ui::CENTER:
        // Empty side attribute
        break;
    }

    TiXmlElement childElem("");

    if (auto subdock = dynamic_cast<const Dock*>(child)) {
      childElem.SetValue("dock");
      if (!sideStr.empty())
        childElem.SetAttribute("side", sideStr);

      save_dock_layout(childElem, subdock);
    }
    else {
      // Set the widget ID as the element name, e.g. <timeline />,
      // <colorbar />, etc.
      childElem.SetValue(child->id());
      if (!sideStr.empty())
        childElem.SetAttribute("side", sideStr);
      if (size.w)
        childElem.SetAttribute("width", size.w);
      if (size.h)
        childElem.SetAttribute("height", size.h);
    }

    elem.InsertEndChild(childElem);
  }
}

static void load_dock_layout(const TiXmlElement* elem, Dock* dock)
{
  const char* elemNameStr = elem->Value();
  if (!elemNameStr) {
    ASSERT(false); // Impossible?
    return;
  }
  const std::string elemName = elemNameStr;

  MainWindow* win = App::instance()->mainWindow();
  ASSERT(win);

  ui::Widget* widget = nullptr;
  Dock* subdock = nullptr;

  int side = ui::CENTER;
  if (auto sideStr = elem->Attribute("side")) {
    if (std::strcmp(sideStr, "left") == 0)
      side = ui::LEFT;
    if (std::strcmp(sideStr, "right") == 0)
      side = ui::RIGHT;
    if (std::strcmp(sideStr, "top") == 0)
      side = ui::TOP;
    if (std::strcmp(sideStr, "bottom") == 0)
      side = ui::BOTTOM;
  }

  const char* widthStr = elem->Attribute("width");
  const char* heightStr = elem->Attribute("height");
  gfx::Size size;
  if (widthStr)
    size.w = base::convert_to<int>(std::string(widthStr));
  if (heightStr)
    size.h = base::convert_to<int>(std::string(heightStr));

  if (elemName == "colorbar") {
    widget = win->colorBar();
  }
  else if (elemName == "contextbar") {
    widget = win->getContextBar();
  }
  else if (elemName == "timeline") {
    widget = win->getTimeline();
  }
  else if (elemName == "toolbar") {
    widget = win->toolBar();
  }
  else if (elemName == "workspace") {
    widget = win->getWorkspace();
  }
  else if (elemName == "dock") {
    subdock = dock->subdock(side);
  }

  if (subdock) {
    auto childElem = elem->FirstChildElement();
    while (childElem) {
      load_dock_layout(childElem, subdock);
      childElem = childElem->NextSiblingElement();
    }
  }
  else {
    dock->dock(side, widget, size);
  }
}

Layout::Layout(const std::string& name, const Dock* dock) : m_name(name)
{
  XmlDocumentRef doc(new TiXmlDocument());
  TiXmlElement layoutsElem("layouts");
  {
    TiXmlElement layoutElem("layout");
    layoutElem.SetAttribute("name", name);

    save_dock_layout(layoutElem, dock);

    layoutsElem.InsertEndChild(layoutElem);
  }

  TiXmlDeclaration declaration("1.0", "utf-8", "");
  doc->InsertEndChild(declaration);
  doc->InsertEndChild(layoutsElem);

  std::stringstream s;
  s << *doc;
  m_data = s.str();
}

bool Layout::loadLayout(Dock* dock) const
{
  XmlDocumentRef doc(new TiXmlDocument);
  doc->Parse(m_data.c_str(), 0, TIXML_DEFAULT_ENCODING);
  TiXmlHandle handle(doc.get());

  TiXmlElement* layoutElem = handle.FirstChild("layouts").FirstChild("layout").ToElement();

  if (!layoutElem)
    return false;

  TiXmlElement* elem = layoutElem->FirstChildElement();
  while (elem) {
    load_dock_layout(elem, dock);
    elem = elem->NextSiblingElement();
  }

  return true;
}

} // namespace app
