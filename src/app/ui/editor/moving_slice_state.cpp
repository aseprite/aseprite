// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/moving_slice_state.h"

#include "app/cmd/set_slice_key.h"
#include "app/cmd/clear_slices.h"
#include "app/context_access.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/expand_cel_canvas.h"
#include "doc/algorithm/rotate.h"
#include "doc/blend_internals.h"
#include "doc/slice.h"
#include "ui/message.h"
#include "render/render.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace app {

using namespace ui;

MovingSliceState::MovingSliceState(Editor* editor,
                                   MouseMessage* msg,
                                   const EditorHit& hit,
                                   const doc::SelectedObjects& selectedSlices)
  : m_frame(editor->frame())
  , m_hit(hit)
  , m_items(std::max<std::size_t>(1, selectedSlices.size()))
  , m_tx(Tx::DontLockDoc, UIContext::instance(),
         UIContext::instance()->activeDocument(),
         (editor->slicesTransforms() ? "Slices Transformation" : "Slice Movement"))
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
  // Prevent using different tilemap and tileset modes when the last selected
  // layer is not a tilemap.
  if (!m_site.layer()->isTilemap()) {
    m_site.tilemapMode(TilemapMode::Pixels);
    m_site.tilesetMode(TilesetMode::Auto);
  }

  if (editor->slicesTransforms() && !m_items.empty()) {
    DocRange range = m_site.range();
    SelectedLayers selectedLayers = range.selectedLayers();
    // Do not take into account invisible layers.
    for (auto it = selectedLayers.begin(); it != selectedLayers.end(); ++it) {
      if (!(*it)->isVisible()) {
        range.eraseAndAdjust(*it);
      }
    }

    m_selectedLayers = range.selectedLayers().toAllLayersList();
    if (m_selectedLayers.empty() && m_site.layer()->isVisible()) {
      m_selectedLayers.push_back(m_site.layer());
    }
  }

  editor->captureMouse();
}

void MovingSliceState::initializeItemsContent() {
  for (auto& item : m_items) {
    // Align slice origin to tiles origin under Tiles mode.
    if (m_site.tilemapMode() == TilemapMode::Tiles) {
      auto origin = m_site.grid().tileToCanvas(m_site.grid().canvasToTile(item.newKey.bounds().origin()));
      auto bounds = gfx::Rect(origin, item.newKey.bounds().size());
      item.newKey.setBounds(bounds);
    }
    // Reserve one ItemContent slot for each selected layer.
    item.content.reserve(m_selectedLayers.size());

    for (const auto* layer : m_selectedLayers) {
      Mask mask;
      ImageRef image = ImageRef();

      mask.add(item.newKey.bounds());
      if (layer &&
          layer->isTilemap() &&
          m_site.tilemapMode() == TilemapMode::Tiles) {
        image.reset(new_tilemap_from_mask(m_site, &mask));
      }
      else {
        image.reset(new_image_from_mask(
          *layer,
          m_frame,
          &mask,
          Preferences::instance().experimental.newBlend()));
      }

      item.pushContent(image);
    }
  }
}

void MovingSliceState::onEnterState(Editor* editor)
{
  if (editor->slicesTransforms() && !m_items.empty()) {
    initializeItemsContent();

    // Clear brush preview, as the extra cel will be replaced with the
    // transformed image.
    editor->brushPreview().hide();

    clearSlices();

    drawExtraCel();

    // Redraw the editor.
    editor->invalidate();
  }
}

bool MovingSliceState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  {
    ContextWriter writer(UIContext::instance(), 1000);
    CmdTransaction* cmds = m_tx;
    for (const auto& item : m_items) {
      item.slice->insert(m_frame, item.oldKey);
      cmds->addAndExecute(writer.context(),
                          new cmd::SetSliceKey(item.slice, m_frame, item.newKey));

      if (editor->slicesTransforms()) {
          for (int i=0; i<m_selectedLayers.size(); ++i) {
            auto* layer = m_selectedLayers[i];
            m_site.layer(layer);
            m_site.frame(m_frame);
            drawExtraCel(i);
            stampExtraCelImage();
          }
          m_site.document()->setExtraCel(ExtraCelRef(nullptr));
      }
    }

    m_tx.commit();
  }

  editor->backToPreviousState();
  editor->releaseMouse();
  editor->invalidate();
  return true;
}

