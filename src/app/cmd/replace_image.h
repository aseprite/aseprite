// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REPLACE_IMAGE_H_INCLUDED
#define APP_CMD_REPLACE_IMAGE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/image_ref.h"

#include <sstream>

namespace app { namespace cmd {
using namespace doc;

class ReplaceImage : public Cmd,
                     public WithSprite {
public:
  ReplaceImage(Sprite* sprite, const ImageRef& oldImage, const ImageRef& newImage);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  size_t onMemSize() const override { return sizeof(*this) + (m_copy ? m_copy->getMemSize() : 0); }

private:
  void replaceImage(ObjectId oldId, const ImageRef& newImage);

  ObjectId m_oldImageId;
  ObjectId m_newImageId;

  // Reference used only to keep the copy of the new image from the
  // ReplaceImage() ctor until the ReplaceImage::onExecute() call.
  // Then the reference is not used anymore.
  ImageRef m_newImage;
  ImageRef m_copy;
};

}} // namespace app::cmd

#endif
