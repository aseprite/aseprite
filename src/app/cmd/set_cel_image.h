// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_CEL_IMAGE_H_INCLUDED
#define APP_CMD_SET_CEL_IMAGE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_cel.h"
#include "doc/image_ref.h"

#include <sstream>

namespace app { namespace cmd {

class SetCelImage : public Cmd,
                    public WithCel {
public:
  SetCelImage(doc::Cel* cel, const doc::ImageRef& newImage);

protected:
  void onExecute() override;
  void onUndo() override;
  void onRedo() override;
  size_t onMemSize() const override { return sizeof(*this); }

private:
  doc::ObjectId m_oldImageId;
  doc::ObjectId m_newImageId;

  // Reference used only to keep the copy of the new image from the
  // SetCelImage() ctor until the SetCelImage::onExecute() call.  Then
  // the reference is not used anymore.
  doc::ImageRef m_newImage;
  doc::ImageRef m_copy;
};

}} // namespace app::cmd

#endif
