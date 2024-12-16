// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_I18N_XML_TRANSLATOR_INCLUDED
#define APP_I18N_XML_TRANSLATOR_INCLUDED
#pragma once

#include <string>

namespace tinyxml2 {
class XMLElement;
}

namespace app {

class XmlTranslator {
public:
  std::string operator()(const tinyxml2::XMLElement* elem, const char* attrName);

  void clearStringIdPrefix();
  void setStringIdPrefix(const char* prefix);

private:
  std::string m_stringIdPrefix;
};

} // namespace app

#endif