void MovingSliceState::stampExtraCelImage()
{
  const Image* image = m_extraCel->image();
  if (!image)
    return;

  const Cel* cel = m_extraCel->cel();

  ExpandCelCanvas expand(
    m_site, m_site.layer(),
    TiledMode::NONE, m_tx,
    ExpandCelCanvas::None);

  gfx::Point dstPt;
  gfx::Size canvasImageSize = image->size();
  if (m_site.tilemapMode() == TilemapMode::Tiles) {
    doc::Grid grid = m_site.grid();
    dstPt = grid.canvasToTile(cel->position());
    canvasImageSize = grid.tileToCanvas(gfx::Rect(dstPt, canvasImageSize)).size();
  }
  else {
    dstPt =  cel->position() - expand.getCel()->position();
  }

  expand.validateDestCanvas(
    gfx::Region(gfx::Rect(cel->position(), canvasImageSize)));

  expand.getDestCanvas()->copy(image, gfx::Clip(dstPt, image->bounds()));

  expand.commit();
}

void MovingSliceState::drawExtraCel(int layerIdx)
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
  else
    m_extraCel.reset();

  m_site.document()->setExtraCel(m_extraCel);

  if (m_extraCel->image()) {
    Image* dst = m_extraCel->image();
    if (m_site.tilemapMode() == TilemapMode::Tiles) {
      dst->setMaskColor(doc::notile);
      dst->clear(dst->maskColor());

      if (m_site.cel()) {
        doc::Grid grid = m_site.grid();
        dst->copy(m_site.cel()->image(),
                  gfx::Clip(0, 0, grid.canvasToTile(bounds)));
      }
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
      drawItem(dst, item, bounds.origin(), layerIdx);
    }
  }
}

void MovingSliceState::drawItem(doc::Image* dst,
                                const Item& item,
                                const gfx::Point& itemsBoundsOrigin,
                                int layerIdx)
{
  const ItemContentRef content = (layerIdx >= 0 ? item.content[layerIdx]
                                                : item.mergedContent);

  content->forEachPart(
    [this, dst, itemsBoundsOrigin]
    (const doc::Image* src, const doc::Mask* mask, const gfx::Rect& bounds) {
      drawImage(dst, src, mask, gfx::Rect(bounds).offset(-itemsBoundsOrigin));
    });
}

void MovingSliceState::drawImage(doc::Image* dst,
                                 const doc::Image* src,
                                 const doc::Mask* mask,
                                 const gfx::Rect& bounds)
{
  ASSERT(dst);

  if (!src) return;

  if (m_site.tilemapMode() == TilemapMode::Tiles) {
    gfx::Rect tilesBounds = m_site.grid().canvasToTile(bounds);
    doc::algorithm::parallelogram(
      dst, src, nullptr,
      tilesBounds.x         , tilesBounds.y,
      tilesBounds.x+tilesBounds.w, tilesBounds.y,
      tilesBounds.x+tilesBounds.w, tilesBounds.y+tilesBounds.h,
      tilesBounds.x         , tilesBounds.y+tilesBounds.h
    );
  }
  else {
    doc::algorithm::parallelogram(
      dst, src, (mask ? mask->bitmap() : nullptr),
      bounds.x         , bounds.y,
      bounds.x+bounds.w, bounds.y,
      bounds.x+bounds.w, bounds.y+bounds.h,
      bounds.x         , bounds.y+bounds.h
    );
  }
}

bool MovingSliceState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  gfx::Point newCursorPos = editor->screenToEditor(msg->position());
  gfx::Point delta = newCursorPos - m_mouseStart;
  gfx::Rect totalBounds = selectedSlicesBounds();

  // Move by tile size under Tiles mode.
  if (editor->slicesTransforms() && m_site.tilemapMode() == TilemapMode::Tiles) {
    delta = m_site.grid().tileToCanvas(m_site.grid().canvasToTile(delta));
  }

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

    // Align slice origin to tiles origin under Tiles mode.
    if (editor->slicesTransforms() && m_site.tilemapMode() == TilemapMode::Tiles) {
      rc.setOrigin(m_site.grid().tileToCanvas(m_site.grid().canvasToTile(rc.origin())));
    }

    if (m_hit.type() == EditorHit::SliceCenter)
      key.setCenter(rc);
    else {
      key.setBounds(rc);
      if (item.isNineSlice()) {
        key.setBorder(item.border());
      }
    }

    // Update the slice key
    item.slice->insert(m_frame, key);
  }

  if (editor->slicesTransforms())
    drawExtraCel();

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

  const auto* keyPtr = slice->getByFrame(m_frame);
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

void MovingSliceState::clearSlices()
{
  ContextWriter writer(UIContext::instance(), 1000);
  if (writer.cel()) {
    std::vector<SliceKey> slicesKeys;
    slicesKeys.reserve(m_items.size());
    for (auto& item : m_items) {
      slicesKeys.push_back(item.newKey);
    }

    CmdTransaction* cmds = m_tx;
    cmds->executeAndAdd(new cmd::ClearSlices(m_site, m_selectedLayers, m_frame, slicesKeys));
  }
}

} // namespace app
