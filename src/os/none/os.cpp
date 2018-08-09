// LAF OS Library
// Copyright (C) 2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/memory.h"
#include "os/system.h"

namespace os {

class NoneSystem : public System {
public:
  void dispose() override { delete this; }
  void activateApp() override { }
  void finishLaunching() override { }
  Capabilities capabilities() const override { return (Capabilities)0; }
  void useWintabAPI(bool enable) override { }
  Logger* logger() override { return nullptr; }
  Menus* menus() override { return nullptr; }
  NativeDialogs* nativeDialogs() override { return nullptr; }
  EventQueue* eventQueue() override { return nullptr; }
  bool gpuAcceleration() const override { return false; }
  void setGpuAcceleration(bool state) override { }
  gfx::Size defaultNewDisplaySize() override { return gfx::Size(0, 0); }
  Display* defaultDisplay() override { return nullptr; }
  Display* createDisplay(int width, int height, int scale) override { return nullptr; }
  Surface* createSurface(int width, int height) override { return nullptr; }
  Surface* createRgbaSurface(int width, int height) override { return nullptr; }
  Surface* loadSurface(const char* filename) override { return nullptr; }
  Surface* loadRgbaSurface(const char* filename) override { return nullptr; }
  Font* loadSpriteSheetFont(const char* filename, int scale) override { return nullptr; }
  Font* loadTrueTypeFont(const char* filename, int height) override { return nullptr; }
  bool isKeyPressed(KeyScancode scancode) override { return false; }
  KeyModifiers keyModifiers() override { return kKeyNoneModifier; }
  int getUnicodeFromScancode(KeyScancode scancode) override { return 0; }
  void setTranslateDeadKeys(bool state) override { }
};

System* create_system_impl() {
  return new NoneSystem;
}

void error_message(const char* msg)
{
  fputs(msg, stderr);
  // TODO
}

} // namespace os

extern int app_main(int argc, char* argv[]);

#if _WIN32
int wmain(int argc, wchar_t* wargv[], wchar_t* envp[]) {
  char** argv;
  if (wargv && argc > 0) {
    argv = new char*[argc];
    for (int i=0; i<argc; ++i)
      argv[i] = base_strdup(base::to_utf8(std::wstring(wargv[i])).c_str());
  }
  else {
    argv = new char*[1];
    argv[0] = base_strdup("");
    argc = 1;
  }
#else
int main(int argc, char* argv[]) {
#endif
  return app_main(argc, argv);
}
