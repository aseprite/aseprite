// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd_sequence.h"
#include "app/cmd_transaction.h"
#include "app/cmd/set_slice_key.h"
#include "app/cmd/clear_slices.h"
#include "app/doc_range.h"
#include "app/context_access.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/moving_slice_state.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/expand_cel_canvas.h"
#include "app/util/new_image_from_mask.h"
#include "doc/algorithm/rotate.h"
#include "doc/blend_internals.h"
#include "doc/color.h"
#include "doc/image_ref.h"
#include "doc/mask.h"
#include "doc/primitives.h"
#include "doc/selected_layers.h"
#include "doc/slice.h"
#include "doc/algorithm/fill_selection.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/region_skia.h"
#include "ui/message.h"
#include "render/render.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
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
  //, m_reader(UIContext::instance())
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

  editor->captureMouse();
}

void MovingSliceState::onEnterState(Editor* editor)
{
  if (editor->slicesTransforms() && !m_items.empty()) {
    for (auto& item : m_items) {
      item.imgs.reserve(m_selectedLayers.size());
      item.masks.reserve(m_selectedLayers.size());
      int i = 0;
      for (const auto* layer : m_selectedLayers) {
        item.masks.push_back(std::make_shared<Mask>());
        item.imgs.push_back(ImageRef());
        item.masks[i]->add(item.newKey.bounds());
        item.masks[i]->freeze();
        if (layer &&
            layer->isTilemap() &&
            m_site.tilemapMode() == TilemapMode::Tiles) {
          //item.img.reset(new_tilemap_from_mask(m_site, item.mask.get()));
        }
        else {
          item.imgs[i].reset(new_image_from_mask(
            *layer,
            m_frame,
            item.masks[i].get(),
            Preferences::instance().experimental.newBlend()));
          // TODO: See if part of the code in fromImage can be replaced
          // refactored to use mask_image (in cel_ops.h)?
          item.masks[i]->fromImage(item.imgs[i].get(), item.masks[i]->origin());
        }

        // If there is just one layer selected, we can use the same image as the
        // mergedImg.
        if (m_selectedLayers.size() == 1) {
          item.mergedImg = item.imgs[0];
          item.mergedMask = item.masks[0];
        }
        else {
          if (i == 0) {
            const gfx::Rect& srcBounds = item.imgs[i]->bounds();
            item.mergedImg.reset(Image::create(layer->sprite()->pixelFormat(), srcBounds.w, srcBounds.h));
            item.mergedImg->clear(layer->sprite()->transparentColor());
            item.mergedMask = std::make_shared<Mask>(*item.masks[i].get());
            item.mergedMask->freeze();
          }
          else {
            item.mergedMask->add(*item.masks[i].get());
          }
          copy_masked_zones(item.mergedImg.get(),
                            item.imgs[i].get(),
                            item.masks[i].get(),
                            item.masks[i]->bounds().x,
                            item.masks[i]->bounds().y);
        }
        i++;
      }
    }

    // Clear brush preview, as the extra cel will be replaced with the
    // transformed image.
    editor->brushPreview().hide();

    clearSlices();

    if (editor->slicesTransforms()) {
      drawSliceContents();

      // Redraw the editor.
      editor->invalidate();
    }
  }
}

EditorState::LeaveAction MovingSliceState::onLeaveState(Editor *editor, EditorState *newState)
{
  //editor->document()->resetTransformation();

  return StandbyState::onLeaveState(editor, newState);
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
            drawSliceContentsByLayer(i);
            stampExtraCelImage();
          }
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

void MovingSliceState::drawSliceContents()
{
  gfx::Rect bounds;
  for (auto& item : m_items)
    bounds |= item.newKey.bounds();

  drawExtraCel(bounds, [this](const gfx::Rect& bounds, Image* dst){
    for (auto& item : m_items) {
      // Draw the transformed pixels in the extra-cel which is the chunk
      // of pixels that the user is moving.
      drawImage(dst,
                item.mergedImg.get(),
                item.mergedMask.get(),
                gfx::Rect(item.newKey.bounds()).offset(-bounds.origin()));
    }
  });
}

void MovingSliceState::drawSliceContentsByLayer(int layerIdx)
{
  gfx::Rect bounds;
  for (auto& item : m_items)
    bounds |= item.newKey.bounds();

  drawExtraCel(bounds, [this, layerIdx](const gfx::Rect& bounds, Image* dst){
    for (auto& item : m_items) {
      drawImage(dst,
                item.imgs[layerIdx].get(),
                item.masks[layerIdx].get(),
                gfx::Rect(item.newKey.bounds()).offset(-bounds.origin()));
    }
  });
}

void MovingSliceState::drawExtraCel(const gfx::Rect& bounds, DrawExtraCelContentFunc drawContent)
{
  int t, opacity = (m_site.layer()->isImage() ?
                    static_cast<LayerImage*>(m_site.layer())->opacity(): 255);
  Cel* cel = m_site.cel();
  if (cel) opacity = MUL_UN8(opacity, cel->opacity(), t);

  if (!m_extraCel)
    m_extraCel.reset(new ExtraCel);

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

    drawContent(bounds, dst);
  }
}

void MovingSliceState::drawImage(doc::Image* dst,
                                 const doc::Image* src,
                                 const doc::Mask* mask,
                                 const gfx::Rect& bounds)
{
  ASSERT(dst);

  if (!src) return;

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
      dst, src, mask->bitmap(),
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
    drawSliceContents();

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
