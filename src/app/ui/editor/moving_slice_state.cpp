// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_slice_key.h"
#include "app/context_access.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/moving_slice_state.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/new_image_from_mask.h"
#include "doc/blend_internals.h"
#include "doc/slice.h"
#include "doc/mask.h"
#include "doc/algorithm/rotate.h"
#include "gfx/point.h"
#include "ui/message.h"
#include "render/render.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace app {

using namespace ui;

MovingSliceState::MovingSliceState(Editor* editor,
                                   MouseMessage* msg,
                                   const EditorHit& hit,
                                   const doc::SelectedObjects& selectedSlices)
  : m_frame(editor->frame())
  , m_hit(hit)
  , m_items(std::max<std::size_t>(1, selectedSlices.size()))
{
  m_mouseStart = editor->screenToEditor(msg->position());

  if (selectedSlices.empty()) {
    m_items[0] = getItemForSlice(m_hit.slice());
  }
  else {
    int i = 0;
    for (Slice* slice : selectedSlices.iterateAs<Slice>()) {
      ASSERT(slice);
      m_items[i++] = getItemForSlice(slice);
    }
  }

  editor->getSite(&m_site);

  editor->captureMouse();
}

void MovingSliceState::onEnterState(Editor* editor)
{
  if (editor->slicesTransforms() && !m_items.empty()) {
    for (auto& item : m_items) {
      item.mask->add(item.newKey.bounds());

      if (m_site.layer() &&
          m_site.layer()->isTilemap() &&
          m_site.tilemapMode() == TilemapMode::Tiles) {
        item.img.reset(new_tilemap_from_mask(m_site, item.mask.get()));
      }
      else {
        item.img.reset(new_image_from_mask(m_site, item.mask.get(),
                                           Preferences::instance().experimental.newBlend()));
      }

      ASSERT(item.img);
      if (!item.img) {
        // We've received a bug report with this case, we're not sure
        // yet how to reproduce it. Probably new_tilemap_from_mask() can
        // return nullptr (e.g. when site.cel() is nullptr?)
        continue;
      }
    }

    // Clear brush preview, as the extra cel will be replaced with the
    // transformed image.
    editor->brushPreview().hide();
  }
}

bool MovingSliceState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  {
    ContextWriter writer(UIContext::instance(), 1000);
    Tx tx(writer, "Slice Movement", ModifyDocument);

    for (const auto& item : m_items) {
      item.slice->insert(m_frame, item.oldKey);
      tx(new cmd::SetSliceKey(item.slice, m_frame, item.newKey));
    }

    tx.commit();
  }

  editor->backToPreviousState();
  editor->releaseMouse();
  editor->invalidate();
  return true;
}

void MovingSliceState::drawExtraCel(Editor* editor)
{
  int t, opacity = (m_site.layer()->isImage() ?
                    static_cast<LayerImage*>(m_site.layer())->opacity(): 255);
  Cel* cel = m_site.cel();
  if (cel) opacity = MUL_UN8(opacity, cel->opacity(), t);

  if (!m_extraCel)
    m_extraCel.reset(new ExtraCel);

  gfx::Rect bounds;
  for (auto& item : m_items)
    bounds |= item.newKey.bounds();


  if (!bounds.isEmpty()) {
    gfx::Size extraCelSize;
    if (m_site.tilemapMode() == TilemapMode::Tiles) {
      // Transforming tiles
      extraCelSize = m_site.grid().canvasToTile(bounds).size();
    }
    else {
      // Transforming pixels
      extraCelSize = bounds.size();
    }

    m_extraCel->create(
      m_site.tilemapMode(),
      m_site.document()->sprite(),
      bounds,
      extraCelSize,
      m_site.frame(),
      opacity);
    m_extraCel->setType(render::ExtraType::PATCH);
    m_extraCel->setBlendMode(m_site.layer()->isImage() ?
                             static_cast<LayerImage*>(m_site.layer())->blendMode():
                             doc::BlendMode::NORMAL);
  }

  m_site.document()->setExtraCel(m_extraCel);

  if (m_extraCel->image()) {
    Image* dst = m_extraCel->image();
    if (m_site.tilemapMode() == TilemapMode::Tiles) {
      dst->setMaskColor(doc::notile);
      dst->clear(dst->maskColor());
  /*
   TODO: Fix this when the TilemapMode::Pixels mode works
      if (m_site.cel()) {
        doc::Grid grid = m_site.grid();
        dst->copy(m_site.cel()->image(),
                  gfx::Clip(0, 0, grid.canvasToTile(bounds)));
        //dst->copy(item.img.get(),
        //          gfx::Clip(0, 0, grid.canvasToTile(bounds)));
      }
  */
    }
    else {
      dst->setMaskColor(m_site.sprite()->transparentColor());
      dst->clear(dst->maskColor());

      render::Render render;
      render.renderLayer(
        dst, m_site.layer(), m_site.frame(),
        gfx::Clip(0, 0, bounds),
        doc::BlendMode::SRC);

    }

    for (auto& item : m_items) {
      // Draw the transformed pixels in the extra-cel which is the chunk
      // of pixels that the user is moving.
      drawImage(item, dst, gfx::PointF(bounds.origin()));
    }
  }
}


