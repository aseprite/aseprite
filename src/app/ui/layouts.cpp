// Aseprite
// Copyright (c) 2022-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/layouts.h"

#include "app/resource_finder.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "base/fs.h"

#include <algorithm>
#include <fstream>

namespace app {

using namespace tinyxml2;

Layouts::Layouts()
{
  try {
    std::string fn = m_userLayoutsFilename = UserLayoutsFilename();
    if (base::is_file(fn))
      load(fn);
  }
  catch (const std::exception& ex) {
    LOG(ERROR, "LAY: Error loading user layouts: %s\n", ex.what());
  }
}

Layouts::~Layouts()
{
  try {
    saveUserLayouts();
  }
  catch (const std::exception& ex) {
    LOG(ERROR, "LAY: Error saving user layouts on exit: %s\n", ex.what());
  }
}

LayoutPtr Layouts::getById(const std::string& id) const
{
  auto it = std::find_if(m_layouts.begin(), m_layouts.end(), [&id](const LayoutPtr& l) {
    return l->matchId(id);
  });
  return (it != m_layouts.end() ? *it : nullptr);
}

bool Layouts::addLayout(const LayoutPtr& layout)
{
  ASSERT(layout);

  const auto it = std::find_if(m_layouts.begin(), m_layouts.end(), [layout](const LayoutPtr& l) {
    return l->matchId(layout->id());
  });

  if (it != m_layouts.end()) {
    *it = layout; // Replace existent layout
    return false;
  }

  m_layouts.push_back(layout);
  return true;
}

void Layouts::removeLayout(const LayoutPtr& layout)
{
  if (m_layouts.size() <= 1) {
    m_layouts.clear();
    return;
  }

  ASSERT(layout);

  const auto it = std::find_if(m_layouts.begin(), m_layouts.end(), [layout](const LayoutPtr& l) {
    return l->matchId(layout->id());
  });

  m_layouts.erase(it);
}

void Layouts::saveUserLayouts()
{
  if (m_userLayoutsFilename.empty())
    return;

  save(m_userLayoutsFilename);

  // TODO: We probably have too much I/O here, but it's the easiest way to keep the XML and
  // internal representations synced up.
  reload();
}

void Layouts::reload()
{
  if (m_userLayoutsFilename.empty())
    return;

  m_layouts.clear();
  load(m_userLayoutsFilename);
}

void Layouts::load(const std::string& fn)
{
  const XMLDocumentRef doc = app::open_xml(fn);
  XMLHandle handle(doc.get());
  XMLElement* layoutElem =
    handle.FirstChildElement("layouts").FirstChildElement("layout").ToElement();

  while (layoutElem) {
    if (auto layout = Layout::MakeFromXmlElement(layoutElem)) {
      m_layouts.push_back(layout);
    }
    layoutElem = layoutElem->NextSiblingElement();
  }
}

void Layouts::save(const std::string& fn) const
{
  auto doc = std::make_unique<XMLDocument>();
  XMLElement* layoutsElem = doc->NewElement("layouts");

  for (const auto& layout : m_layouts) {
    layoutsElem->InsertEndChild(layout->xmlElement()->DeepClone(doc.get()));
  }

  doc->InsertEndChild(doc->NewDeclaration(R"(xml version="1.0" encoding="utf-8")"));
  doc->InsertEndChild(layoutsElem);
  save_xml(doc.get(), fn);
}

// static
std::string Layouts::UserLayoutsFilename()
{
  ResourceFinder rf;
  rf.includeUserDir("user.aseprite-layouts");
  return rf.getFirstOrCreateDefault();
}

} // namespace app
