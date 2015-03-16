// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef _WIN32
  #include "she/win/clipboard.h"
  #include "she/win/native_dialogs.h"
#else
  #include "she/clipboard_simple.h"
#endif

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
#endif
    return m_nativeDialogs;
  }

  Clipboard* createClipboard() override {
#ifdef _WIN32
    return new ClipboardWin32();
#else
    return new ClipboardImpl();
#endif
  }

private:
  NativeDialogs* m_nativeDialogs;
};

} // namespace she
