// Aseprite UI Library
// Copyright (C) 2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_TRANSLATION_DELEGATE_H_INCLUDED
#define UI_TRANSLATION_DELEGATE_H_INCLUDED
#pragma once

#include <string>

namespace ui {

// Translates strings displayed in the ui-lib widgets.
class TranslationDelegate {
public:
  virtual ~TranslationDelegate() {}

  // ui::Entry popup
  virtual std::string copy() { return "&Copy"; }
  virtual std::string cut() { return "Cu&t"; }
  virtual std::string paste() { return "&Paste"; }
  virtual std::string selectAll() { return "Select &All"; }
};

} // namespace ui

#endif
