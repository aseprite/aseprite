// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

// Uncomment this in case you want to debug range ops
//#define TRACE_RANGE_OPS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/doc_range_ops.h"

#include "app/app.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/doc_range.h"
#include "app/transaction.h"
#include "app/tx.h"
#include "doc/layer.h"
#include "doc/sprite.h"

#include <stdexcept>

#ifdef TRACE_RANGE_OPS
#include <iostream>
#endif

namespace app {

enum Op { Move, Copy };

static void move_or_copy_cels(
  DocApi& api, Op op,
  const LayerList& srcLayers,
  const LayerList& dstLayers,
  const SelectedFrames& srcFrames,
  const SelectedFrames& dstFrames)
{
  ASSERT(srcLayers.size() == dstLayers.size());

  for (layer_t i=0; i<srcLayers.size(); ++i) {
    auto srcFrame = srcFrames.begin();
    auto dstFrame = dstFrames.begin();
    auto srcFrameEnd = srcFrames.end();
    auto dstFrameEnd = dstFrames.end();

    for (; srcFrame != srcFrameEnd &&
           dstFrame != dstFrameEnd; ++srcFrame, ++dstFrame) {
      if (i >= 0 && i < srcLayers.size() && srcLayers[i]->isImage()) {
        LayerImage* srcLayer = static_cast<LayerImage*>(srcLayers[i]);

        if (i < dstLayers.size() && dstLayers[i]->isImage()) {
          LayerImage* dstLayer = static_cast<LayerImage*>(dstLayers[i]);

#ifdef TRACE_RANGE_OPS
          std::clog << (op == Move ? "Moving": "Copying")
                    << " cel " << srcLayer->name() << "[" << *srcFrame << "]"
                    << " into " << dstLayer->name() << "[" << *dstFrame << "]\n";
#endif

          switch (op) {
            case Move: api.moveCel(srcLayer, *srcFrame, dstLayer, *dstFrame); break;
            case Copy: api.copyCel(srcLayer, *srcFrame, dstLayer, *dstFrame); break;
          }
        }
        // All cels moved from a image layer and dropped in other kind
        // of layer (e.g. a group) will be discarded/deleted.
        else if (op == Move) {
          api.clearCel(srcLayer, *srcFrame);
        }
      }
    }
  }
}

static DocRange move_or_copy_frames(
  DocApi& api, Op op,
  Sprite* sprite,
  const DocRange& srcRange,
  frame_t dstFrame,
  const DocRangePlace place,
  const TagsHandling tagsHandling)
{
  const SelectedFrames& srcFrames = srcRange.selectedFrames();

#ifdef TRACE_RANGE_OPS
  std::clog << "move_or_copy_frames frames[";
  for (auto srcFrame : srcFrames) {
    std::clog << srcFrame << ", ";
  }
  std::clog << "] "
            << (place == kDocRangeBefore ? "before":
                place == kDocRangeAfter ? "after":
                                          "as first child")
            << " " << dstFrame << "\n";
#endif

  auto srcFrame = srcFrames.begin();
  auto srcFrameEnd = srcFrames.end();
  frame_t srcDelta = 0;
  frame_t firstCopiedBlock = 0;
  frame_t dstBeforeFrame =
    (place == kDocRangeBefore ? dstFrame:
                                dstFrame+1);

  for (; srcFrame != srcFrameEnd; ++srcFrame) {
    frame_t fromFrame = (*srcFrame)+srcDelta;

    switch (op) {

      case Move:
        if ((*srcFrame) >= dstBeforeFrame) {
          srcDelta = 0;
          fromFrame = *srcFrame;
        }
        break;

      case Copy:
        if (fromFrame >= dstBeforeFrame-1 && firstCopiedBlock) {
          srcDelta += firstCopiedBlock;
          fromFrame += firstCopiedBlock;
          firstCopiedBlock = 0;
        }
        break;
    }

#ifdef TRACE_RANGE_OPS
    std::clog << " [";
    for (frame_t i=0; i<=sprite->lastFrame(); ++i) {
      std::clog << (sprite->frameDuration(i)-1);
    }
    std::clog << "] => "
              << (op == Move ? "Move": "Copy")
              << " " << (*srcFrame) << "+" << (srcDelta)
              << (place == kDocRangeBefore ? " before ": " after ")
              << dstFrame << " => ";
#endif

    switch (op) {

      case Move:
        api.moveFrame(sprite, fromFrame, dstFrame,
                      (place == kDocRangeBefore ? kDropBeforeFrame:
                                                  kDropAfterFrame),
                      tagsHandling);

        if (fromFrame < dstBeforeFrame-1) {
          --srcDelta;
        }
        else if (fromFrame > dstBeforeFrame-1) {
          ++dstBeforeFrame;
          ++dstFrame;
        }
        break;

      case Copy:
        api.copyFrame(sprite, fromFrame, dstFrame,
                      (place == kDocRangeBefore ? kDropBeforeFrame:
                                                  kDropAfterFrame),
                      tagsHandling);

        if (fromFrame < dstBeforeFrame-1) {
          ++firstCopiedBlock;
        }
        else if (fromFrame >= dstBeforeFrame-1) {
          ++srcDelta;
        }
        ++dstBeforeFrame;
        ++dstFrame;
        break;
    }

#ifdef TRACE_RANGE_OPS
    std::clog << " [";
    for (frame_t i=0; i<=sprite->lastFrame(); ++i) {
      std::clog << (sprite->frameDuration(i)-1);
    }
    std::clog << "]\n";
#endif
  }

  DocRange result;
  if (!srcRange.selectedLayers().empty())
    result.selectLayers(srcRange.selectedLayers());
  result.startRange(nullptr, dstBeforeFrame-srcFrames.size(), DocRange::kFrames);
  result.endRange(nullptr, dstBeforeFrame-1);
  return result;
}

static bool has_child(LayerGroup* parent, Layer* child)
{
  for (auto c : parent->layers()) {
    if (c == child)
      return true;
    else if (c->isGroup() &&
             has_child(static_cast<LayerGroup*>(c), child))
      return true;
  }
  return false;
}

static DocRange drop_range_op(
  Doc* doc,
  const Op op,
  const DocRange& from,
  DocRangePlace place,
  const TagsHandling tagsHandling,
  DocRange to)
{
  // Convert "first child" operation into a insert after last child.
  LayerGroup* parent = nullptr;
  if (to.type() == DocRange::kLayers &&
      !to.selectedLayers().empty()) {
    if (place == kDocRangeFirstChild &&
        (*to.selectedLayers().begin())->isGroup()) {
      place = kDocRangeAfter;
      parent = static_cast<LayerGroup*>((*to.selectedLayers().begin()));

      to.clearRange();
      to.startRange(parent->lastLayer(), -1, DocRange::kLayers);
      to.endRange(parent->lastLayer(), -1);
    }
    else {
      parent = (*to.selectedLayers().begin())->parent();
    }

    // Check that we're not moving a group inside itself
    for (auto moveThis : from.selectedLayers()) {
      if (moveThis == parent ||
          (moveThis->isGroup() &&
           has_child(static_cast<LayerGroup*>(moveThis), parent)))
        return from;
    }
  }

  if (place != kDocRangeBefore &&
      place != kDocRangeAfter) {
    ASSERT(false);
    throw std::invalid_argument("Invalid 'place' argument");
  }

  Sprite* sprite = doc->sprite();

  // Check noop/trivial/do nothing cases, i.e., move a range to the same place.
  // Also check invalid cases, like moving a Background layer.
  switch (from.type()) {

    case DocRange::kCels:
      if (from == to)
        return from;
      break;

    case DocRange::kFrames:
      if (op == Move) {
        // Simple cases with one continuos range of frames that are a
        // no-op.
        if ((from.selectedFrames().ranges() == 1) &&
            ((to.firstFrame() >= from.firstFrame() &&
              to.lastFrame() <= from.lastFrame()) ||
             (place == kDocRangeBefore && to.firstFrame() == from.lastFrame()+1) ||
             (place == kDocRangeAfter && to.lastFrame() == from.firstFrame()-1)) &&
            // If there are tags, this might not be a no-op
            (sprite->tags().empty() ||
             tagsHandling == kDontAdjustTags)) {
          return from;
        }
      }
      break;

    case DocRange::kLayers:
      if (op == Move) {
        SelectedLayers srcSelLayers = from.selectedLayers();
        SelectedLayers dstSelLayers = to.selectedLayers();
        LayerList srcLayers = srcSelLayers.toLayerList();
        LayerList dstLayers = dstSelLayers.toLayerList();
        ASSERT(!srcLayers.empty());
        if (srcLayers.empty())
          return from;

        // dstLayers can be nullptr when we insert the first child in
        // a group.

        // Check no-ops when we move layers at the same level (all
        // layers with the same parent), all adjacents, and which are
        // moved to the same place.
        if (!dstSelLayers.empty() &&
            srcSelLayers.hasSameParent() &&
            dstSelLayers.hasSameParent() &&
            are_layers_adjacent(srcLayers) &&
            are_layers_adjacent(dstLayers)) {
          for (Layer* srcLayer : srcLayers)
            if (dstSelLayers.contains(srcLayer))
              return from;

          if ((place == kDocRangeBefore
               && dstLayers.front() == srcLayers.back()->getNext()) ||
              (place == kDocRangeAfter
               && dstLayers.back() == srcLayers.front()->getPrevious()))
            return from;
        }

        // We cannot move the background
        for (Layer* layer : srcSelLayers)
          if (layer->isBackground())
            throw std::runtime_error("The background layer cannot be moved");
      }

      // Before background
      if (place == kDocRangeBefore) {
        for (Layer* background : to.selectedLayers()) {
          if (background && background->isBackground())
            throw std::runtime_error("You cannot move or copy something below the background layer");
        }
      }
      break;
  }

  const char* undoLabel = NULL;
  switch (op) {
    case Move: undoLabel = "Move Range"; break;
    case Copy: undoLabel = "Copy Range"; break;
    default:
      ASSERT(false);
      throw std::invalid_argument("Invalid 'op' argument");
  }
  DocRange resultRange;

  {
    const app::Context* context = static_cast<app::Context*>(doc->context());
    const ContextReader reader(context);
    ContextWriter writer(reader);
    Tx tx(writer.context(), undoLabel, ModifyDocument);
    DocApi api = doc->getApi(tx);

    // TODO Try to add the range with just one call to DocApi
    // methods, to avoid generating a lot of cmd::SetCelFrame (see
    // DocApi::setCelFramePosition() function).

    switch (from.type()) {

      case DocRange::kCels: {
        LayerList allLayers = sprite->allBrowsableLayers();
        if (allLayers.empty())
          break;

        LayerList srcLayers = from.selectedLayers().toLayerList();
        LayerList dstLayers = to.selectedLayers().toLayerList();
        if (srcLayers.empty() ||
            dstLayers.empty())
          throw std::invalid_argument("You need to specify a non-empty cels range");

        if (find_layer_index(allLayers, srcLayers.front()) <
            find_layer_index(allLayers, dstLayers.front())) {
          std::reverse(srcLayers.begin(), srcLayers.end());
          std::reverse(dstLayers.begin(), dstLayers.end());
        }

        if (from.firstFrame() < to.firstFrame()) {
          auto srcFrames = from.selectedFrames().makeReverse();
          auto dstFrames = to.selectedFrames().makeReverse();

          move_or_copy_cels(api, op, srcLayers, dstLayers, srcFrames, dstFrames);
        }
        else {
          const auto& srcFrames = from.selectedFrames();
          const auto& dstFrames = to.selectedFrames();

          move_or_copy_cels(api, op, srcLayers, dstLayers, srcFrames, dstFrames);
        }

        resultRange = to;
        break;
      }

      case DocRange::kFrames: {
        frame_t dstFrame;
        if (place == kDocRangeBefore)
          dstFrame = to.firstFrame();
        else
          dstFrame = to.lastFrame();

        resultRange = move_or_copy_frames(api, op, sprite,
                                          from, dstFrame,
                                          place, tagsHandling);
        break;
      }

      case DocRange::kLayers: {
        LayerList allLayers = sprite->allBrowsableLayers();
        if (allLayers.empty())
          break;

        LayerList srcLayers = from.selectedLayers().toLayerList();
        LayerList dstLayers = to.selectedLayers().toLayerList();
        ASSERT(!srcLayers.empty());

        switch (op) {

          case Move:
            if (place == kDocRangeBefore) {
              Layer* beforeThis = (!dstLayers.empty() ? dstLayers.front(): nullptr);
              Layer* afterThis  = nullptr;

              for (Layer* srcLayer : srcLayers) {
                if (afterThis)
                  api.restackLayerAfter(srcLayer, parent, afterThis);
                else
                  api.restackLayerBefore(srcLayer, parent, beforeThis);

                afterThis = srcLayer;
              }
            }
            else if (place == kDocRangeAfter) {
              Layer* afterThis = (!dstLayers.empty() ? dstLayers.back(): nullptr);
              for (Layer* srcLayer : srcLayers) {
                api.restackLayerAfter(srcLayer, parent, afterThis);
                afterThis = srcLayer;
              }
            }

            // Same set of layers than the "from" range
            resultRange = from;
            break;

          case Copy: {
            if (place == kDocRangeBefore) {
              Layer* beforeThis = (!dstLayers.empty() ? dstLayers.front(): nullptr);
              for (Layer* srcLayer :  srcLayers) {
                Layer* copiedLayer = api.duplicateLayerBefore(
                  srcLayer, parent, beforeThis);

                resultRange.startRange(copiedLayer, -1, DocRange::kLayers);
                resultRange.endRange(copiedLayer, -1);
              }
            }
            else if (place == kDocRangeAfter) {
              std::reverse(srcLayers.begin(), srcLayers.end());

              Layer* afterThis = (!dstLayers.empty() ? dstLayers.back(): nullptr);
              for (Layer* srcLayer :  srcLayers) {
                Layer* copiedLayer = api.duplicateLayerAfter(
                  srcLayer, parent, afterThis);

                resultRange.startRange(copiedLayer, -1, DocRange::kLayers);
                resultRange.endRange(copiedLayer, -1);
              }
            }
            break;
          }
        }
        break;
      }
    }

    if (resultRange.type() != DocRange::kNone)
      tx.setNewDocRange(resultRange);

    tx.commit();
  }

  return resultRange;
}

DocRange move_range(Doc* doc,
                    const DocRange& from,
                    const DocRange& to,
                    const DocRangePlace place,
                    const TagsHandling tagsHandling)
{
  return drop_range_op(doc, Move, from, place,
                       tagsHandling, DocRange(to));
}

DocRange copy_range(Doc* doc,
                    const DocRange& from,
                    const DocRange& to,
                    const DocRangePlace place,
                    const TagsHandling tagsHandling)
{
  return drop_range_op(doc, Copy, from, place,
                       tagsHandling, DocRange(to));
}

void reverse_frames(Doc* doc, const DocRange& range)
{
  const app::Context* context = static_cast<app::Context*>(doc->context());
  const ContextReader reader(context);
  ContextWriter writer(reader);
  Tx tx(writer.context(), "Reverse Frames");
  DocApi api = doc->getApi(tx);
  Sprite* sprite = doc->sprite();
  LayerList layers;
  frame_t frameBegin, frameEnd;
  bool moveFrames = false;
  bool swapCels = false;

  switch (range.type()) {
    case DocRange::kCels:
      frameBegin = range.firstFrame();
      frameEnd = range.lastFrame();
      layers = range.selectedLayers().toLayerList();
      swapCels = true;
      break;
    case DocRange::kFrames:
      frameBegin = range.firstFrame();
      frameEnd = range.lastFrame();
      layers = sprite->allLayers();
      moveFrames = true;
      break;
    case DocRange::kLayers:
      frameBegin = frame_t(0);
      frameEnd = sprite->totalFrames()-1;
      layers = range.selectedLayers().toLayerList();
      swapCels = true;
      break;
  }

  if (moveFrames) {
    for (frame_t frameRev = frameEnd+1;
         frameRev > frameBegin;
         --frameRev) {
      api.moveFrame(sprite, frameBegin, frameRev,
                    kDropBeforeFrame,
                    kDontAdjustTags);
    }
  }
  else if (swapCels) {
    for (Layer* layer : layers) {
      if (!layer->isImage())
        continue;

      for (frame_t frame = frameBegin,
             frameRev = frameEnd;
           frame != (frameBegin+frameEnd)/2+1;
           ++frame, --frameRev) {
        if (frame == frameRev)
          continue;

        LayerImage* imageLayer = static_cast<LayerImage*>(layer);
        api.swapCel(imageLayer, frame, frameRev);
      }
    }
  }

  tx.setNewDocRange(range);
  tx.commit();
}

} // namespace app
