// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_GUI_XML_INCLUDED
#define APP_GUI_XML_INCLUDED
#pragma once

#include "app/xml_document.h"

#include <memory>
#include <string>

namespace app {

// Singleton class to load and access "gui.xml" file.
class GuiXml {
public:
  // Returns the GuiXml singleton. If it was not created yet, the
  // gui.xml file will be loaded by the first time, which could
  // generated an exception if there are errors in the XML file.
  static GuiXml* instance();
  static void destroyInstance();

  GuiXml();

  // Returns the tinyxml document instance.
  tinyxml2::XMLDocument* doc() { return m_doc.get(); }

  // Returns the name of the gui.xml file.
  const char* filename() { return m_doc->Value(); }

private:
  XMLDocumentRef m_doc;
  friend class std::unique_ptr<GuiXml>;
};

} // namespace app

#endif
