// Aseprite
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/i18n/strings.h"

#include "app/app.h"
#include "app/extensions.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "app/xml_document.h"
#include "app/xml_exception.h"
#include "base/fs.h"
#include "cfg/cfg.h"

namespace app {

static Strings* singleton = nullptr;
static const char* kDefLanguage = "en";

// static
void Strings::createInstance(Preferences& pref,
                             Extensions& exts)
{
  ASSERT(!singleton);
  singleton = new Strings(pref, exts);
}

// static
Strings* Strings::instance()
{
  return singleton;
}

Strings::Strings(Preferences& pref,
                 Extensions& exts)
  : m_pref(pref)
  , m_exts(exts)
{
  loadLanguage(currentLanguage());
}

std::set<std::string> Strings::availableLanguages() const
{
  std::set<std::string> result;

  // Add languages in data/strings/
  ResourceFinder rf;
  rf.includeDataDir("strings");
  while (rf.next()) {
    if (!base::is_directory(rf.filename()))
      continue;

    for (const auto& fn : base::list_files(rf.filename())) {
      const std::string langId = base::get_file_title(fn);
      result.insert(langId);
    }
  }

  // Add languages in extensions
  for (const auto& ext : m_exts) {
    if (ext->isEnabled() &&
        ext->hasLanguages()) {
      for (const auto& langId : ext->languages())
        result.insert(langId.first);
    }
  }

  ASSERT(result.find(kDefLanguage) != result.end());
  return result;
}

std::string Strings::currentLanguage() const
{
  return m_pref.general.language();
}

void Strings::setCurrentLanguage(const std::string& langId)
{
  // Do nothing (same language)
  if (currentLanguage() == langId)
    return;

  m_pref.general.language(langId);
  loadLanguage(langId);

  LanguageChange();
}

void Strings::loadLanguage(const std::string& langId)
{
  m_strings.clear();
  loadStringsFromDataDir(kDefLanguage);
  if (langId != kDefLanguage) {
    loadStringsFromDataDir(langId);
    loadStringsFromExtension(langId);
  }
}

void Strings::loadStringsFromDataDir(const std::string& langId)
{
  // Load the English language file from the Aseprite data directory (so we have the most update list of strings)
  LOG("I18N: Loading strings/%s.ini file\n", langId.c_str());
  ResourceFinder rf;
  rf.includeDataDir(("strings/" + langId + ".ini").c_str());
  if (!rf.findFirst()) {
    LOG("strings/%s.ini was not found", langId.c_str());
    return;
  }

  loadStringsFromFile(rf.filename());
}

void Strings::loadStringsFromExtension(const std::string& langId)
{
  std::string fn = m_exts.languagePath(langId);
  if (!fn.empty() && base::is_file(fn))
    loadStringsFromFile(fn);
}

void Strings::loadStringsFromFile(const std::string& fn)
{
  cfg::CfgFile cfg;
  cfg.load(fn);

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
