// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
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
#include "app/util/cel_ops.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "render/rasterize.h"
#include "render/render.h"

namespace app { namespace cmd {

using namespace doc;

CopyCel::CopyCel(Layer* srcLayer,
                 frame_t srcFrame,
                 Layer* dstLayer,
                 frame_t dstFrame,
                 bool continuous)
  : m_srcLayer(srcLayer)
  , m_dstLayer(dstLayer)
  , m_srcFrame(srcFrame)
  , m_dstFrame(dstFrame)
  , m_continuous(continuous)
{
}

void CopyCel::onExecute()
{
  Layer* srcLayer = m_srcLayer.layer();
  Layer* dstLayer = m_dstLayer.layer();

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

  Image* srcImage = (srcCel ? srcCel->image() : NULL);
  ImageRef dstImage;
  dstCel = dstLayer->cel(m_dstFrame);
  if (dstCel)
    dstImage = dstCel->imageRef();

  bool createLink = (srcLayer == dstLayer && m_continuous);

  // For background layer
  if (dstLayer->isBackground()) {
    ASSERT(dstCel);
    ASSERT(dstImage);
    if (!dstCel || !dstImage || !srcCel || !srcImage)
      return;

    ASSERT(!dstLayer->isTilemap()); // TODO support background tilemaps

    if (createLink) {
      executeAndAdd(new cmd::SetCelData(dstCel, srcCel->dataRef()));
    }
    // Rasterize tilemap into the regular image background layer
    else if (srcLayer->isTilemap()) {
      ImageRef tmp(Image::createCopy(dstImage.get()));
      render::rasterize(tmp.get(), srcCel, 0, 0, false);
      executeAndAdd(new cmd::CopyRect(dstImage.get(), tmp.get(), gfx::Clip(tmp->bounds())));
    }
    else {
      BlendMode blend = (srcLayer->isBackground() ? BlendMode::SRC : BlendMode::NORMAL);

      ImageRef tmp(Image::createCopy(dstImage.get()));
      render::composite_image(tmp.get(),
                              srcImage,
                              srcSprite->palette(m_srcFrame),
                              srcCel->x(),
                              srcCel->y(),
                              255,
                              blend);
      executeAndAdd(new cmd::CopyRect(dstImage.get(), tmp.get(), gfx::Clip(tmp->bounds())));
    }
  }
  // For transparent layers
  else {
    if (dstCel)
      executeAndAdd(new cmd::RemoveCel(dstCel));

    if (srcCel) {
      if (createLink)
        dstCel = Cel::MakeLink(m_dstFrame, srcCel);
      else
        dstCel = create_cel_copy(this, srcCel, dstSprite, dstLayer, m_dstFrame);

      executeAndAdd(new cmd::AddCel(dstLayer, dstCel));
    }
  }
}

void CopyCel::onFireNotifications()
{
  CmdSequence::onFireNotifications();

  // The m_srcLayer can be nullptr now because the layer from where we
  // copied this cel might not exist anymore (e.g. if we copied the
  // cel from another document that is already closed)
  // ASSERT(m_srcLayer.layer());

  ASSERT(m_dstLayer.layer());

  static_cast<Doc*>(m_dstLayer.layer()->sprite()->document())
    ->notifyCelCopied(m_srcLayer.layer(), m_srcFrame, m_dstLayer.layer(), m_dstFrame);
}

}} // namespace app::cmd
