// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_COMMON_SYSTEM_H
#define SHE_COMMON_SYSTEM_H
#pragma once

#ifdef _WIN32
  #include "she/win/native_dialogs.h"
#elif defined(__APPLE__)
  #include "she/osx/native_dialogs.h"
#elif defined(ASEPRITE_WITH_GTK_FILE_DIALOG_SUPPORT) && defined(__linux__)
  #include "she/gtk/native_dialogs.h"
#else
  #include "she/native_dialogs.h"
#endif

#include "she/common/freetype_font.h"
#include "she/common/sprite_sheet_font.h"
#include "she/system.h"

namespace she {

#ifdef __APPLE__
Logger* getOsxLogger();
#endif

class CommonSystem : public System {
public:
  CommonSystem()
    : m_nativeDialogs(nullptr) {
  }

  ~CommonSystem() {
    delete m_nativeDialogs;
  }

  void dispose() override {
    delete this;
  }

  Logger* logger() override {
#ifdef __APPLE__
    return getOsxLogger();
#else
    return nullptr;
#endif
  }

  NativeDialogs* nativeDialogs() override {
#ifdef _WIN32
    if (!m_nativeDialogs)
      m_nativeDialogs = new NativeDialogsWin32();
#elif defined(__APPLE__)
    if (!m_nativeDialogs)
      m_nativeDialogs = new NativeDialogsOSX();
#elif defined(ASEPRITE_WITH_GTK_FILE_DIALOG_SUPPORT) && defined(__linux__)
    if (!m_nativeDialogs)
      m_nativeDialogs = new NativeDialogsGTK3();
#endif
    return m_nativeDialogs;
  }

  Font* loadSpriteSheetFont(const char* filename, int scale) override {
    Surface* sheet = loadRgbaSurface(filename);
    Font* font = nullptr;
    if (sheet) {
      sheet->applyScale(scale);
      font = SpriteSheetFont::fromSurface(sheet);
    }
    return font;
  }

  Font* loadTrueTypeFont(const char* filename, int height) override {
    return loadFreeTypeFont(filename, height);
  }

  KeyModifiers keyModifiers() override {
    return
      (KeyModifiers)
      ((isKeyPressed(kKeyLShift) ||
        isKeyPressed(kKeyRShift) ? kKeyShiftModifier: 0) |
       (isKeyPressed(kKeyLControl) ||
        isKeyPressed(kKeyRControl) ? kKeyCtrlModifier: 0) |
       (isKeyPressed(kKeyAlt) ? kKeyAltModifier: 0) |
       (isKeyPressed(kKeyAltGr) ? (kKeyCtrlModifier | kKeyAltModifier): 0) |
       (isKeyPressed(kKeyCommand) ? kKeyCmdModifier: 0) |
       (isKeyPressed(kKeySpace) ? kKeySpaceModifier: 0) |
       (isKeyPressed(kKeyLWin) ||
        isKeyPressed(kKeyRWin) ? kKeyWinModifier: 0));
  }

  void clearKeyboardBuffer() override {
    // Do nothing
  }

private:
  NativeDialogs* m_nativeDialogs;
};

} // namespace she

#endif
