// Aseprite Steam Wrapper
// Copyright (c) 2020 Igara Studio S.A.
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef STEAM_STEAM_H_INCLUDED
#define STEAM_STEAM_H_INCLUDED
#pragma once

namespace steam {

class SteamAPI {
public:
  static SteamAPI* instance();

  SteamAPI();
  ~SteamAPI();

  bool initialized() const;
  void runCallbacks();

  bool writeScreenshot(void* rgbBuffer,
                       uint32_t sizeInBytes,
                       int width, int height);

private:
  class Impl;
  Impl* m_impl;
};

} // namespace steam

#endif
