// Aseprite
// Copyright (C) 2025-2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_cel_image.h"

#include "doc/cel.h"

namespace app { namespace cmd {

using namespace doc;

SetCelImage::SetCelImage(Cel* cel, const ImageRef& newImage)
  : WithCel(cel)
  , m_newImage(newImage)
  , m_bounds(cel->bounds())
{
}

void SetCelImage::onExecute()
{
  Cel* cel = this->cel();
  ImageRef oldImage = cel->imageRef();

  cel->data()->setImage(m_newImage, cel->layer());
  cel->data()->incrementVersion();

  if (oldImage)
    m_suspendedImage.suspend(oldImage);

  m_newImage.reset();
}

void SetCelImage::onUndo()
{
  Cel* cel = this->cel();
  ImageRef currentImage = cel->imageRef();
  gfx::Rect currentBounds = cel->bounds();
  ImageRef previousImage;
  if (m_suspendedImage.object())
    previousImage = m_suspendedImage.restore();

  cel->data()->setImage(previousImage, cel->layer());
  cel->data()->setBounds(m_bounds);
  cel->data()->incrementVersion();

  m_bounds = currentBounds;

  if (currentImage)
    m_suspendedImage.suspend(currentImage);
}

void SetCelImage::onRedo()
{
  onUndo();
}

}} // namespace app::cmd
