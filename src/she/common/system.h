// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_COMMON_SYSTEM_H
#define SHE_COMMON_SYSTEM_H
#pragma once

#ifdef _WIN32
  #include "she/win/clipboard.h"
  #include "she/win/native_dialogs.h"
#elif defined(__APPLE__)
  #include "she/osx/clipboard.h"
  #include "she/osx/native_dialogs.h"
#elif defined(ASEPRITE_WITH_GTK_FILE_DIALOG_SUPPORT) && defined(__linux__)
  #include "she/clipboard_simple.h"
  #include "she/gtk/native_dialogs.h"
#else
  #include "she/clipboard_simple.h"
  #include "she/native_dialogs.h"
#endif

#include "she/common/freetype_font.h"
#include "she/common/sprite_sheet_font.h"

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

  Clipboard* createClipboard() override {
#ifdef _WIN32
    return new ClipboardWin32();
#elif defined(__APPLE__)
    return new ClipboardOSX();
#else
    return new ClipboardImpl();
#endif
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

private:
  NativeDialogs* m_nativeDialogs;
};

} // namespace she

#endif
