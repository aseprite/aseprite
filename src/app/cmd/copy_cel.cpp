// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/copy_cel.h"

#include "app/cmd/add_cel.h"
#include "app/cmd/add_frame.h"
#include "app/cmd/clear_cel.h"
#include "app/cmd/copy_rect.h"
#include "app/cmd/remove_cel.h"
#include "app/cmd/set_cel_data.h"
#include "app/cmd/unlink_cel.h"
#include "app/doc.h"
#include "app/util/create_cel_copy.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "render/render.h"

namespace app {
namespace cmd {

using namespace doc;

CopyCel::CopyCel(
  LayerImage* srcLayer, frame_t srcFrame,
  LayerImage* dstLayer, frame_t dstFrame, bool continuous)
  : m_srcLayer(srcLayer)
  , m_dstLayer(dstLayer)
  , m_srcFrame(srcFrame)
  , m_dstFrame(dstFrame)
  , m_continuous(continuous)
{
}

void CopyCel::onExecute()
{
  LayerImage* srcLayer = static_cast<LayerImage*>(m_srcLayer.layer());
  LayerImage* dstLayer = static_cast<LayerImage*>(m_dstLayer.layer());

  ASSERT(srcLayer);
  ASSERT(dstLayer);

  Sprite* srcSprite = srcLayer->sprite();
  Sprite* dstSprite = dstLayer->sprite();
  ASSERT(srcSprite);
  ASSERT(dstSprite);
  ASSERT(m_srcFrame >= 0 && m_srcFrame < srcSprite->totalFrames());
  ASSERT(m_dstFrame >= 0);

  Cel* srcCel = srcLayer->cel(m_srcFrame);
  Cel* dstCel = dstLayer->cel(m_dstFrame);

  // Clear destination cel if it does exist. It'll be overriden by the
  // copy of srcCel.
  if (dstCel) {
    if (dstCel->links())
      executeAndAdd(new cmd::UnlinkCel(dstCel));
    executeAndAdd(new cmd::ClearCel(dstCel));
  }

  // Add empty frames until newFrame
  while (dstSprite->totalFrames() <= m_dstFrame)
    executeAndAdd(new cmd::AddFrame(dstSprite, dstSprite->totalFrames()));

  Image* srcImage = (srcCel ? srcCel->image(): NULL);
  ImageRef dstImage;
  dstCel = dstLayer->cel(m_dstFrame);
  if (dstCel)
    dstImage = dstCel->imageRef();

  bool createLink =
    (srcLayer == dstLayer && m_continuous);

  // For background layer
  if (dstLayer->isBackground()) {
    ASSERT(dstCel);
    ASSERT(dstImage);
    if (!dstCel || !dstImage ||
        !srcCel || !srcImage)
      return;

    if (createLink) {
      executeAndAdd(new cmd::SetCelData(dstCel, srcCel->dataRef()));
    }
    else {
      BlendMode blend = (srcLayer->isBackground() ?
                         BlendMode::SRC:
                         BlendMode::NORMAL);

      ImageRef tmp(Image::createCopy(dstImage.get()));
      render::composite_image(
        tmp.get(), srcImage,
        srcSprite->palette(m_srcFrame),
        srcCel->x(), srcCel->y(), 255, blend);
      executeAndAdd(new cmd::CopyRect(dstImage.get(), tmp.get(), gfx::Clip(tmp->bounds())));
    }
  }
  // For transparent layers
  else {
    if (dstCel)
      executeAndAdd(new cmd::RemoveCel(dstCel));

    if (srcCel) {
      if (createLink) {
        dstCel = Cel::createLink(srcCel);
        dstCel->setFrame(m_dstFrame);
      }
      else
        dstCel = create_cel_copy(srcCel, dstSprite, dstLayer, m_dstFrame);

      executeAndAdd(new cmd::AddCel(dstLayer, dstCel));
    }
  }
}

void CopyCel::onFireNotifications()
{
  CmdSequence::onFireNotifications();

  ASSERT(m_srcLayer.layer());
  ASSERT(m_dstLayer.layer());

  static_cast<Doc*>(m_dstLayer.layer()->sprite()->document())
    ->notifyCelCopied(
      m_srcLayer.layer(), m_srcFrame,
      m_dstLayer.layer(), m_dstFrame);
}

} // namespace cmd
} // namespace app
