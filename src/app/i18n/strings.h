// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_I18N_STRINGS_INCLUDED
#define APP_I18N_STRINGS_INCLUDED
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace app {

  // Singleton class to load and access "strings/english.txt" file.
  class Strings {
  public:
    static Strings* instance();

    const std::string& translate(const char* id);

  private:
    Strings();

    std::unordered_map<std::string, std::string> m_strings;
  };

  #define tr(id) (Strings::instance()->translate(id))

} // namespace app

#endif
