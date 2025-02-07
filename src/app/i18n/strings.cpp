// Aseprite
// Copyright (C) 2023-2024  Igara Studio S.A.
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
#include "base/debug.h"
#include "base/fs.h"
#include "cfg/cfg.h"

#include <algorithm>

namespace app {

static Strings* singleton = nullptr;

const char* Strings::kDefLanguage = "en";

// static
void Strings::createInstance(Preferences& pref, Extensions& exts)
{
  ASSERT(!singleton);
  singleton = new Strings(pref, exts);
}

// static
Strings* Strings::instance()
{
  return singleton;
}

Strings::Strings(Preferences& pref, Extensions& exts) : m_pref(pref), m_exts(exts)
{
  loadLanguage(currentLanguage());
}

std::set<LangInfo> Strings::availableLanguages() const
{
  std::set<LangInfo> result;

  // Add languages in data/strings/ + data/strings.git/
  ResourceFinder rf;
  rf.includeDataDir("strings");
  rf.includeDataDir("strings.git");
  while (rf.next()) {
    const std::string stringsPath = rf.filename();
    if (!base::is_directory(stringsPath))
      continue;

    for (const auto& fn : base::list_files(stringsPath, base::ItemType::Files, "*.ini")) {
      const std::string langId = base::get_file_title(fn);
      std::string path = base::join_path(stringsPath, fn);
      std::string displayName = langId;

      // Load display name
      cfg::CfgFile cfg;
      if (cfg.load(path))
        displayName = cfg.getValue("_", "display_name", displayName.c_str());

      result.insert(LangInfo(langId, path, displayName));
    }
  }

  // Add languages in extensions
  for (const auto& ext : m_exts) {
    if (ext->isEnabled() && ext->hasLanguages()) {
      for (const auto& lang : ext->languages())
        result.insert(lang.second);
    }
  }

  // Check that the default language exists.
  ASSERT(std::find_if(result.begin(), result.end(), [](const LangInfo& li) {
           return li.id == kDefLanguage;
         }) != result.end());

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

void Strings::logError(const char* id, const char* error) const
{
  LOG(ERROR,
      "I18N: Error in \"%s.ini\" file, string id \"%s\" is \"%s\", threw \"%s\" error\n",
      currentLanguage().c_str(),
      id,
      translate(id).c_str(),
      error);
}

// static
std::string Strings::VFormat(const char* id, const fmt::format_args& vargs)
{
  Strings* s = Strings::instance();

  // First we try to translate the ID string with the current
  // translation. If it fails (e.g. a fmt::format_error) it can be
  // because an ill-formed string for the fmt library.
  //
  // In that case then we try to use the regular English string from
  // the en.ini (which can fail too if the user modified the en.ini
  // file by hand).
  try {
    return fmt::vformat(s->translate(id), vargs);
  }
  catch (const std::runtime_error& e) {
    s->logError(id, e.what());
    // Continue with default translation (English)...
  }

  try {
    return fmt::vformat(s->defaultString(id), vargs);
  }
  catch (const std::runtime_error& e) {
    // This is unexpected, invalid en.ini file
    ASSERT(false);
    s->logError(id, e.what());
    return id;
  }
}

void Strings::loadLanguage(const std::string& langId)
{
  m_strings.clear();
  loadStringsFromDataDir(kDefLanguage);
  m_default = m_strings;

  if (langId != kDefLanguage) {
    loadStringsFromDataDir(langId);
    loadStringsFromExtension(langId);
  }
}

void Strings::loadStringsFromDataDir(const std::string& langId)
{
  // Load the English language file from the Aseprite data directory (so we have the most update
  // list of strings)
  LOG("I18N: Loading %s.ini file\n", langId.c_str());
  ResourceFinder rf;
  rf.includeDataDir(base::join_path("strings", langId + ".ini").c_str());
  rf.includeDataDir(base::join_path("strings.git", langId + ".ini").c_str());
  if (!rf.findFirst()) {
    LOG("I18N: %s.ini was not found\n", langId.c_str());
    return;
  }
  LOG("I18N: %s found\n", rf.filename().c_str());
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
    std::string value;
    textId.push_back('.');
    for (auto key : keys) {
      textId.append(key);

      value = cfg.getValue(section.c_str(), key.c_str(), "");

      // Process escaped chars (\\, \n, \s, \t, etc.)
      for (int i = 0; i < int(value.size());) {
        if (value[i] == '\\') {
          value.erase(i, 1);
          if (i == int(value.size()))
            break;
          int chr = value[i];
          switch (chr) {
            case '\\': chr = '\\'; break;
            case 'n':  chr = '\n'; break;
            case 't':  chr = '\t'; break;
            case 's':  chr = ' '; break;
          }
          value[i] = chr;
        }
        else {
          ++i;
        }
      }
      m_strings[textId] = value;

      // TRACE("I18N: Reading string %s -> %s\n", textId.c_str(), m_strings[textId].c_str());

      textId.erase(section.size() + 1);
    }
  }
}

const std::string& Strings::translate(const char* id) const
{
  auto it = m_strings.find(id);
  if (it != m_strings.end())
    return it->second;
  return m_strings[id] = id;
}

const std::string& Strings::defaultString(const char* id) const
{
  auto it = m_default.find(id);
  if (it != m_default.end())
    return it->second;
  return m_default[id] = id;
}

} // namespace app
