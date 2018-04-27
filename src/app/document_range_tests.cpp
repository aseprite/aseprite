// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "tests/test.h"

#include "app/context.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/document_range.h"
#include "app/document_range_ops.h"
#include "app/document_undo.h"
#include "app/transaction.h"
#include "base/unique_ptr.h"
#include "doc/doc.h"
#include "doc/test_context.h"

using namespace app;
using namespace doc;

namespace std {

std::ostream& operator<<(std::ostream& os, const doc::Layer* layer) {
  if (layer)
    os << '"' << layer->name() << '"';
  else
    os << "(null layer)";
  return os;
}

std::ostream& operator<<(std::ostream& os, const app::DocumentRange& range) {
  os << "{ layers: [";

  bool first = true;
  for (auto layer : range.selectedLayers()) {
    if (first)
      first = false;
    else
      os << ", ";
    os << layer;
  }

  os << "]";
  os << ", frames: [";

  first = true;
  for (auto frame : range.selectedFrames()) {
    if (first)
      first = false;
    else
      os << ", ";
    os << frame;
  }

  os << "] }";
  return os;
}

std::ostream& operator<<(std::ostream& os, const doc::LayerList& layers) {
  os << "[";
  bool first = true;
  for (auto layer : layers) {
    if (first)
      first = false;
    else
      os << ", ";
    os << layer;
  }
  return os << "]";
}

} // namespace std

typedef base::UniquePtr<app::Document> DocumentPtr;

#define EXPECT_LAYER_ORDER(a, b, c, d) \
  EXPECT_TRUE(expect_layer(a, 0));     \
  EXPECT_TRUE(expect_layer(b, 1));     \
  EXPECT_TRUE(expect_layer(c, 2));     \
  EXPECT_TRUE(expect_layer(d, 3));

#define EXPECT_FRAME_ORDER(a, b, c, d) \
  EXPECT_TRUE(expect_frame(a, 0) &&    \
              expect_frame(b, 1) &&    \
              expect_frame(c, 2) &&    \
              expect_frame(d, 3));

#define EXPECT_FRAME_ORDER6(a, b, c, d, e, f)   \
  EXPECT_TRUE(expect_frame(a, 0) &&             \
              expect_frame(b, 1) &&             \
              expect_frame(c, 2) &&             \
              expect_frame(d, 3) &&             \
              expect_frame(e, 4) &&             \
              expect_frame(f, 5));

#define EXPECT_FRAME_COPY1(a, b, c, d, e) \
  EXPECT_TRUE(expect_frame(a, 0));        \
  EXPECT_TRUE(expect_frame(b, 1));        \
  EXPECT_TRUE(expect_frame(c, 2));        \
  EXPECT_TRUE(expect_frame(d, 3));        \
  EXPECT_TRUE(expect_frame(e, 4));

#define EXPECT_FRAME_COPY2(a, b, c, d, e, f)    \
  EXPECT_TRUE(expect_frame(a, 0));              \
  EXPECT_TRUE(expect_frame(b, 1));              \
  EXPECT_TRUE(expect_frame(c, 2));              \
  EXPECT_TRUE(expect_frame(d, 3));              \
  EXPECT_TRUE(expect_frame(e, 4));              \
  EXPECT_TRUE(expect_frame(f, 5));

#define EXPECT_FRAME_COPY3(a, b, c, d, e, f, g) \
  EXPECT_TRUE(expect_frame(a, 0));              \
  EXPECT_TRUE(expect_frame(b, 1));              \
  EXPECT_TRUE(expect_frame(c, 2));              \
  EXPECT_TRUE(expect_frame(d, 3));              \
  EXPECT_TRUE(expect_frame(e, 4));              \
  EXPECT_TRUE(expect_frame(f, 5));              \
  EXPECT_TRUE(expect_frame(g, 6));

#define EXPECT_CEL(y, x, v, u)                  \
  EXPECT_TRUE(expect_cel(y, x, v, u));

#define EXPECT_EMPTY_CEL(y, x)                  \
  EXPECT_TRUE(expect_empty_cel(y, x));

class DocRangeOps : public ::testing::Test {
public:
  DocRangeOps() {
    expected_color = rgba(255, 255, 255, 255);

    doc.reset(static_cast<app::Document*>(ctx.documents().add(6, 4)));
    sprite = doc->sprite();
    layer1 = dynamic_cast<LayerImage*>(sprite->root()->firstLayer());
    layer2 = new LayerImage(sprite);
    layer3 = new LayerImage(sprite);
    layer4 = new LayerImage(sprite);
    sprite->root()->addLayer(layer2);
    sprite->root()->addLayer(layer3);
    sprite->root()->addLayer(layer4);
    EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);

    layer1->setName("layer1");
    layer2->setName("layer2");
    layer3->setName("layer3");
    layer4->setName("layer4");

    sprite->setTotalFrames(frame_t(6));
    sprite->setFrameDuration(frame_t(0), 1); // These durations can be used to identify
    sprite->setFrameDuration(frame_t(1), 2); // frames after a move operation
    sprite->setFrameDuration(frame_t(2), 3);
    sprite->setFrameDuration(frame_t(3), 4);
    sprite->setFrameDuration(frame_t(4), 5);
    sprite->setFrameDuration(frame_t(5), 6);

