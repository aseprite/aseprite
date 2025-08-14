// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_cel_image.h"

#include "doc/cel.h"
#include "doc/sprite.h"

namespace app { namespace cmd {

using namespace doc;

SetCelImage::SetCelImage(Cel* cel, const ImageRef& newImage)
  : WithCel(cel)
  , m_oldImageId(cel->imageId())
  , m_newImageId(newImage ? newImage->id() : NullId)
  , m_newImage(newImage)
{
}

void SetCelImage::onExecute()
{
  Cel* cel = this->cel();
  Sprite* sprite = cel->sprite();

  // Save old image in m_copy. We cannot keep an ImageRef to this
  // image, because there are other undo branches that could try to
  // modify/re-add this same image ID
  if (m_oldImageId) {
    ImageRef oldImage = sprite->getImageRef(m_oldImageId);
    ASSERT(oldImage);
    m_copy.reset(Image::createCopy(oldImage.get()));
  }

  cel->data()->setImage(m_newImage, cel->layer());
  cel->data()->incrementVersion();

  m_newImage.reset();
}

void SetCelImage::onUndo()
{
  Cel* cel = this->cel();
  Sprite* sprite = cel->sprite();

  ImageRef newImage;
  if (m_newImageId) {
    newImage = sprite->getImageRef(m_newImageId);
    ASSERT(newImage);
  }
  if (m_oldImageId) {
    ASSERT(!sprite->getImageRef(m_oldImageId));
    m_copy->setId(m_oldImageId);
  }

  cel->data()->setImage(m_copy, cel->layer());
  m_copy.reset(newImage ? Image::createCopy(newImage.get()) : nullptr);
}

void SetCelImage::onRedo()
{
  Cel* cel = this->cel();
  Sprite* sprite = cel->sprite();

  ImageRef oldImage;
  if (m_oldImageId)
    oldImage = sprite->getImageRef(m_oldImageId);

  if (m_newImageId) {
    ASSERT(!sprite->getImageRef(m_newImageId));
    m_copy->setId(m_newImageId);
    cel->data()->setImage(m_copy, cel->layer());
  }
  else {
    cel->data()->setImage(nullptr, nullptr);
  }
  cel->data()->incrementVersion();

  m_copy.reset(oldImage ? Image::createCopy(oldImage.get()) : nullptr);
}

}} // namespace app::cmd
