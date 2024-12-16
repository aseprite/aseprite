// Aseprite Steam Wrapper
// Copyright (c) 2020-2024 Igara Studio S.A.
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef STEAM_STEAM_H_INCLUDED
#define STEAM_STEAM_H_INCLUDED
#pragma once

#include <memory>

namespace steam {

class SteamAPI {
public:
  static SteamAPI* instance();

  SteamAPI();
  ~SteamAPI();

  bool isInitialized() const;
  void runCallbacks();

  bool writeScreenshot(void* rgbBuffer, uint32_t sizeInBytes, int width, int height);

private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace steam

#endif
