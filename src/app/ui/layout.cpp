// Aseprite
// Copyright (C) 2022-2024  Igara Studio S.A.
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
#include "app/ui/status_bar.h"
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

using namespace tinyxml2;

static void save_dock_layout(XMLElement* elem, const Dock* dock)
{
  for (const auto* child : dock->children()) {
    const int side = dock->whichSideChildIsDocked(child);
    const gfx::Size size = dock->getUserDefinedSizeAtSide(side);

    std::string sideStr;
    switch (side) {
      case ui::LEFT:   sideStr = "left"; break;
      case ui::RIGHT:  sideStr = "right"; break;
      case ui::TOP:    sideStr = "top"; break;
      case ui::BOTTOM: sideStr = "bottom"; break;
      case ui::CENTER:
      default:
        // Empty side attribute
        break;
    }

    XMLElement* childElem = elem->InsertNewChildElement("");

    if (const auto* subdock = dynamic_cast<const Dock*>(child)) {
      childElem->SetValue("dock");
      if (!sideStr.empty())
        childElem->SetAttribute("side", sideStr.c_str());

      save_dock_layout(childElem, subdock);
    }
    else {
      // Set the widget ID as the element name, e.g. <timeline />,
      // <colorbar />, etc.
      childElem->SetValue(child->id().c_str());
      if (!sideStr.empty())
        childElem->SetAttribute("side", sideStr.c_str());
      if (size.w)
        childElem->SetAttribute("width", size.w);
      if (size.h)
        childElem->SetAttribute("height", size.h);
    }
  }
}

static void load_dock_layout(const XMLElement* elem, Dock* dock)
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
  if (const auto* sideStr = elem->Attribute("side")) {
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
  else if (elemName == "statusbar") {
    widget = win->statusBar();
  }
  else if (elemName == "dock") {
    subdock = dock->subdock(side);
  }

  if (subdock) {
    const auto* childElem = elem->FirstChildElement();
    while (childElem) {
      load_dock_layout(childElem, subdock);
      childElem = childElem->NextSiblingElement();
    }
  }
  else {
    dock->dock(side, widget, size);
  }
}

// static
LayoutPtr Layout::MakeFromXmlElement(const XMLElement* layoutElem)
{
  const char* name = layoutElem->Attribute("name");
  const char* id = layoutElem->Attribute("id");

  if (id == nullptr || name == nullptr) {
    LOG(WARNING, "Invalid XML layout provided\n");
    return nullptr;
  }

  auto layout = std::make_shared<Layout>();
  layout->m_id = id;
  layout->m_name = name;
  layout->m_elem = layoutElem->DeepClone(&layout->m_dummyDoc)->ToElement();

  ASSERT(!layout->m_name.empty() && !layout->m_id.empty());

  if (layout->m_elem->ChildElementCount() == 0) // TODO: More error checking here.
    return nullptr;

  if (layout->m_name.empty() || layout->m_id.empty())
    return nullptr;

  return layout;
}

// static
LayoutPtr Layout::MakeFromDock(const std::string& id, const std::string& name, const Dock* dock)
{
  auto layout = std::make_shared<Layout>();
  layout->m_id = id;
  layout->m_name = name;

  layout->m_elem = layout->m_dummyDoc.NewElement("layout");
  layout->m_elem->SetAttribute("id", id.c_str());
  layout->m_elem->SetAttribute("name", name.c_str());
  save_dock_layout(layout->m_elem, dock);

  return layout;
}

bool Layout::matchId(const std::string_view id) const
{
  if (m_id == id)
    return true;

  if ((m_id.empty() && id == kDefault) || (m_id == kDefault && id.empty()))
    return true;

  return false;
}

bool Layout::loadLayout(Dock* dock) const
{
  if (!m_elem)
    return false;

  XMLElement* elem = m_elem->FirstChildElement();
  while (elem) {
    load_dock_layout(elem, dock);
    elem = elem->NextSiblingElement();
  }

  return true;
}

bool Layout::isValidName(const std::string_view name)
{
  if (name.empty())
    return false;
  if (name[0] == '_')
    return false;
  if (name.length() > 128)
    return false;
  return true;
}

} // namespace app
