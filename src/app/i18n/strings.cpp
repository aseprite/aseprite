// Aseprite
// Copyright (C) 2016, 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/i18n/strings.h"

#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "base/fs.h"
#include "cfg/cfg.h"

namespace app {

// static
Strings* Strings::instance()
{
  static Strings* singleton = nullptr;
  if (!singleton)
    singleton = new Strings();
  return singleton;
}

Strings::Strings()
{
  const std::string lang = Preferences::instance().general.language();
  LOG("I18N: Loading strings/%s.ini file\n", lang.c_str());

  ResourceFinder rf;
  rf.includeDataDir(("strings/" + lang + ".ini").c_str());
  if (!rf.findFirst())
    throw base::Exception("strings/" + lang + ".txt was not found");

  cfg::CfgFile cfg;
  cfg.load(rf.filename());

  std::vector<std::string> sections;
  std::vector<std::string> keys;
  cfg.getAllSections(sections);
  for (const auto& section : sections) {
    keys.clear();
    cfg.getAllKeys(section.c_str(), keys);

    std::string textId = section;
    textId.push_back('.');
    for (auto key : keys) {
      textId.append(key);
      m_strings[textId] = cfg.getValue(section.c_str(), key.c_str(), "");

      //TRACE("I18N: Reading string %s -> %s\n", textId.c_str(), m_strings[textId].c_str());

      textId.erase(section.size()+1);
    }
  }
}

const std::string& Strings::translate(const char* id) const
{
  auto it = m_strings.find(id);
  if (it != m_strings.end())
    return it->second;
  else
    return m_strings[id] = id;
}

} // namespace app
