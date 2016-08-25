// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

// Uncomment this in case you want to debug range ops
//#define TRACE_RANGE_OPS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document_range_ops.h"

#include "app/context_access.h"
#include "app/document_api.h"
#include "app/document_range.h"
#include "app/transaction.h"
#include "doc/layer.h"
#include "doc/sprite.h"

#include <stdexcept>

#ifdef TRACE_RANGE_OPS
#include <iostream>
#endif

namespace app {

enum Op { Move, Copy };

template<typename T>
static void move_or_copy_cels(
  DocumentApi& api, Op op,
  LayerList& srcLayers,
  LayerList& dstLayers,
  T& srcFrames,
  T& dstFrames)
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
          LayerImage* srcLayer = static_cast<LayerImage*>(srcLayers[i]);
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
        else if (op == Move) {
          api.clearCel(srcLayer, *srcFrame);
        }
      }
    }
  }
}

template<typename T>
static DocumentRange move_or_copy_frames(
  DocumentApi& api, Op op,
  Sprite* sprite,
  T& srcFrames,
  frame_t dstFrame)
{
#ifdef TRACE_RANGE_OPS
  std::clog << "move_or_copy_frames frames[";
  for (auto srcFrame : srcFrames) {
    std::clog << srcFrame << ", ";
  }
  std::clog << "] => " << dstFrame << "\n";
#endif

  auto srcFrame = srcFrames.begin();
  auto srcFrameEnd = srcFrames.end();
  frame_t srcDelta = 0;
  frame_t firstCopiedBlock = 0;

  for (; srcFrame != srcFrameEnd; ++srcFrame) {
    frame_t fromFrame = (*srcFrame)+srcDelta;

    switch (op) {

      case Move:
        if ((*srcFrame) >= dstFrame) {
          srcDelta = 0;
          fromFrame = *srcFrame;
        }
        break;

      case Copy:
        if (fromFrame >= dstFrame-1 && firstCopiedBlock) {
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
              << " " << (*srcFrame) << "+" << (srcDelta) << " -> " << dstFrame << " => ";
#endif

    switch (op) {

      case Move:
        api.moveFrame(sprite, fromFrame, dstFrame);

        if (fromFrame < dstFrame-1) {
          --srcDelta;
        }
        else if (fromFrame > dstFrame-1) {
          ++dstFrame;
        }
        break;

      case Copy:
        api.copyFrame(sprite, fromFrame, dstFrame);

        if (fromFrame < dstFrame-1) {
          ++firstCopiedBlock;
        }
        else if (fromFrame >= dstFrame-1) {
          ++srcDelta;
        }
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

  DocumentRange result;
  result.startRange(nullptr, dstFrame-srcFrames.size(), DocumentRange::kFrames);
  result.endRange(nullptr, dstFrame-1);
  return result;
}

static DocumentRange drop_range_op(
  Document* doc, Op op, const DocumentRange& from,
  DocumentRangePlace place, const DocumentRange& to)
{
  if (place != kDocumentRangeBefore &&
      place != kDocumentRangeAfter) {
    ASSERT(false);
    throw std::invalid_argument("Invalid 'place' argument");
  }

  Sprite* sprite = doc->sprite();

  // Check noop/trivial/do nothing cases, i.e., move a range to the same place.
  // Also check invalid cases, like moving a Background layer.
  switch (from.type()) {

    case DocumentRange::kCels:
      if (from == to)
        return from;
      break;

    case DocumentRange::kFrames:
      if (op == Move) {
        // Simple cases with one continuos range of frames that are a
        // no-op.
        if ((from.selectedFrames().ranges() == 1) &&
            ((to.firstFrame() >= from.firstFrame() &&
              to.lastFrame() <= from.lastFrame()) ||
             (place == kDocumentRangeBefore && to.firstFrame() == from.lastFrame()+1) ||
             (place == kDocumentRangeAfter && to.lastFrame() == from.firstFrame()-1)))
          return from;
      }
      break;

    case DocumentRange::kLayers:
      if (op == Move) {
          SelectedLayers srcSelLayers = from.selectedLayers();
          SelectedLayers dstSelLayers = to.selectedLayers();
          LayerList srcLayers = srcSelLayers.toLayerList();
          LayerList dstLayers = dstSelLayers.toLayerList();
          if (srcLayers.empty() || dstLayers.empty())
            return from;

        // Check no-ops when we move layers at the same level (all
        // layers with the same parent), all adjacents, and which are
        // moved to the same place.
        if (srcSelLayers.hasSameParent() &&
            dstSelLayers.hasSameParent() &&
            are_layers_adjacent(srcLayers) &&
            are_layers_adjacent(dstLayers)) {
          for (Layer* srcLayer : srcLayers)
            if (dstSelLayers.contains(srcLayer))
              return from;

          if ((place == kDocumentRangeBefore
               && dstLayers.front() == srcLayers.back()->getNext()) ||
              (place == kDocumentRangeAfter
               && dstLayers.back() == srcLayers.front()->getPrevious()))
            return from;
        }

        // We cannot move the background
        for (Layer* layer : srcSelLayers)
          if (layer->isBackground())
            throw std::runtime_error("The background layer cannot be moved");
      }

      // Before background
      if (place == kDocumentRangeBefore) {
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
  DocumentRange resultRange;

  {
    const app::Context* context = static_cast<app::Context*>(doc->context());
    const ContextReader reader(context);
    ContextWriter writer(reader, 500);
    Transaction transaction(writer.context(), undoLabel, ModifyDocument);
    DocumentApi api = doc->getApi(transaction);

    // TODO Try to add the range with just one call to DocumentApi
    // methods, to avoid generating a lot of cmd::SetCelFrame (see
    // DocumentApi::setCelFramePosition() function).

    switch (from.type()) {

      case DocumentRange::kCels: {
        LayerList allLayers = sprite->allBrowsableLayers();
        if (allLayers.empty())
          break;

        LayerList srcLayers = from.selectedLayers().toLayerList();
        LayerList dstLayers = to.selectedLayers().toLayerList();
        if (srcLayers.empty())
          throw std::invalid_argument("You need to specify a non-empty cels range");

        if (find_layer_index(allLayers, srcLayers.front()) <
            find_layer_index(allLayers, dstLayers.front())) {
          std::reverse(srcLayers.begin(), srcLayers.end());
          std::reverse(dstLayers.begin(), dstLayers.end());
        }

        if (from.firstFrame() < to.firstFrame()) {
          auto srcFrames = from.selectedFrames().reversed();
          auto dstFrames = to.selectedFrames().reversed();

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

      case DocumentRange::kFrames: {
        frame_t dstFrame;
        if (place == kDocumentRangeBefore)
          dstFrame = to.firstFrame();
        else
          dstFrame = to.lastFrame()+1;

        resultRange =
          move_or_copy_frames(api, op, sprite,
                              from.selectedFrames(), dstFrame);
        break;
      }

      case DocumentRange::kLayers: {
        LayerList allLayers = sprite->allBrowsableLayers();
        if (allLayers.empty())
          break;

        LayerList srcLayers = from.selectedLayers().toLayerList();
        LayerList dstLayers = to.selectedLayers().toLayerList();
        ASSERT(!srcLayers.empty());
        ASSERT(!dstLayers.empty());

        switch (op) {

          case Move:
            if (place == kDocumentRangeBefore) {
              Layer* beforeThis = dstLayers.front();
              Layer* afterThis  = nullptr;

              for (Layer* srcLayer : srcLayers) {
                if (afterThis)
                  api.restackLayerAfter(srcLayer, afterThis);
                else
                  api.restackLayerBefore(srcLayer, beforeThis);

                afterThis = srcLayer;
              }
            }
            else if (place == kDocumentRangeAfter) {
              Layer* afterThis = dstLayers.back();
              for (Layer* srcLayer : srcLayers) {
                api.restackLayerAfter(srcLayer, afterThis);
                afterThis = srcLayer;
              }
            }

            // Same set of layers than the "from" range
            resultRange = from;
            break;

          case Copy: {
            if (place == kDocumentRangeBefore) {
              for (Layer* srcLayer :  srcLayers) {
                Layer* copiedLayer =
                  api.duplicateLayerBefore(srcLayer, dstLayers.front());

                resultRange.startRange(copiedLayer, -1, DocumentRange::kLayers);
                resultRange.endRange(copiedLayer, -1);
              }
            }
            else if (place == kDocumentRangeAfter) {
              std::reverse(srcLayers.begin(), srcLayers.end());

              for (Layer* srcLayer :  srcLayers) {
                Layer* copiedLayer =
                  api.duplicateLayerAfter(srcLayer, dstLayers.back());

                resultRange.startRange(copiedLayer, -1, DocumentRange::kLayers);
                resultRange.endRange(copiedLayer, -1);
              }
            }
            break;
          }
        }
        break;
      }
    }

    transaction.commit();
  }

  return resultRange;
}

DocumentRange move_range(Document* doc, const DocumentRange& from, const DocumentRange& to, DocumentRangePlace place)
{
  return drop_range_op(doc, Move, from, place, to);
}

DocumentRange copy_range(Document* doc, const DocumentRange& from, const DocumentRange& to, DocumentRangePlace place)
{
  return drop_range_op(doc, Copy, from, place, to);
}

void reverse_frames(Document* doc, const DocumentRange& range)
{
  const app::Context* context = static_cast<app::Context*>(doc->context());
  const ContextReader reader(context);
  ContextWriter writer(reader, 500);
  Transaction transaction(writer.context(), "Reverse Frames");
  DocumentApi api = doc->getApi(transaction);
  Sprite* sprite = doc->sprite();
  LayerList layers;
  frame_t frameBegin, frameEnd;
  bool moveFrames = false;
  bool swapCels = false;

  switch (range.type()) {
    case DocumentRange::kCels:
      frameBegin = range.firstFrame();
      frameEnd = range.lastFrame();
      layers = range.selectedLayers().toLayerList();
      swapCels = true;
      break;
    case DocumentRange::kFrames:
      frameBegin = range.firstFrame();
      frameEnd = range.lastFrame();
      layers = sprite->allLayers();
      moveFrames = true;
      break;
    case DocumentRange::kLayers:
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
      api.moveFrame(sprite, frameBegin, frameRev);
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

  transaction.commit();
}

} // namespace app
