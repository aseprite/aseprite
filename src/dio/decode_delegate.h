// Aseprite Document IO Library
// Copyright (c) 2023 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_DECODE_DELEGATE_H_INCLUDED
#define DIO_DECODE_DELEGATE_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "doc/frame.h"
#include "doc/sprite.h"

#include <string>

namespace dio {

class DecodeDelegate {
public:
  virtual ~DecodeDelegate() { }

  // Used to log errors
  virtual void error(const std::string& msg) { }

  // Sets an error when an incompatibility issue is
  // detected.
  virtual void incompatibilityError(const std::string &msg) { }

  // Used to report progress of the whole operation
  virtual void progress(double fromZeroToOne) { }

  // Return true if the operation was cancelled by the user
  virtual bool isCanceled() { return false; }

  // Return true if you want to read just the first frame (e.g. useful
  // to generate a thumbnail)
  virtual bool decodeOneFrame() { return false; }

  // Default color for slices without user data
  virtual doc::color_t defaultSliceColor() {
    return doc::rgba(0, 0, 255, 255);
  }

  // Called when the sprite is decoded successfully
  virtual void onSprite(doc::Sprite* sprite) {
    // Discard the sprite, you should overwrite this behavior, use the
    // sprite and then discard it when you don't need it anymore.
    delete sprite;
  }

  // Returns true if we want to cache the read compressed data of
  // tilesets exactly as they are in the disk (so we can save it
  // without re-compressing).
  virtual bool cacheCompressedTilesets() const {
    return false;
  }
};

} // namespace dio

#endif