    LayerList layers = sprite->allLayers();

    for (layer_t i=0; i<layers.size(); i++) {
      LayerImage* layer = static_cast<LayerImage*>(layers[i]);

      for (frame_t j=0; j<sprite->totalFrames(); j++) {
        Cel* cel = layer->cel(frame_t(j));
        ImageRef image;
        if (cel)
          image = cel->imageRef();
        else {
          image.reset(Image::create(IMAGE_RGB, 6, 4));
          cel = new Cel(frame_t(j), image);
          layer->addCel(cel);
        }

        clear_image(image.get(), i + j*100);
        put_pixel(image.get(), i, j, expected_color);
      }
    }
  }

  ~DocRangeOps() {
    doc->close();
  }

protected:
  bool expect_layer(Layer* expected_layer, layer_t layer) {
    LayerList layers = sprite->allLayers();
    return expect_layer_frame(
      find_layer_index(layers, expected_layer), -1, layer, -1);
  }

  bool expect_frame(layer_t expected_frame, frame_t frame) {
    LayerList layers = sprite->allLayers();
    for (int i=0; i<(int)layers.size(); ++i) {
      if (!expect_layer_frame(i, expected_frame, i, frame))
        return false;
    }
    return true;
  }

  bool expect_layer_frame(layer_t expected_layer, frame_t expected_frame,
                          layer_t layer, frame_t frame) {
    if (frame >= 0) {
      if (!expect_cel(expected_layer, expected_frame, layer, frame))
        return false;

      EXPECT_EQ((expected_frame+1), sprite->frameDuration(frame));
      return ((expected_frame+1) == sprite->frameDuration(frame));
    }

    if (layer >= 0) {
      LayerList layers = sprite->allLayers();
      Layer* a = layers[expected_layer];
      Layer* b = layers[layer];
      EXPECT_EQ(a->name(), b->name()) << layers;
      if (a != b)
        return false;
    }

    return true;
  }

  bool expect_cel(layer_t expected_layer, frame_t expected_frame,
                  layer_t layer, frame_t frame) {
    LayerList layers = sprite->allLayers();
    Cel* cel = layers[layer]->cel(frame);
    if (!cel)
      return false;

    color_t color = get_pixel(
      cel->image(),
      expected_layer, expected_frame);

    EXPECT_EQ(expected_color, color)
      << " - expecting layer " << expected_layer << " in " << layer << " and it is " << int(color%100) << "\n"
      << " - expecting frame " << expected_frame << " in " << frame << " and it is " << int(color/100);

    return (expected_color == color);
  }

  bool expect_empty_cel(layer_t layer, frame_t frame) {
    LayerList layers = sprite->allLayers();
    Cel* cel = layers[layer]->cel(frame);

    EXPECT_EQ(NULL, cel);
    return (cel == NULL);
  }

  DocumentRange range(Layer* fromLayer, frame_t fromFrNum, Layer* toLayer, frame_t toFrNum, DocumentRange::Type type) {
    DocumentRange r;
    r.startRange(fromLayer, fromFrNum, type);
    r.endRange(toLayer, toFrNum);
    return r;
  }

  DocumentRange range(layer_t fromLayer, frame_t fromFrNum,
                      layer_t toLayer, frame_t toFrNum, DocumentRange::Type type) {
    LayerList layers = sprite->allLayers();
    return range(layers[fromLayer], fromFrNum, layers[toLayer], toFrNum, type);
  }

  DocumentRange layers_range(Layer* fromLayer, Layer* toLayer) {
    return range(fromLayer, -1, toLayer, -1, DocumentRange::kLayers);
  }

  DocumentRange layers_range(layer_t fromLayer, layer_t toLayer) {
    LayerList layers = sprite->allLayers();
    return layers_range(layers[fromLayer], layers[toLayer]);
  }

  DocumentRange layers_range(Layer* layer) {
    return range(layer, -1, layer, -1, DocumentRange::kLayers);
  }

  DocumentRange layers_range(layer_t layer) {
    LayerList layers = sprite->allLayers();
    return layers_range(layers[layer]);
  }

  DocumentRange frames_range(frame_t fromFrame, frame_t toFrame) {
    return range(nullptr, fromFrame, nullptr, toFrame, DocumentRange::kFrames);
  }

  DocumentRange frames_range(frame_t frame) {
    return range(nullptr, frame, nullptr, frame, DocumentRange::kFrames);
  }

  DocumentRange cels_range(layer_t fromLayer, frame_t fromFrNum,
                           layer_t toLayer, frame_t toFrNum) {
    return range(fromLayer, fromFrNum, toLayer, toFrNum, DocumentRange::kCels);
  }

  TestContextT<app::Context> ctx;
  DocumentPtr doc;
  Sprite* sprite;
  LayerImage* layer1;
  LayerImage* layer2;
  LayerImage* layer3;
  LayerImage* layer4;
  color_t expected_color;
};

