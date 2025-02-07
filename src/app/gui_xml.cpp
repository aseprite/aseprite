// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/gui_xml.h"

#include "app/resource_finder.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "base/fs.h"

namespace app {

static std::unique_ptr<GuiXml> g_singleton;

// static
GuiXml* GuiXml::instance()
{
  if (!g_singleton)
    g_singleton = std::make_unique<GuiXml>();
  return g_singleton.get();
}

// static
void GuiXml::destroyInstance()
{
  g_singleton.reset();
}

GuiXml::GuiXml()
{
  LOG("GUIXML: Loading gui.xml file\n");

  ResourceFinder rf;
  rf.includeDataDir("gui.xml");
  if (!rf.findFirst())
    throw base::Exception("gui.xml was not found");

  // Load the XML file. As we've already checked "path" existence,
  // in a case of exception we should show the error and stop.
  m_doc = app::open_xml(rf.filename());
}

} // namespace app
