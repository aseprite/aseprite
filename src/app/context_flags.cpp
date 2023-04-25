// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/context_flags.h"

#include "app/context.h"
#include "app/doc.h"
#include "app/site.h"
#include "app/ui/editor/editor.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

ContextFlags::ContextFlags()
{
  m_flags = 0;
}

void ContextFlags::update(Context* context)
{
  Site site = context->activeSite();
  Doc* document = site.document();

  m_flags = 0;

  if (document) {
    m_flags |= HasActiveDocument;

    if (document->readLock(0)) {
      m_flags |= ActiveDocumentIsReadable;

      if (document->isMaskVisible())
        m_flags |= HasVisibleMask;

      updateFlagsFromSite(site);

      if (document->canWriteLockFromRead() && !document->isReadOnly())
        m_flags |= ActiveDocumentIsWritable;

      document->unlock();
    }

#ifdef ENABLE_UI
    // TODO this is a hack, try to find a better design to handle this
    // "moving pixels" state.
    auto editor = Editor::activeEditor();
    if (editor &&
        editor->document() == document &&
        editor->isMovingPixels()) {
      // Flags enabled when we are in MovingPixelsState
      m_flags |=
        HasVisibleMask |
        ActiveDocumentIsReadable |
        ActiveDocumentIsWritable;

      updateFlagsFromSite(editor->getSite());
    }
#endif // ENABLE_UI
  }
}

void ContextFlags::updateFlagsFromSite(const Site& site)
{
  const Sprite* sprite = site.sprite();
  if (!sprite)
    return;

  m_flags |= HasActiveSprite;

  if (sprite->backgroundLayer())
    m_flags |= HasBackgroundLayer;

  const Layer* layer = site.layer();
  frame_t frame = site.frame();
  if (!layer)
    return;

  m_flags |= HasActiveLayer;

  if (layer->isBackground())
    m_flags |= ActiveLayerIsBackground;

  if (layer->isVisibleHierarchy())
    m_flags |= ActiveLayerIsVisible;

  if (layer->isEditableHierarchy())
    m_flags |= ActiveLayerIsEditable;

  if (layer->isReference())
    m_flags |= ActiveLayerIsReference;

  if (layer->isTilemap())
    m_flags |= ActiveLayerIsTilemap;

  if (layer->isImage()) {
    m_flags |= ActiveLayerIsImage;

    Cel* cel = layer->cel(frame);
    if (cel) {
      m_flags |= HasActiveCel;

      if (cel->image())
        m_flags |= HasActiveImage;
    }
  }

  if (site.selectedColors().picks() > 0)
    m_flags |= HasSelectedColors;

  if (site.selectedTiles().picks() > 0)
    m_flags |= HasSelectedTiles;
}

} // namespace app