TEST_F(DocRangeOps, MoveLayersNoOp) {
  // Move one layer to the same place

  EXPECT_EQ(layers_range(layer1),
            move_range(doc,
                       layers_range(layer1),
                       layers_range(layer1), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer1),
            move_range(doc,
                       layers_range(layer1),
                       layers_range(layer2), kDocumentRangeBefore));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer4),
            move_range(doc,
                       layers_range(layer4),
                       layers_range(layer4), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer4),
            move_range(doc,
                       layers_range(layer4),
                       layers_range(layer4), kDocumentRangeBefore));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer4),
            move_range(doc,
                       layers_range(layer4),
                       layers_range(layer3), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  // Move two layer to the same place

  EXPECT_EQ(layers_range(layer1, layer2),
            move_range(doc,
                       layers_range(layer1, layer2),
                       layers_range(layer1), kDocumentRangeBefore));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer1, layer2),
            move_range(doc,
                       layers_range(layer1, layer2),
                       layers_range(layer1), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer1, layer2),
            move_range(doc,
                       layers_range(layer1, layer2),
                       layers_range(layer2), kDocumentRangeBefore));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer1, layer2),
            move_range(doc,
                       layers_range(layer1, layer2),
                       layers_range(layer2), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer1, layer2),
            move_range(doc,
                       layers_range(layer1, layer2),
                       layers_range(layer3), kDocumentRangeBefore));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer3, layer4),
            move_range(doc,
                       layers_range(layer3, layer4),
                       layers_range(layer2), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer3, layer4),
            move_range(doc,
                       layers_range(layer3, layer4),
                       layers_range(layer3), kDocumentRangeBefore));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer3, layer4),
            move_range(doc,
                       layers_range(layer3, layer4),
                       layers_range(layer3), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer3, layer4),
            move_range(doc,
                       layers_range(layer3, layer4),
                       layers_range(layer4), kDocumentRangeBefore));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(layers_range(layer3, layer4),
            move_range(doc,
                       layers_range(layer3, layer4),
                       layers_range(layer4), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  // Move four layers

  DocumentRangePlace places[] = { kDocumentRangeBefore, kDocumentRangeAfter };
  for (int i=0; i<2; ++i) {
    for (int layer=0; layer<4; ++layer) {
      EXPECT_EQ(layers_range(layer1, layer4),
                move_range(doc,
                           layers_range(layer1, layer4),
                           layers_range(layer), places[i]));
      EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
      EXPECT_FALSE(doc->undoHistory()->canUndo());
    }

    for (int layer=0; layer<3; ++layer) {
      EXPECT_EQ(layers_range(layer1, layer4),
                move_range(doc,
                           layers_range(layer1, layer4),
                           layers_range(layer, layer+1), places[i]));
      EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
      EXPECT_FALSE(doc->undoHistory()->canUndo());
    }

    for (int layer=0; layer<2; ++layer) {
      EXPECT_EQ(layers_range(layer1, layer4),
                move_range(doc,
                           layers_range(layer1, layer4),
                           layers_range(layer, layer+2), places[i]));
      EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
      EXPECT_FALSE(doc->undoHistory()->canUndo());
    }

    EXPECT_EQ(layers_range(layer1, layer4),
              move_range(doc,
                         layers_range(layer1, layer4),
                         layers_range(layer1, layer4), places[i]));
    EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
    EXPECT_FALSE(doc->undoHistory()->canUndo());
  }
}

TEST_F(DocRangeOps, MoveFramesNoOp) {
  // Move one frame to the same place

  EXPECT_EQ(frames_range(0),
            move_range(doc,
                       frames_range(0),
                       frames_range(0), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(0),
            move_range(doc,
                       frames_range(0),
                       frames_range(1), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(3),
            move_range(doc,
                       frames_range(3),
                       frames_range(3), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(3),
            move_range(doc,
                       frames_range(3),
                       frames_range(3), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(3),
            move_range(doc,
                       frames_range(3),
                       frames_range(2), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  // Move two frame to the same place

  EXPECT_EQ(frames_range(0, 1),
            move_range(doc,
                       frames_range(0, 1),
                       frames_range(0), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(0, 1),
            move_range(doc,
                       frames_range(0, 1),
                       frames_range(0), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(0, 1),
            move_range(doc,
                       frames_range(0, 1),
                       frames_range(1), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(0, 1),
            move_range(doc,
                       frames_range(0, 1),
                       frames_range(1), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(0, 1),
            move_range(doc,
                       frames_range(0, 1),
                       frames_range(2), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(2, 3),
            move_range(doc,
                       frames_range(2, 3),
                       frames_range(1), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(2, 3),
            move_range(doc,
                       frames_range(2, 3),
                       frames_range(2), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(2, 3),
            move_range(doc,
                       frames_range(2, 3),
                       frames_range(2), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(2, 3),
            move_range(doc,
                       frames_range(2, 3),
                       frames_range(3), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  EXPECT_EQ(frames_range(2, 3),
            move_range(doc,
                       frames_range(2, 3),
                       frames_range(3), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
  EXPECT_FALSE(doc->undoHistory()->canUndo());

  // Move four frames

  DocumentRangePlace places[] = { kDocumentRangeBefore, kDocumentRangeAfter };
  for (int i=0; i<2; ++i) {
    for (int frame=0; frame<4; ++frame) {
      EXPECT_EQ(frames_range(0, 3),
                move_range(doc,
                           frames_range(0, 3),
                           frames_range(frame), places[i]));
      EXPECT_FRAME_ORDER(0, 1, 2, 3);
      EXPECT_FALSE(doc->undoHistory()->canUndo());
    }

    for (int frame=0; frame<3; ++frame) {
      EXPECT_EQ(frames_range(0, 3),
                move_range(doc,
                           frames_range(0, 3),
                           frames_range(frame, frame+1), places[i]));
      EXPECT_FRAME_ORDER(0, 1, 2, 3);
      EXPECT_FALSE(doc->undoHistory()->canUndo());
    }

    for (int frame=0; frame<2; ++frame) {
      EXPECT_EQ(frames_range(0, 3),
                move_range(doc,
                           frames_range(0, 3),
                           frames_range(frame, frame+2), places[i]));
      EXPECT_FRAME_ORDER(0, 1, 2, 3);
      EXPECT_FALSE(doc->undoHistory()->canUndo());
    }

    EXPECT_EQ(frames_range(0, 3),
              move_range(doc,
                         frames_range(0, 3),
                         frames_range(0, 3), places[i]));
    EXPECT_FRAME_ORDER(0, 1, 2, 3);
    EXPECT_FALSE(doc->undoHistory()->canUndo());
  }
}

TEST_F(DocRangeOps, MoveCelsNoOp) {
  EXPECT_EQ(cels_range(0, 0, 1, 1),
            move_range(doc,
                       cels_range(0, 0, 1, 1),
                       cels_range(0, 0, 1, 1), kDocumentRangeAfter));
  EXPECT_CEL(0, 0, 0, 0);
  EXPECT_CEL(0, 1, 0, 1);
  EXPECT_CEL(1, 0, 1, 0);
  EXPECT_CEL(1, 1, 1, 1);
  EXPECT_FALSE(doc->undoHistory()->canUndo());
}

TEST_F(DocRangeOps, CopyCelsNoOp) {
  // TODO
}

TEST_F(DocRangeOps, MoveLayers) {
  // One layer at the bottom of another
  EXPECT_EQ(layers_range(layer1),
            move_range(doc,
                       layers_range(layer1),
                       layers_range(layer2), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer2, layer1, layer3, layer4);
  doc->undoHistory()->undo();
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);

  // One layer at the bottom
  EXPECT_EQ(layers_range(layer2),
            move_range(doc,
                       layers_range(layer2),
                       layers_range(layer1), kDocumentRangeBefore));
  EXPECT_LAYER_ORDER(layer2, layer1, layer3, layer4);
  doc->undoHistory()->undo();
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);

  // Try with a background
  layer1->setBackground(true);
  EXPECT_THROW({
      move_range(doc,
        layers_range(layer1),
        layers_range(layer2), kDocumentRangeAfter);
    }, std::exception);
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  layer1->setBackground(false);

  // Move one layer to the top
  EXPECT_EQ(layers_range(layer2),
            move_range(doc,
                       layers_range(layer2),
                       layers_range(layer4), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer1, layer3, layer4, layer2);
  doc->undoHistory()->undo();
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);

  // Move one layers before other
  EXPECT_EQ(layers_range(layer2),
            move_range(doc,
                       layers_range(layer2),
                       layers_range(layer4), kDocumentRangeBefore));
  EXPECT_LAYER_ORDER(layer1, layer3, layer2, layer4);
  doc->undoHistory()->undo();
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);

  EXPECT_EQ(layers_range(layer1),
            move_range(doc,
                       layers_range(layer1),
                       layers_range(layer3, layer4), kDocumentRangeBefore));
  EXPECT_LAYER_ORDER(layer2, layer1, layer3, layer4);
  doc->undoHistory()->undo();
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);

  // Move two layers on top of other
  EXPECT_EQ(layers_range(layer2, layer3),
            move_range(doc,
                       layers_range(layer2, layer3),
                       layers_range(layer4), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer1, layer4, layer2, layer3);

  EXPECT_EQ(layers_range(layer2, layer3),
            move_range(doc,
                       layers_range(layer2, layer3),
                       layers_range(layer1), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);

  // Move three layers at the bottom (but we cannot because the bottom is a background layer)
  layer1->setBackground(true);
  EXPECT_THROW({
      move_range(doc,
        layers_range(layer2, layer4),
        layers_range(layer1), kDocumentRangeBefore);
    }, std::exception);
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
  layer1->setBackground(false);

  // Move three layers at the top
  EXPECT_EQ(layers_range(layer1, layer3),
            move_range(doc,
                       layers_range(layer1, layer3),
                       layers_range(layer4), kDocumentRangeAfter));
  EXPECT_LAYER_ORDER(layer4, layer1, layer2, layer3);
  doc->undoHistory()->undo();
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);

  // Move three layers at the bottom
  EXPECT_EQ(layers_range(layer2, layer4),
            move_range(doc,
                       layers_range(layer2, layer4),
                       layers_range(layer1), kDocumentRangeBefore));
  EXPECT_LAYER_ORDER(layer2, layer3, layer4, layer1);
  doc->undoHistory()->undo();
  EXPECT_LAYER_ORDER(layer1, layer2, layer3, layer4);
}

TEST_F(DocRangeOps, MoveFrames) {
  // Move frame 0 after 1
  EXPECT_EQ(frames_range(1),
            move_range(doc,
                       frames_range(0),
                       frames_range(1), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(1, 0, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  // Move frame 1 after frame 5 (at the end)
  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);
  EXPECT_EQ(frames_range(5),
            move_range(doc,
                       frames_range(1),
                       frames_range(5), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER6(0, 2, 3, 4, 5, 1);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);

  // Move frames 1,2 after 5
  EXPECT_EQ(frames_range(4, 5),
            move_range(
              doc,
              frames_range(1, 2),
              frames_range(5), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER6(0, 3, 4, 5, 1, 2);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);

  // Move frames 1,2 after 3
  EXPECT_EQ(frames_range(1, 2),
            move_range(doc,
                       frames_range(2, 3),
                       frames_range(0), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(0, 2, 3, 1);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  // Move three frames at the beginning
  EXPECT_EQ(frames_range(0, 2),
            move_range(doc,
                       frames_range(1, 3),
                       frames_range(0), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER(1, 2, 3, 0);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  // Move three frames at the end
  EXPECT_EQ(frames_range(1, 3),
            move_range(doc,
                       frames_range(0, 2),
                       frames_range(3), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(3, 0, 1, 2);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
}

TEST_F(DocRangeOps, MoveFramesNonAdjacent) {
  // Move frames 0,2...

  DocumentRange from;
  from.startRange(nullptr, 0, DocumentRange::kFrames); from.endRange(nullptr, 0);
  from.startRange(nullptr, 2, DocumentRange::kFrames); from.endRange(nullptr, 2);

  // Move frames 0,2 after 3...

  EXPECT_EQ(frames_range(2, 3),
            move_range(doc, from, frames_range(3), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(1, 3, 0, 2);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  // Move frames 0,2 before 3...

  EXPECT_EQ(frames_range(1, 2),
            move_range(doc, from, frames_range(3), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER(1, 0, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  // Move frames 0,2 before 0...

  EXPECT_EQ(frames_range(0, 1),
            move_range(doc, from, frames_range(0), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER(0, 2, 1, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  // Move frames 0,2 after 0...

  EXPECT_EQ(frames_range(0, 1),
            move_range(doc, from, frames_range(0), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(0, 2, 1, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  // Move frames 0,2 before 1...

  EXPECT_EQ(frames_range(0, 1),
            move_range(doc, from, frames_range(1), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER(0, 2, 1, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  // Move frames 0,2 after 1...

  EXPECT_EQ(frames_range(1, 2),
            move_range(doc, from, frames_range(1), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER(1, 0, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  // Move 1,2,5...

  from.clearRange();
  from.startRange(nullptr, 1, DocumentRange::kFrames);
  from.endRange(nullptr, 2);
  from.startRange(nullptr, 5, DocumentRange::kFrames);
  from.endRange(nullptr, 5);

  // Move 1,2,5 before 4...

  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);
  EXPECT_EQ(frames_range(2, 4),
            move_range(doc, from, frames_range(4), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER6(0, 3, 1, 2, 5, 4);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);

  // Move 1,2,5 after 4...

  EXPECT_EQ(frames_range(3, 5),
            move_range(doc, from, frames_range(4), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER6(0, 3, 4, 1, 2, 5);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);

  // Move 1,2,5 before 2...

  EXPECT_EQ(frames_range(1, 3),
            move_range(doc, from, frames_range(2), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER6(0, 1, 2, 5, 3, 4);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);

  // Move 1,2,5 after 2...

  EXPECT_EQ(frames_range(1, 3),
            move_range(doc, from, frames_range(2), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER6(0, 1, 2, 5, 3, 4);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);

  // Move 1,2,5 before 1...

  EXPECT_EQ(frames_range(1, 3),
            move_range(doc, from, frames_range(1), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER6(0, 1, 2, 5, 3, 4);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);

  // Move 1,3,5...

  from.clearRange();
  from.startRange(nullptr, 1, DocumentRange::kFrames);
  from.endRange(nullptr, 1);
  from.startRange(nullptr, 3, DocumentRange::kFrames);
  from.endRange(nullptr, 3);
  from.startRange(nullptr, 5, DocumentRange::kFrames);
  from.endRange(nullptr, 5);

  // Move 1,3,5 before 4...

  EXPECT_EQ(frames_range(2, 4),
            move_range(doc, from, frames_range(4), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER6(0, 2, 1, 3, 5, 4);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);

  // Move 1,3,5 after 4...

  EXPECT_EQ(frames_range(3, 5),
            move_range(doc, from, frames_range(4), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER6(0, 2, 4, 1, 3, 5);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);

  // Move 1,3,5 before 5...

  EXPECT_EQ(frames_range(3, 5),
            move_range(doc, from, frames_range(5), kDocumentRangeBefore));
  EXPECT_FRAME_ORDER6(0, 2, 4, 1, 3, 5);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);

  // Move 1,3,5 after 5...

  EXPECT_EQ(frames_range(3, 5),
            move_range(doc, from, frames_range(5), kDocumentRangeAfter));
  EXPECT_FRAME_ORDER6(0, 2, 4, 1, 3, 5);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER6(0, 1, 2, 3, 4, 5);
}

TEST_F(DocRangeOps, MoveCels) {
  DocumentRangePlace ignore = kDocumentRangeBefore;

  move_range(doc,
             cels_range(0, 0, 0, 0),
             cels_range(0, 1, 0, 1), ignore);
  EXPECT_CEL(0, 0, 0, 1);
  EXPECT_EMPTY_CEL(0, 0);
  doc->undoHistory()->undo();
  EXPECT_CEL(0, 0, 0, 0);

  move_range(doc,
             cels_range(0, 0, 0, 1),
             cels_range(0, 2, 0, 3), ignore);
  EXPECT_CEL(0, 0, 0, 2);
  EXPECT_CEL(0, 1, 0, 3);
  EXPECT_EMPTY_CEL(0, 0);
  EXPECT_EMPTY_CEL(0, 1);
  doc->undoHistory()->undo();

  move_range(doc,
             cels_range(0, 0, 1, 1),
             cels_range(2, 2, 3, 3), ignore);
  EXPECT_CEL(0, 0, 2, 2);
  EXPECT_CEL(0, 1, 2, 3);
  EXPECT_CEL(1, 0, 3, 2);
  EXPECT_CEL(1, 1, 3, 3);
  EXPECT_EMPTY_CEL(0, 0);
  EXPECT_EMPTY_CEL(0, 1);
  EXPECT_EMPTY_CEL(1, 0);
  EXPECT_EMPTY_CEL(1, 1);
  doc->undoHistory()->undo();

  move_range(doc,
             cels_range(0, 0, 0, 3),
             cels_range(1, 0, 1, 3), ignore);
  EXPECT_CEL(0, 0, 1, 0);
  EXPECT_CEL(0, 1, 1, 1);
  EXPECT_CEL(0, 2, 1, 2);
  EXPECT_CEL(0, 3, 1, 3);
  EXPECT_EMPTY_CEL(0, 0);
  EXPECT_EMPTY_CEL(0, 1);
  EXPECT_EMPTY_CEL(0, 2);
  EXPECT_EMPTY_CEL(0, 3);
  doc->undoHistory()->undo();

  // Moving with overlapping areas
  move_range(doc,
             cels_range(0, 0, 0, 2),
             cels_range(0, 1, 0, 3), ignore);
  EXPECT_CEL(0, 0, 0, 1);
  EXPECT_CEL(0, 1, 0, 2);
  EXPECT_CEL(0, 2, 0, 3);
  EXPECT_EMPTY_CEL(0, 0);
  doc->undoHistory()->undo();
}

TEST_F(DocRangeOps, CopyLayers) {
  // TODO
}

TEST_F(DocRangeOps, CopyFrames) {
  // Copy one frame
  EXPECT_EQ(frames_range(2),
            copy_range(doc,
                       frames_range(0),
                       frames_range(2, 3), kDocumentRangeBefore));
  EXPECT_FRAME_COPY1(0, 1, 0, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  EXPECT_EQ(frames_range(4),
            copy_range(doc,
                       frames_range(0),
                       frames_range(2, 3), kDocumentRangeAfter));
  EXPECT_FRAME_COPY1(0, 1, 2, 3, 0);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  EXPECT_EQ(frames_range(0),
            copy_range(doc,
                       frames_range(3),
                       frames_range(0, 1), kDocumentRangeBefore));
  EXPECT_FRAME_COPY1(3, 0, 1, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  EXPECT_EQ(frames_range(2),
            copy_range(doc,
                       frames_range(3),
                       frames_range(0, 1), kDocumentRangeAfter));
  EXPECT_FRAME_COPY1(0, 1, 3, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  // Copy three frames

  EXPECT_EQ(frames_range(3, 5),
            copy_range(doc,
                       frames_range(0, 2),
                       frames_range(3), kDocumentRangeBefore));
  EXPECT_FRAME_COPY3(0, 1, 2, 0, 1, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  EXPECT_EQ(frames_range(4, 6),
            copy_range(doc,
                       frames_range(0, 2),
                       frames_range(3), kDocumentRangeAfter));
  EXPECT_FRAME_COPY3(0, 1, 2, 3, 0, 1, 2);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  EXPECT_EQ(frames_range(0, 2),
            copy_range(doc,
                       frames_range(1, 3),
                       frames_range(0), kDocumentRangeBefore));
  EXPECT_FRAME_COPY3(1, 2, 3, 0, 1, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  EXPECT_EQ(frames_range(1, 3),
            copy_range(doc,
                       frames_range(1, 3),
                       frames_range(0), kDocumentRangeAfter));
  EXPECT_FRAME_COPY3(0, 1, 2, 3, 1, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  EXPECT_EQ(frames_range(0, 2),
            copy_range(doc,
                       frames_range(0, 2),
                       frames_range(0, 2), kDocumentRangeBefore));
  EXPECT_FRAME_COPY3(0, 1, 2, 0, 1, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  EXPECT_EQ(frames_range(3, 5),
            copy_range(doc,
                       frames_range(0, 2),
                       frames_range(0, 2), kDocumentRangeAfter));
  EXPECT_FRAME_COPY3(0, 1, 2, 0, 1, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
}

TEST_F(DocRangeOps, CopyFramesNonAdjacent) {
  // Copy frames 0 and 2...

  DocumentRange from;
  from.startRange(nullptr, 0, DocumentRange::kFrames); from.endRange(nullptr, 0);
  from.startRange(nullptr, 2, DocumentRange::kFrames); from.endRange(nullptr, 2);

  EXPECT_EQ(frames_range(3, 4),
            copy_range(doc, from, frames_range(3), kDocumentRangeBefore));
  EXPECT_FRAME_COPY2(0, 1, 2, 0, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  EXPECT_EQ(frames_range(4, 5),
            copy_range(doc, from, frames_range(3), kDocumentRangeAfter));
  EXPECT_FRAME_COPY2(0, 1, 2, 3, 0, 2);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  EXPECT_EQ(frames_range(2, 3),
            copy_range(doc, from, frames_range(1), kDocumentRangeAfter));
  EXPECT_FRAME_COPY2(0, 1, 0, 2, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  EXPECT_EQ(frames_range(1, 2),
            copy_range(doc, from, frames_range(1), kDocumentRangeBefore));
  EXPECT_FRAME_COPY2(0, 0, 2, 1, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  // Copy frames 1 and 3...

  from.clearRange();
  from.startRange(nullptr, 1, DocumentRange::kFrames); from.endRange(nullptr, 1);
  from.startRange(nullptr, 3, DocumentRange::kFrames); from.endRange(nullptr, 3);

  EXPECT_EQ(frames_range(0, 1),
            copy_range(doc, from, frames_range(0), kDocumentRangeBefore));
  EXPECT_FRAME_COPY2(1, 3, 0, 1, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  EXPECT_EQ(frames_range(1, 2),
            copy_range(doc, from, frames_range(0), kDocumentRangeAfter));
  EXPECT_FRAME_COPY2(0, 1, 3, 1, 2, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
}

TEST_F(DocRangeOps, CopyCels) {
  // TODO
}

TEST_F(DocRangeOps, ReverseFrames) {
  reverse_frames(doc, frames_range(0, 0));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  reverse_frames(doc, frames_range(1, 1));
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  reverse_frames(doc, frames_range(1, 2));
  EXPECT_FRAME_ORDER(0, 2, 1, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  reverse_frames(doc, frames_range(0, 2));
  EXPECT_FRAME_ORDER(2, 1, 0, 3);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  reverse_frames(doc, frames_range(1, 3));
  EXPECT_FRAME_ORDER(0, 3, 2, 1);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);

  reverse_frames(doc, frames_range(0, 3));
  EXPECT_FRAME_ORDER(3, 2, 1, 0);
  doc->undoHistory()->undo();
  EXPECT_FRAME_ORDER(0, 1, 2, 3);
}

TEST_F(DocRangeOps, ReverseCels) {
  // TODO
}

TEST(DocRangeOps2, DropInsideBugs) {
  TestContextT<app::Context> ctx;
  DocumentPtr doc(static_cast<app::Document*>(ctx.documents().add(4, 4)));
  auto sprite = doc->sprite();
  auto layer1 = dynamic_cast<LayerImage*>(sprite->root()->firstLayer());
  auto layer2 = new LayerGroup(sprite);
  auto layer3 = new LayerGroup(sprite);
  auto layer4 = new LayerGroup(sprite);
  sprite->root()->addLayer(layer2);
  sprite->root()->addLayer(layer3);
  sprite->root()->addLayer(layer4);

  EXPECT_EQ(layer1, sprite->root()->layers()[0]);
  EXPECT_EQ(layer2, sprite->root()->layers()[1]);
  EXPECT_EQ(layer3, sprite->root()->layers()[2]);
  EXPECT_EQ(layer4, sprite->root()->layers()[3]);

  // layer4     layer4
  // layer3 -->   layer1
  // layer2     layer3
  // layer1     layer2
  DocumentRange from, to;
  from.selectLayer(layer1);
  to.selectLayer(layer4);
  move_range(doc, from, to, kDocumentRangeFirstChild);
  EXPECT_EQ(layer2, sprite->root()->layers()[0]);
  EXPECT_EQ(layer3, sprite->root()->layers()[1]);
  EXPECT_EQ(layer4, sprite->root()->layers()[2]);
  EXPECT_EQ(layer1, layer4->layers()[0]);

  // layer4       layer4
  //   layer1 -->   layer1
  // layer3         layer3
  // layer2       layer2
  from.clearRange(); from.selectLayer(layer3);
  to.clearRange(); to.selectLayer(layer1);
  move_range(doc, from, to, kDocumentRangeBefore);
  EXPECT_EQ(layer2, sprite->root()->layers()[0]);
  EXPECT_EQ(layer4, sprite->root()->layers()[1]);
  EXPECT_EQ(layer3, layer4->layers()[0]);
  EXPECT_EQ(layer1, layer4->layers()[1]);

  // layer4       layer4
  //   layer1 -->   layer3
  //   layer3         layer1
  // layer2       layer2
  from.clearRange(); from.selectLayer(layer1);
  to.clearRange(); to.selectLayer(layer3);
  move_range(doc, from, to, kDocumentRangeFirstChild);
  EXPECT_EQ(layer2, sprite->root()->layers()[0]);
  EXPECT_EQ(layer4, sprite->root()->layers()[1]);
  EXPECT_EQ(layer3, layer4->layers()[0]);
  EXPECT_EQ(layer1, layer3->layers()[0]);

  // move layer4 as first child of layer4 (invalid operation)
  from.clearRange(); from.selectLayer(layer4);
  to.clearRange(); to.selectLayer(layer4);
  move_range(doc, from, to, kDocumentRangeFirstChild);
  // Everything is exactly the same (no new undo operation)
  EXPECT_EQ(layer2, sprite->root()->layers()[0]);
  EXPECT_EQ(layer4, sprite->root()->layers()[1]);
  EXPECT_EQ(layer3, layer4->layers()[0]);
  EXPECT_EQ(layer1, layer3->layers()[0]);

  // move layer4 as first child of layer3 (invalid operation)
  from.clearRange(); from.selectLayer(layer4);
  to.clearRange(); to.selectLayer(layer3);
  move_range(doc, from, to, kDocumentRangeFirstChild);
  // Everything is exactly the same (no new undo operation)
  EXPECT_EQ(layer2, sprite->root()->layers()[0]);
  EXPECT_EQ(layer4, sprite->root()->layers()[1]);
  EXPECT_EQ(layer3, layer4->layers()[0]);
  EXPECT_EQ(layer1, layer3->layers()[0]);

  // move layer4 after layer1 (invalid operation)
  from.clearRange(); from.selectLayer(layer4);
  to.clearRange(); to.selectLayer(layer1);
  move_range(doc, from, to, kDocumentRangeAfter);
  // Everything is exactly the same (no new undo operation)
  EXPECT_EQ(layer2, sprite->root()->layers()[0]);
  EXPECT_EQ(layer4, sprite->root()->layers()[1]);
  EXPECT_EQ(layer3, layer4->layers()[0]);
  EXPECT_EQ(layer1, layer3->layers()[0]);

  // move layer4 after layer1 (invalid operation)
  from.clearRange(); from.selectLayer(layer4);
  to.clearRange(); to.selectLayer(layer1);
  move_range(doc, from, to, kDocumentRangeBefore);
  // Everything is exactly the same (no new undo operation)
  EXPECT_EQ(layer2, sprite->root()->layers()[0]);
  EXPECT_EQ(layer4, sprite->root()->layers()[1]);
  EXPECT_EQ(layer3, layer4->layers()[0]);
  EXPECT_EQ(layer1, layer3->layers()[0]);

  // move layer2 inside layer2 (invalid operation)
  from.clearRange(); from.selectLayer(layer2);
  to.clearRange(); to.selectLayer(layer2);
  move_range(doc, from, to, kDocumentRangeFirstChild);
  // Everything is exactly the same (no new undo operation)
  EXPECT_EQ(layer2, sprite->root()->layers()[0]);
  EXPECT_EQ(layer4, sprite->root()->layers()[1]);
  EXPECT_EQ(layer3, layer4->layers()[0]);
  EXPECT_EQ(layer1, layer3->layers()[0]);

  // layer4       layer4
  //   layer1 <--   layer3
  //   layer3         layer1
  // layer2       layer2
  doc->undoHistory()->undo();
  EXPECT_EQ(layer2, sprite->root()->layers()[0]);
  EXPECT_EQ(layer4, sprite->root()->layers()[1]);
  EXPECT_EQ(layer3, layer4->layers()[0]);
  EXPECT_EQ(layer1, layer4->layers()[1]);

  // layer4        layer4
  //   layer1 <--    layer1
  // layer3          layer3
  // layer2        layer2
  doc->undoHistory()->undo();
  EXPECT_EQ(layer2, sprite->root()->layers()[0]);
  EXPECT_EQ(layer3, sprite->root()->layers()[1]);
  EXPECT_EQ(layer4, sprite->root()->layers()[2]);
  EXPECT_EQ(layer1, layer4->layers()[0]);

  // layer4     layer4
  // layer3 <--   layer1
  // layer2     layer3
  // layer1     layer2
  doc->undoHistory()->undo();
  EXPECT_EQ(layer1, sprite->root()->layers()[0]);
  EXPECT_EQ(layer2, sprite->root()->layers()[1]);
  EXPECT_EQ(layer3, sprite->root()->layers()[2]);
  EXPECT_EQ(layer4, sprite->root()->layers()[3]);

  doc->close();
}
