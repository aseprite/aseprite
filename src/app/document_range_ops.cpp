// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/document_range_ops.h"

#include "app/app.h"            // TODO remove this dependency
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/document_range.h"
#include "app/transaction.h"
#include "doc/layer.h"
#include "doc/sprite.h"

#include <stdexcept>

namespace app {

enum Op { Move, Copy };

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
        if ((to.frameBegin() >= from.frameBegin() && to.frameEnd() <= from.frameEnd()) ||
            (place == kDocumentRangeBefore && to.frameBegin() == from.frameEnd()+1) ||
            (place == kDocumentRangeAfter && to.frameEnd() == from.frameBegin()-1))
          return from;
      }
      break;
    case DocumentRange::kLayers:
      if (op == Move) {
        if ((to.layerBegin() >= from.layerBegin() && to.layerEnd() <= from.layerEnd()) ||
            (place == kDocumentRangeBefore && to.layerBegin() == from.layerEnd()+1) ||
            (place == kDocumentRangeAfter && to.layerEnd() == from.layerBegin()-1))
          return from;

        // We cannot move the background
        for (LayerIndex i = from.layerBegin(); i <= from.layerEnd(); ++i)
          if (sprite->indexToLayer(i)->isBackground())
            throw std::runtime_error("The background layer cannot be moved");
      }

      // Before background
      if (place == kDocumentRangeBefore) {
        Layer* background = sprite->indexToLayer(to.layerBegin());
        if (background && background->isBackground())
          throw std::runtime_error("You cannot move or copy something below the background layer");
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
    // methods, to avoid generating a lot of SetCelFrame undoers (see
    // DocumentApi::setCelFramePosition).

    switch (from.type()) {

      case DocumentRange::kCels:
        {
          std::vector<Layer*> layers;
          sprite->getLayersList(layers);

          int srcLayerBegin, srcLayerStep, srcLayerEnd;
          int dstLayerBegin, dstLayerStep;
          frame_t srcFrameBegin, srcFrameStep, srcFrameEnd;
          frame_t dstFrameBegin, dstFrameStep;

          if (to.layerBegin() <= from.layerBegin()) {
            srcLayerBegin = from.layerBegin();
            srcLayerStep = 1;
            srcLayerEnd = from.layerEnd()+1;
            dstLayerBegin = to.layerBegin();
            dstLayerStep = 1;
          }
          else {
            srcLayerBegin = from.layerEnd();
            srcLayerStep = -1;
            srcLayerEnd = from.layerBegin()-1;
            dstLayerBegin = to.layerEnd();
            dstLayerStep = -1;
          }

          if (to.frameBegin() <= from.frameBegin()) {
            srcFrameBegin = from.frameBegin();
            srcFrameStep = frame_t(1);
            srcFrameEnd = from.frameEnd()+1;
            dstFrameBegin = to.frameBegin();
            dstFrameStep = frame_t(1);
          }
          else {
            srcFrameBegin = from.frameEnd();
            srcFrameStep = frame_t(-1);
            srcFrameEnd = from.frameBegin()-1;
            dstFrameBegin = to.frameEnd();
            dstFrameStep = frame_t(-1);
          }

          for (int srcLayerIdx = srcLayerBegin,
                 dstLayerIdx = dstLayerBegin; srcLayerIdx != srcLayerEnd; ) {
            for (frame_t srcFrame = srcFrameBegin,
                   dstFrame = dstFrameBegin; srcFrame != srcFrameEnd; ) {
              if (dstLayerIdx < 0 || dstLayerIdx >= int(layers.size()) ||
                  srcLayerIdx < 0 || srcLayerIdx >= int(layers.size()))
                break;

              LayerImage* srcLayer = static_cast<LayerImage*>(layers[srcLayerIdx]);
              LayerImage* dstLayer = static_cast<LayerImage*>(layers[dstLayerIdx]);

              switch (op) {
                case Move: api.moveCel(srcLayer, srcFrame, dstLayer, dstFrame); break;
                case Copy: api.copyCel(srcLayer, srcFrame, dstLayer, dstFrame); break;
              }

              srcFrame += srcFrameStep;
              dstFrame += dstFrameStep;
            }
            srcLayerIdx += srcLayerStep;
            dstLayerIdx += dstLayerStep;
          }

          resultRange = to;
        }
        break;

      case DocumentRange::kFrames:
        {
          frame_t srcFrameBegin = 0, srcFrameStep, srcFrameEnd = 0;
          frame_t dstFrameBegin = 0, dstFrameStep;

          switch (op) {

            case Move:
              if (place == kDocumentRangeBefore) {
                if (to.frameBegin() <= from.frameBegin()) {
                  srcFrameBegin = from.frameBegin();
                  srcFrameStep = frame_t(1);
                  srcFrameEnd = from.frameEnd()+1;
                  dstFrameBegin = to.frameBegin();
                  dstFrameStep = frame_t(1);
                }
                else {
                  srcFrameBegin = from.frameEnd();
                  srcFrameStep = frame_t(-1);
                  srcFrameEnd = from.frameBegin()-1;
                  dstFrameBegin = to.frameBegin();
                  dstFrameStep = frame_t(-1);
                }
              }
              else if (place == kDocumentRangeAfter) {
                if (to.frameEnd() <= from.frameBegin()) {
                  srcFrameBegin = from.frameBegin();
                  srcFrameStep = frame_t(1);
                  srcFrameEnd = from.frameEnd()+1;
                  dstFrameBegin = to.frameEnd()+1;
                  dstFrameStep = frame_t(1);
                }
                else {
                  srcFrameBegin = from.frameEnd();
                  srcFrameStep = frame_t(-1);
                  srcFrameEnd = from.frameBegin()-1;
                  dstFrameBegin = to.frameEnd()+1;
                  dstFrameStep = frame_t(-1);
                }
              }
              break;

            case Copy:
              if (place == kDocumentRangeBefore) {
                if (to.frameBegin() <= from.frameBegin()) {
                  srcFrameBegin = from.frameBegin();
                  srcFrameStep = frame_t(2);
                  srcFrameEnd = from.frameBegin() + 2*from.frames();
                  dstFrameBegin = to.frameBegin();
                  dstFrameStep = frame_t(1);
                }
                else {
                  srcFrameBegin = from.frameEnd();
                  srcFrameStep = frame_t(-1);
                  srcFrameEnd = from.frameBegin()-1;
                  dstFrameBegin = to.frameBegin();
                  dstFrameStep = frame_t(0);
                }
              }
              else if (place == kDocumentRangeAfter) {
                if (to.frameEnd() <= from.frameBegin()) {
                  srcFrameBegin = from.frameBegin();
                  srcFrameStep = frame_t(2);
                  srcFrameEnd = from.frameBegin() + 2*from.frames();
                  dstFrameBegin = to.frameEnd()+1;
                  dstFrameStep = frame_t(1);
                }
                else {
                  srcFrameBegin = from.frameEnd();
                  srcFrameStep = frame_t(-1);
                  srcFrameEnd = from.frameBegin()-1;
                  dstFrameBegin = to.frameEnd()+1;
                  dstFrameStep = frame_t(0);
                }
              }
              break;
          }

          for (frame_t srcFrame = srcFrameBegin,
                 dstFrame = dstFrameBegin; srcFrame != srcFrameEnd; ) {
            switch (op) {
              case Move: api.moveFrame(sprite, srcFrame, dstFrame); break;
              case Copy: api.copyFrame(sprite, srcFrame, dstFrame); break;
            }
            srcFrame += srcFrameStep;
            dstFrame += dstFrameStep;
          }

          if (place == kDocumentRangeBefore) {
            resultRange.startRange(LayerIndex::NoLayer, frame_t(to.frameBegin()), from.type());
            resultRange.endRange(LayerIndex::NoLayer, frame_t(to.frameBegin()+from.frames()-1));
          }
          else if (place == kDocumentRangeAfter) {
            resultRange.startRange(LayerIndex::NoLayer, frame_t(to.frameEnd()+1), from.type());
            resultRange.endRange(LayerIndex::NoLayer, frame_t(to.frameEnd()+1+from.frames()-1));
          }

          if (op == Move && from.frameBegin() < to.frameBegin())
            resultRange.displace(0, -from.frames());
        }
        break;

      case DocumentRange::kLayers:
        {
          std::vector<Layer*> layers;
          sprite->getLayersList(layers);

          if (layers.empty())
            break;

          switch (op) {

            case Move:
              if (place == kDocumentRangeBefore) {
                for (LayerIndex i = from.layerBegin(); i <= from.layerEnd(); ++i) {
                  api.restackLayerBefore(
                    layers[i],
                    layers[to.layerBegin()]);
                }
              }
              else if (place == kDocumentRangeAfter) {
                for (LayerIndex i = from.layerEnd(); i >= from.layerBegin(); --i) {
                  api.restackLayerAfter(
                    layers[i],
                    layers[to.layerEnd()]);
                }
              }
              break;

            case Copy:
              if (place == kDocumentRangeBefore) {
                for (LayerIndex i = from.layerBegin(); i <= from.layerEnd(); ++i) {
                  api.duplicateLayerBefore(
                    layers[i],
                    layers[to.layerBegin()]);
                }
              }
              else if (place == kDocumentRangeAfter) {
                for (LayerIndex i = from.layerEnd(); i >= from.layerBegin(); --i) {
                  api.duplicateLayerAfter(
                    layers[i],
                    layers[to.layerEnd()]);
                }
              }
              break;
          }

          if (place == kDocumentRangeBefore) {
            resultRange.startRange(LayerIndex(to.layerBegin()), frame_t(-1), from.type());
            resultRange.endRange(LayerIndex(to.layerBegin()+from.layers()-1), frame_t(-1));
          }
          else if (place == kDocumentRangeAfter) {
            resultRange.startRange(LayerIndex(to.layerEnd()+1), frame_t(-1), from.type());
            resultRange.endRange(LayerIndex(to.layerEnd()+1+from.layers()-1), frame_t(-1));
          }

          if (op == Move && from.layerBegin() < to.layerBegin())
            resultRange.displace(-from.layers(), 0);
        }
        break;
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
  frame_t frameBegin, frameEnd;
  int layerBegin, layerEnd;
  bool moveFrames = false;
  bool swapCels = false;

  switch (range.type()) {
    case DocumentRange::kCels:
      frameBegin = range.frameBegin();
      frameEnd = range.frameEnd();
      layerBegin = range.layerBegin();
      layerEnd = range.layerEnd() + 1;
      swapCels = true;
      break;
    case DocumentRange::kFrames:
      frameBegin = range.frameBegin();
      frameEnd = range.frameEnd();
      moveFrames = true;
      break;
    case DocumentRange::kLayers:
      frameBegin = frame_t(0);
      frameEnd = sprite->totalFrames()-1;
      layerBegin = range.layerBegin();
      layerEnd = range.layerEnd() + 1;
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
    std::vector<Layer*> layers;
    sprite->getLayersList(layers);

    for (int layerIdx = layerBegin; layerIdx != layerEnd; ++layerIdx) {
      for (frame_t frame = frameBegin,
             frameRev = frameEnd;
           frame != (frameBegin+frameEnd)/2+1;
           ++frame, --frameRev) {
        if (frame == frameRev)
          continue;

        LayerImage* layer = static_cast<LayerImage*>(layers[layerIdx]);
        api.swapCel(layer, frame, frameRev);
      }
    }
  }

  transaction.commit();
}

} // namespace app
