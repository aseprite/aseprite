// Aseprite
// Copyright (C) 2023-2024  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_I18N_STRINGS_INCLUDED
#define APP_I18N_STRINGS_INCLUDED
#pragma once

#include "app/i18n/lang_info.h"
#include "fmt/core.h"
#include "obs/signal.h"
#include "strings.ini.h"

#include <set>
#include <string>
#include <unordered_map>

namespace app {

class Preferences;
class Extensions;

// Singleton class to load and access "strings/en.ini" file.
class Strings : public app::gen::Strings<app::Strings> {
public:
  static const char* kDefLanguage;

  static void createInstance(Preferences& pref, Extensions& exts);
  static Strings* instance();

  const std::string& translate(const char* id) const;
  const std::string& defaultString(const char* id) const;

  std::set<LangInfo> availableLanguages() const;
  std::string currentLanguage() const;
  void setCurrentLanguage(const std::string& langId);

  void logError(const char* id, const char* error) const;

  static const std::string& Translate(const char* id)
  {
    Strings* s = Strings::instance();
    return s->translate(id);
  }

  // Formats a string with the given arguments, if it fails
  // (e.g. because the translation contains an invalid formatted
  // string) it tries to return the original string from the default
  // en.ini file.
  template<typename... Args>
  static std::string Format(const char* id, Args&&... args)
  {
    return VFormat(id, fmt::make_format_args(args...));
  }

  static std::string VFormat(const char* id, const fmt::format_args& vargs);

  obs::signal<void()> LanguageChange;

private:
  Strings(Preferences& pref, Extensions& exts);

  void loadLanguage(const std::string& langId);
  void loadStringsFromDataDir(const std::string& langId);
  void loadStringsFromExtension(const std::string& langId);
  void loadStringsFromFile(const std::string& fn);

  Preferences& m_pref;
  Extensions& m_exts;
  mutable std::unordered_map<std::string, std::string> m_default; // Default strings from en.ini
  mutable std::unordered_map<std::string, std::string> m_strings; // Strings from current language
};

} // namespace app

#endif