void MovingSliceState::drawImage(const Item& item, doc::Image* dst, const gfx::PointF& pt)
{
  ASSERT(dst);

  if (!item.img.get()) return;

  gfx::Rect bounds = item.newKey.bounds();

  if (m_site.tilemapMode() == TilemapMode::Tiles) {

 /* TODO: Finish this when TilemapMode::Pixels works
    drawTransformedTilemap(
      transformation,
      dst, m_originalImage.get(),
      m_initialMask.get());
    */
  }
  else {
    doc::algorithm::parallelogram(
      dst, item.img.get(), item.mask->bitmap(),
      bounds.x-pt.x         , bounds.y-pt.y,
      bounds.x+bounds.w-pt.x, bounds.y-pt.y,
      bounds.x+bounds.w-pt.x, bounds.y+bounds.h-pt.y,
      bounds.x-pt.x         , bounds.y+bounds.h-pt.y
    );
  }
}

bool MovingSliceState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  gfx::Point newCursorPos = editor->screenToEditor(msg->position());
  gfx::Point delta = newCursorPos - m_mouseStart;
  gfx::Rect totalBounds = selectedSlicesBounds();

  ASSERT(totalBounds.w > 0);
  ASSERT(totalBounds.h > 0);

  for (auto& item : m_items) {
    auto& key = item.newKey;
    key = item.oldKey;

    gfx::Rect rc =
      (m_hit.type() == EditorHit::SliceCenter ? key.center():
                                                key.bounds());

    // Move slice
    if (m_hit.border() == (CENTER | MIDDLE)) {
      rc.x += delta.x;
      rc.y += delta.y;
    }
    // Move/resize 9-slices center
    else if (m_hit.type() == EditorHit::SliceCenter) {
      if (m_hit.border() & LEFT) {
        rc.x += delta.x;
        rc.w -= delta.x;
        if (rc.w < 1) {
          rc.x += rc.w-1;
          rc.w = 1;
        }
      }
      if (m_hit.border() & TOP) {
        rc.y += delta.y;
        rc.h -= delta.y;
        if (rc.h < 1) {
          rc.y += rc.h-1;
          rc.h = 1;
        }
      }
      if (m_hit.border() & RIGHT) {
        rc.w += delta.x;
        if (rc.w < 1)
          rc.w = 1;
      }
      if (m_hit.border() & BOTTOM) {
        rc.h += delta.y;
        if (rc.h < 1)
          rc.h = 1;
      }
    }
    // Move/resize bounds
    else {
      if (m_hit.border() & LEFT) {
        rc.x += delta.x * (totalBounds.x2() - rc.x) / totalBounds.w;
        rc.w -= delta.x * rc.w / totalBounds.w;
        if (rc.w < 1) {
          rc.x += rc.w-1;
          rc.w = 1;
        }
      }
      if (m_hit.border() & TOP) {
        rc.y += delta.y * (totalBounds.y2() - rc.y) / totalBounds.h;
        rc.h -= delta.y * rc.h / totalBounds.h;
        if (rc.h < 1) {
          rc.y += rc.h-1;
          rc.h = 1;
        }
      }
      if (m_hit.border() & RIGHT) {
        rc.x += delta.x * (rc.x - totalBounds.x) / totalBounds.w;
        rc.w += delta.x * rc.w / totalBounds.w;
        if (rc.w < 1)
          rc.w = 1;
      }
      if (m_hit.border() & BOTTOM) {
        rc.y += delta.y * (rc.y - totalBounds.y) / totalBounds.h;
        rc.h += delta.y * rc.h / totalBounds.h;
        if (rc.h < 1)
          rc.h = 1;
      }
    }

    if (m_hit.type() == EditorHit::SliceCenter)
      key.setCenter(rc);
    else
      key.setBounds(rc);

    // Update the slice key
    item.slice->insert(m_frame, key);
  }

  if (editor->slicesTransforms())
    drawExtraCel(editor);

  // Redraw the editor.
  editor->invalidate();

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingSliceState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
  switch (m_hit.border()) {
    case TOP | LEFT:
      editor->showMouseCursor(kSizeNWCursor);
      break;
    case TOP:
      editor->showMouseCursor(kSizeNCursor);
      break;
    case TOP | RIGHT:
      editor->showMouseCursor(kSizeNECursor);
      break;
    case LEFT:
      editor->showMouseCursor(kSizeWCursor);
      break;
    case RIGHT:
      editor->showMouseCursor(kSizeECursor);
      break;
    case BOTTOM | LEFT:
      editor->showMouseCursor(kSizeSWCursor);
      break;
    case BOTTOM:
      editor->showMouseCursor(kSizeSCursor);
      break;
    case BOTTOM | RIGHT:
      editor->showMouseCursor(kSizeSECursor);
      break;
  }
  return true;
}

MovingSliceState::Item MovingSliceState::getItemForSlice(doc::Slice* slice)
{
  Item item;
  item.slice = slice;
  item.mask = std::make_shared<Mask>();

  auto keyPtr = slice->getByFrame(m_frame);
  ASSERT(keyPtr);
  if (keyPtr)
    item.oldKey = item.newKey = *keyPtr;

  return item;
}

gfx::Rect MovingSliceState::selectedSlicesBounds() const
{
  gfx::Rect bounds;
  for (auto& item : m_items)
    bounds |= item.oldKey.bounds();
  return bounds;
}

} // namespace app
