// Aseprite Document IO Library
// Copyright (c) 2026 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_ENCODE_DELEGATE_H_INCLUDED
#define DIO_ENCODE_DELEGATE_H_INCLUDED
#pragma once

#include "doc/frame.h"
#include "doc/frames_sequence.h"

#include <string>

namespace doc {
class Sprite;
}

namespace dio {

class EncodeDelegate {
public:
  virtual ~EncodeDelegate() {}

  // Used to log errors
  virtual void error(const std::string& msg) {}

  // Used to report progress of the whole operation
  virtual void progress(double fromZeroToOne) {}

  // Return true if the operation was cancelled by the user
  virtual bool isCanceled() { return false; }

  virtual doc::Sprite* sprite() = 0;
  virtual const doc::FramesSequence& framesSequence() const = 0;

  virtual bool composeGroups() = 0;
  virtual bool preserveColorProfile() = 0;
  virtual bool cacheCompressedTilesets() = 0;
  virtual int preferredCelType() { return ASE_FILE_COMPRESSED_CEL; }
  virtual int preferredTilemapCelType() { return ASE_FILE_COMPRESSED_TILEMAP; }

  doc::frame_t fromFrame() const { return framesSequence().firstFrame(); }
  doc::frame_t toFrame() const { return framesSequence().lastFrame(); }
  doc::frame_t frames() const { return (doc::frame_t)framesSequence().size(); }
};

} // namespace dio

#endif
