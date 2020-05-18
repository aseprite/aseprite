// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/i18n/xml_translator.h"

#include "app/i18n/strings.h"
#include "tinyxml.h"

namespace app {

std::string XmlTranslator::operator()(const TiXmlElement* elem,
                                      const char* attrName)
{
  const char* value = elem->Attribute(attrName);
  if (!value)
    return std::string();
  else if (value[0] == '@') {   // Translate string
    if (value[1] == '.')
      return Strings::instance()->translate((m_stringIdPrefix + (value+1)).c_str());
    else
      return Strings::instance()->translate(value+1);
  }
  else if (value[0] == '!')     // Raw string
    return std::string(value+1);
  else
    return std::string(value);
}

void XmlTranslator::clearStringIdPrefix()
{
  m_stringIdPrefix.clear();
}

void XmlTranslator::setStringIdPrefix(const char* prefix)
{
  m_stringIdPrefix = prefix;
}

} // namespace app
