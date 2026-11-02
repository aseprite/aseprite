// Aseprite 2.0
// Copyright (C) 2026 Antigravity / Aseprite 2.0 contributors
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/add_slice.h"
#include "app/cmd/add_tag.h"
#include "app/cmd/set_slice_key.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/doc_api.h"
#include "app/extra_cel.h"
#include "app/file/file.h"
#include "app/file_selector.h"
#include "app/pref/preferences.h"
#include "app/site.h"
#include "app/tx.h"
#include "app/ui/doc_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/standby_state.h"
#include "doc/mask.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/conversion_to_surface.h"
#include "base/fs.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/color.h"
#include "doc/frames_sequence.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/layer_smart_object.h"
#include "doc/primitives.h"
#include "doc/slice.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "doc/tags.h"
#include "fmt/format.h"
#include "os/sampling.h"
#include "os/surface.h"
#include "os/system.h"
#include "render/render.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/graphics.h"
#include "ui/label.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"
#include "ui/scale.h"
#include "ui/separator.h"
#include "ui/size_hint_event.h"
#include "ui/slider.h"
#include "ui/system.h"
#include "ui/timer.h"
#include "ui/view.h"
#include "ui/widget.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace app {

namespace {

enum FaceId : int {
  kFaceTop = 0,
  kFaceDown = 1,
  kFaceFront = 2,
  kFaceBack = 3,
  kFaceRight = 4,
  kFaceLeft = 5,
  kFaceCount = 6
};

enum RenderStyle : int {
  kRenderTexturedLit = 0,
  kRenderTexturedFlat = 1,
  kRenderColoredFaces = 2,
  kRenderNormalShaded = 3,
  kRenderDepthShaded = 4,
  kRenderWireframeTex = 5
};

struct FaceSpec {
  const char* sliceName;
  const char* displayName;
  int gridCol;
  int gridRow;
  uint8_t r, g, b;
  int defaultYaw;
  int defaultPitch;
};

// 4-Column 2-Row layout matching reference UI:
// Col 0: Top (row 0),    Bottom (row 1)
// Col 1: Front (row 0),  Back (row 1)
// Col 2: Right (row 0),  Left (row 1)
// Col 3: Preview (row 0), Fields (row 1)
constexpr std::array<FaceSpec, kFaceCount> kFaceSpecs = {{
  { "TOP",   "Top",    0, 0, 168,  85, 247,  45,  60 },
  { "DOWN",  "Bottom", 0, 1, 244,  63,  94,  45, -60 },
  { "FRONT", "Front",  1, 0,   0, 229, 255,   0,   0 },
  { "BACK",  "Back",   1, 1,  59, 130, 246, 180,   0 },
  { "RIGHT", "Right",  2, 0, 245, 158,  11, -90,   0 },
  { "LEFT",  "Left",   2, 1,  16, 185, 129,  90,   0 },
}};

inline uint8_t clampByte(float v)
{
  if (v <= 0.0f) return 0;
  if (v >= 255.0f) return 255;
  return static_cast<uint8_t>(v + 0.5f);
}

inline float pixelLuma(uint32_t c)
{
  if (doc::rgba_geta(c) <= 16)
    return 0.0f;
  return (0.299f * doc::rgba_getr(c) +
          0.587f * doc::rgba_getg(c) +
          0.114f * doc::rgba_getb(c)) * (1.0f / 255.0f);
}

uint32_t starterSurfacePixel(int face, int u, int v, int W, int H)
{
  const bool isBorder = (u == 0 || v == 0 || u == W - 1 || v == H - 1);
  const bool isInnerBorder = (u == 1 || v == 1 || u == W - 2 || v == H - 2);
  const bool isCorner = ((u <= 2 && v <= 2) || (u >= W - 3 && v <= 2) ||
                         (u <= 2 && v >= H - 3) || (u >= W - 3 && v >= H - 3));
  if (isCorner)
    return doc::rgba(251, 191, 36, 255);
  if (isBorder)
    return doc::rgba(30, 41, 59, 255);
  if (isInnerBorder) {
    const auto& spec = kFaceSpecs[face];
    return doc::rgba(spec.r / 2 + 30, spec.g / 2 + 30, spec.b / 2 + 30, 255);
  }

  const float cx = (W - 1) * 0.5f;
  const float cy = (H - 1) * 0.5f;
  const float dx = u - cx;
  const float dy = v - cy;
  const float dist = std::sqrt(dx * dx + dy * dy);
  const int minDim = std::min(W, H);

  switch (face) {
    case kFaceFront: {
      if (v == 3 && u >= 4 && u <= W - 5)
        return doc::rgba(0, 229, 255, 255);
      if (dist <= 2.3f)
        return doc::rgba(148, 163, 184, 255);
      if (dist <= minDim * 0.30f)
        return doc::rgba(15, 23, 42, 255);
      if (dist <= minDim * 0.36f)
        return doc::rgba(71, 85, 105, 255);
      return (((u + v) & 1) == 0) ? doc::rgba(30, 41, 59, 255) : doc::rgba(51, 65, 85, 255);
    }
    case kFaceBack: {
      if (v >= 4 && v <= 8 && (v & 1) == 0 && u >= 5 && u <= W - 6)
        return doc::rgba(15, 23, 42, 255);
      if (dist <= 2.5f)
        return doc::rgba(239, 68, 68, 255);
      return doc::rgba(45, 55, 72, 255);
    }
    case kFaceLeft:
    case kFaceRight: {
      if (v >= H / 2 - 2 && v <= H / 2 + 2 && u >= 5 && u <= W - 6)
        return doc::rgba(28, 25, 23, 255);
      return (((u + v) % 3) == 0) ? doc::rgba(93, 64, 55, 255) : doc::rgba(78, 52, 46, 255);
    }
    case kFaceTop: {
      if (v >= H / 2 - 1 && v <= H / 2 + 1 && u >= 4 && u <= W - 5)
        return doc::rgba(15, 23, 42, 255);
      if (v >= 3 && v <= 4 && u >= 4 && u <= W - 5 && (u % 3 == 0))
        return doc::rgba(253, 224, 71, 255);
      return doc::rgba(109, 76, 65, 255);
    }
    case kFaceDown:
    default: {
      if ((u >= 3 && u <= 5 && v >= 3 && v <= 5) ||
          (u >= W - 6 && u <= W - 4 && v >= 3 && v <= 5) ||
          (u >= 3 && u <= 5 && v >= H - 6 && v <= H - 4) ||
          (u >= W - 6 && u <= W - 4 && v >= H - 6 && v <= H - 4)) {
        return doc::rgba(15, 23, 42, 255);
      }
      return doc::rgba(51, 65, 85, 255);
    }
  }
}

struct SixSurfaceData {
  int S = 24;
  std::array<int, kFaceCount> w = { 24, 24, 24, 24, 24, 24 };
  std::array<int, kFaceCount> h = { 24, 24, 24, 24, 24, 24 };
  std::array<std::vector<uint32_t>, kFaceCount> pixels;
  std::array<std::vector<uint8_t>, kFaceCount> depthPixels;
  std::array<gfx::Rect, kFaceCount> bounds;
  std::array<bool, 3> colLinked = { false, false, false };
  bool hasSlices = false;
  bool hasCustomDepthLayer = false;

  void init(int faceSize)
  {
    S = std::max(8, faceSize);
    for (int f = 0; f < kFaceCount; ++f) {
      w[f] = S;
      h[f] = S;
      pixels[f].assign(S * S, 0);
      depthPixels[f].assign(S * S, 128);
    }
    colLinked.fill(false);
  }

  void setFaceSize(int face, int fw, int fh)
  {
    fw = std::clamp(fw, 4, 256);
    fh = std::clamp(fh, 4, 256);
    w[face] = fw;
    h[face] = fh;
    pixels[face].assign(fw * fh, 0);
    depthPixels[face].assign(fw * fh, 128);
    S = std::max(S, std::max(fw, fh));
  }

  void propagateDimensionLink(int srcFace, int newW, int newH)
  {
    newW = std::clamp(newW, 4, 256);
    newH = std::clamp(newH, 4, 256);

    int X = w[kFaceFront];
    int Y = h[kFaceFront];
    int Z = w[kFaceRight];

    switch (srcFace) {
      case kFaceTop:
      case kFaceDown:
        X = newW;
        Z = newH;
        break;
      case kFaceFront:
      case kFaceBack:
        X = newW;
        Y = newH;
        break;
      case kFaceRight:
      case kFaceLeft:
        Z = newW;
        Y = newH;
        break;
    }

    setFaceSize(kFaceTop,   X, Z);
    setFaceSize(kFaceDown,  X, Z);
    setFaceSize(kFaceFront, X, Y);
    setFaceSize(kFaceBack,  X, Y);
    setFaceSize(kFaceRight, Z, Y);
    setFaceSize(kFaceLeft,  Z, Y);
  }

  uint32_t sample(int face, int u, int v) const
  {
    const int fw = w[face];
    const int fh = h[face];
    u = std::clamp(u, 0, fw - 1);
    v = std::clamp(v, 0, fh - 1);
    return pixels[face][v * fw + u];
  }

  float sampleDepthNorm(int face, int u, int v) const
  {
    const int fw = w[face];
    const int fh = h[face];
    u = std::clamp(u, 0, fw - 1);
    v = std::clamp(v, 0, fh - 1);
    if (hasCustomDepthLayer) {
      return float(depthPixels[face][v * fw + u]) * (1.0f / 255.0f);
    }
    return pixelLuma(pixels[face][v * fw + u]);
  }

  bool hasPixels(int face) const
  {
    if (face < 0 || face >= kFaceCount) return false;
    const auto& px = pixels[face];
    for (uint32_t c : px) {
      if (doc::rgba_geta(c) > 16)
        return true;
    }
    return false;
  }

  int symmetricFace(int face) const
  {
    if (hasPixels(face)) return face;
    switch (face) {
      case kFaceTop:   if (hasPixels(kFaceDown)) return kFaceDown; break;
      case kFaceDown:  if (hasPixels(kFaceTop))  return kFaceTop; break;
      case kFaceFront: if (hasPixels(kFaceBack)) return kFaceBack; break;
      case kFaceBack:  if (hasPixels(kFaceFront)) return kFaceFront; break;
      case kFaceRight: if (hasPixels(kFaceLeft))  return kFaceLeft; break;
      case kFaceLeft:  if (hasPixels(kFaceRight)) return kFaceRight; break;
    }
    return face;
  }

  uint32_t sampleSymmetric(int face, int u, int v) const
  {
    const int sf = symmetricFace(face);
    return sample(sf, u, v);
  }

  bool isFaceSolid(int face, int u, int v) const
  {
    return doc::rgba_geta(sampleSymmetric(face, u, v)) > 16;
  }
};

struct SculptedVoxelVolume {
  int vx = 24;
  int vy = 24;
  int vz = 24;
  std::vector<uint8_t> solid;

  bool isSolid(int x, int y, int z) const
  {
    if (x < 0 || x >= vx || y < 0 || y >= vy || z < 0 || z >= vz)
      return false;
    return solid[(y * vz + z) * vx + x] != 0;
  }

  void setSolid(int x, int y, int z, bool val)
  {
    if (x >= 0 && x < vx && y >= 0 && y < vy && z >= 0 && z < vz)
      solid[(y * vz + z) * vx + x] = (val ? 1 : 0);
  }
};

static SculptedVoxelVolume buildSculptedVolume(const SixSurfaceData& data,
                                              int pivX, int pivY, int pivZ)
{
  SculptedVoxelVolume vol;
  const int dimX = std::max({ data.w[kFaceFront], data.w[kFaceBack], data.w[kFaceTop], data.w[kFaceDown], 8 });
  const int dimY = std::max({ data.h[kFaceFront], data.h[kFaceBack], data.h[kFaceLeft], data.h[kFaceRight], 8 });
  const int dimZ = std::max({ data.w[kFaceLeft], data.w[kFaceRight], data.h[kFaceTop], data.h[kFaceDown], 8 });

  vol.vx = std::clamp(dimX, 8, 64);
  vol.vy = std::clamp(dimY, 8, 64);
  vol.vz = std::clamp(dimZ, 8, 64);
  vol.solid.assign(vol.vx * vol.vy * vol.vz, 0);

  const bool hasTop   = data.hasPixels(kFaceTop);
  const bool hasDown  = data.hasPixels(kFaceDown);
  const bool hasFront = data.hasPixels(kFaceFront);
  const bool hasBack  = data.hasPixels(kFaceBack);
  const bool hasRight = data.hasPixels(kFaceRight);
  const bool hasLeft  = data.hasPixels(kFaceLeft);

  const bool activeTopDown   = (hasTop || hasDown);
  const bool activeFrontBack = (hasFront || hasBack);
  const bool activeRightLeft = (hasRight || hasLeft);

  if (!activeTopDown && !activeFrontBack && !activeRightLeft) {
    vol.solid.assign(vol.vx * vol.vy * vol.vz, 1);
    return vol;
  }

  const int pivIndexX = std::clamp(int(pivX * 0.01f * (vol.vx - 1)), 0, vol.vx - 1);
  const int pivIndexY = std::clamp(int(pivY * 0.01f * (vol.vy - 1)), 0, vol.vy - 1);
  const int pivIndexZ = std::clamp(int(pivZ * 0.01f * (vol.vz - 1)), 0, vol.vz - 1);

  const bool onlyTopDown = activeTopDown && !activeFrontBack && !activeRightLeft;
  const bool onlyFrontBack = activeFrontBack && !activeTopDown && !activeRightLeft;
  const bool onlyRightLeft = activeRightLeft && !activeTopDown && !activeFrontBack;

  const int slabYMin = std::max(0, pivIndexY - 1);
  const int slabYMax = std::min(vol.vy - 1, pivIndexY + 1);

  const int slabZMin = std::max(0, pivIndexZ - 1);
  const int slabZMax = std::min(vol.vz - 1, pivIndexZ + 1);

  const int slabXMin = std::max(0, pivIndexX - 1);
  const int slabXMax = std::min(vol.vx - 1, pivIndexX + 1);

  for (int y = 0; y < vol.vy; ++y) {
    if (onlyTopDown && (y < slabYMin || y > slabYMax))
      continue;

    for (int z = 0; z < vol.vz; ++z) {
      if (onlyFrontBack && (z < slabZMin || z > slabZMax))
        continue;

      for (int x = 0; x < vol.vx; ++x) {
        if (onlyRightLeft && (x < slabXMin || x > slabXMax))
          continue;

        bool solid = true;

        if (activeTopDown) {
          const int uTop = std::clamp(int((float(x) + 0.5f) / float(vol.vx) * data.w[kFaceTop]), 0, data.w[kFaceTop] - 1);
          const int vTop = std::clamp(int((float(z) + 0.5f) / float(vol.vz) * data.h[kFaceTop]), 0, data.h[kFaceTop] - 1);
          bool topSolid = data.isFaceSolid(kFaceTop, uTop, vTop);

          if (!data.colLinked[0] && hasDown) {
            const int uDown = std::clamp(int((float(x) + 0.5f) / float(vol.vx) * data.w[kFaceDown]), 0, data.w[kFaceDown] - 1);
            const int vDown = std::clamp(int((float(vol.vz - 1 - z) + 0.5f) / float(vol.vz) * data.h[kFaceDown]), 0, data.h[kFaceDown] - 1);
            bool downSolid = data.isFaceSolid(kFaceDown, uDown, vDown);
            topSolid = (hasTop ? (topSolid || downSolid) : downSolid);
          }
          if (!topSolid) {
            solid = false;
          }
        }

        if (solid && activeFrontBack) {
          const int uFront = std::clamp(int((float(x) + 0.5f) / float(vol.vx) * data.w[kFaceFront]), 0, data.w[kFaceFront] - 1);
          const int vFront = std::clamp(int((float(y) + 0.5f) / float(vol.vy) * data.h[kFaceFront]), 0, data.h[kFaceFront] - 1);
          bool frontSolid = data.isFaceSolid(kFaceFront, uFront, vFront);

          if (!data.colLinked[1] && hasBack) {
            const int uBack = std::clamp(int((float(vol.vx - 1 - x) + 0.5f) / float(vol.vx) * data.w[kFaceBack]), 0, data.w[kFaceBack] - 1);
            const int vBack = std::clamp(int((float(y) + 0.5f) / float(vol.vy) * data.h[kFaceBack]), 0, data.h[kFaceBack] - 1);
            bool backSolid = data.isFaceSolid(kFaceBack, uBack, vBack);
            frontSolid = (hasFront ? (frontSolid || backSolid) : backSolid);
          }
          if (!frontSolid) {
            solid = false;
          }
        }

        if (solid && activeRightLeft) {
          const int uRight = std::clamp(int((float(vol.vz - 1 - z) + 0.5f) / float(vol.vz) * data.w[kFaceRight]), 0, data.w[kFaceRight] - 1);
          const int vRight = std::clamp(int((float(y) + 0.5f) / float(vol.vy) * data.h[kFaceRight]), 0, data.h[kFaceRight] - 1);
          bool sideSolid = data.isFaceSolid(kFaceRight, uRight, vRight);

          if (!data.colLinked[2] && hasLeft) {
            const int uLeft = std::clamp(int((float(z) + 0.5f) / float(vol.vz) * data.w[kFaceLeft]), 0, data.w[kFaceLeft] - 1);
            const int vLeft = std::clamp(int((float(y) + 0.5f) / float(vol.vy) * data.h[kFaceLeft]), 0, data.h[kFaceLeft] - 1);
            bool leftSolid = data.isFaceSolid(kFaceLeft, uLeft, vLeft);
            sideSolid = (hasRight ? (sideSolid || leftSolid) : leftSolid);
          }
          if (!sideSolid) {
            solid = false;
          }
        }

        if (solid) {
          vol.setSolid(x, y, z, true);
        }
      }
    }
  }

  return vol;
}

static int matchFaceIndex(const std::string& name)
{
  if (name.empty()) return -1;
  std::string lower;
  lower.reserve(name.size());
  for (char c : name)
    lower.push_back(std::tolower(static_cast<unsigned char>(c)));

  if (lower.rfind("surface_", 0) == 0)
    lower = lower.substr(8);
  else if (lower.rfind("face_", 0) == 0)
    lower = lower.substr(5);

  if (lower == "top") return kFaceTop;
  if (lower == "down" || lower == "bottom") return kFaceDown;
  if (lower == "front") return kFaceFront;
  if (lower == "back") return kFaceBack;
  if (lower == "right") return kFaceRight;
  if (lower == "left") return kFaceLeft;
  return -1;
}

static doc::Slice* findSliceForFace(doc::Sprite* spr, int face)
{
  if (!spr || face < 0 || face >= kFaceCount)
    return nullptr;
  for (auto* slice : spr->slices()) {
    if (slice && matchFaceIndex(slice->name()) == face) {
      return slice;
    }
  }
  return nullptr;
}

static doc::LayerImage* findLayerByName(doc::Sprite* spr, const std::string& name)
{
  if (!spr)
    return nullptr;
  for (doc::Layer* lyr : spr->allLayers()) {
    if (lyr->isImage() && lyr->name() == name)
      return static_cast<doc::LayerImage*>(lyr);
  }
  return nullptr;
}

static doc::LayerImage* findOrCreateLayer(app::Doc* doc,
                                          app::DocApi& api,
                                          doc::Sprite* spr,
                                          const std::string& name)
{
  if (auto* existing = findLayerByName(spr, name))
    return existing;
  return api.newLayer(spr->root(), name);
}

struct SurfaceLayerPair {
  doc::LayerImage* baseLayer = nullptr;
  doc::LayerImage* texLayer = nullptr;
};

static SurfaceLayerPair findSurfaceLayers(doc::Sprite* spr, int face)
{
  SurfaceLayerPair pair;
  if (!spr || face < 0 || face >= kFaceCount)
    return pair;

  const std::string faceName = kFaceSpecs[face].sliceName;
  const std::string baseName = "Surface_" + faceName + "_Base";
  const std::string texName = "Surface_" + faceName + "_Texture";
  const std::string defaultName = "Surface_" + faceName;
  const std::string groupName = "Surface_" + faceName + "_Group";

  // 1. Check if there's a group named Surface_<Face>, Surface_<Face>_Group, or Surface_<Face>_Texture (unpacked SO)
  for (doc::Layer* lyr : spr->allLayers()) {
    if (lyr->isGroup() && (lyr->name() == defaultName || lyr->name() == groupName || lyr->name() == texName ||
                           (face == kFaceDown && (lyr->name() == "Surface_BOTTOM" || lyr->name() == "Surface_Bottom" || lyr->name() == "Surface_BOTTOM_Group" || lyr->name() == "Surface_Bottom_Group")))) {
      doc::LayerGroup* grp = static_cast<doc::LayerGroup*>(lyr);
      for (doc::Layer* child : grp->layers()) {
        if (!child->isImage()) continue;
        doc::LayerImage* childImg = static_cast<doc::LayerImage*>(child);
        const std::string& cn = child->name();
        if (cn == "Texture" || cn == "texture" || cn == texName || child->isSmartObject()) {
          if (!pair.texLayer) pair.texLayer = childImg;
        }
        else if (cn == "Base" || cn == "base" || cn == baseName || !pair.baseLayer) {
          if (!pair.baseLayer) pair.baseLayer = childImg;
        }
      }
      if (pair.baseLayer && pair.texLayer)
        return pair;
    }
  }

  // 2. Check flat layers: Surface_<Face>_Base and Surface_<Face>_Texture
  if (!pair.baseLayer) {
    pair.baseLayer = findLayerByName(spr, baseName);
    if (!pair.baseLayer && face == kFaceDown) {
      pair.baseLayer = findLayerByName(spr, "Surface_BOTTOM_Base");
      if (!pair.baseLayer)
        pair.baseLayer = findLayerByName(spr, "Surface_Bottom_Base");
    }
  }
  if (!pair.texLayer) {
    pair.texLayer = findLayerByName(spr, texName);
    if (!pair.texLayer && face == kFaceDown) {
      pair.texLayer = findLayerByName(spr, "Surface_BOTTOM_Texture");
      if (!pair.texLayer)
        pair.texLayer = findLayerByName(spr, "Surface_Bottom_Texture");
    }
  }

  // 3. Fallback: if no base layer found, check Surface_<Face>
  if (!pair.baseLayer) {
    pair.baseLayer = findLayerByName(spr, defaultName);
    if (!pair.baseLayer && face == kFaceDown) {
      pair.baseLayer = findLayerByName(spr, "Surface_BOTTOM");
      if (!pair.baseLayer)
        pair.baseLayer = findLayerByName(spr, "Surface_Bottom");
    }
  }

  return pair;
}


// ---------------------------------------------------------------------------
// Shared UV sheets: one Base, one Texture and one Depth layer for all faces.
// ---------------------------------------------------------------------------
// Before this change every face had its own Base/Texture layers
// (Surface_Top_Base, Surface_Front_Texture, ...). Now Easymetric keeps
// exactly three image layers:
//
//   Surface_Base     - one sheet, every face paints into its own UV rect
//   Surface_Texture  - one sheet, same UV rects as Base
//   Surface_DEPTH    - one sheet, same UV rects as Base (already shared
//                      by name, but previously written at (0,0) for
//                      every face, so faces overwrote each other)
//
// A face's UV rect is its grid cell in the sheet: FaceSpec already
// carries gridCol/gridRow for the studio layout (Top/Bottom = col 0,
// Front/Back = col 1, Right/Left = col 2; top faces = row 0, bottom
// faces = row 1), so the UV layout is the same 3x2 grid the user sees.
// Each surface panel is then just a window onto its UV rect of the
// shared layers (Editor::setClipSpriteBounds), and the 3D preview
// samples each face from its own rect instead of from its own layer.
// Clicking a panel therefore no longer switches layers at all: it
// switches the UV rect being edited, while the active layer stays
// on the same shared sheet. This is the "single sheet of paper,
// UV-mapped" model: the sheet is not copied per surface, the model
// cuts out the rectangle it needs for each surface.
// ---------------------------------------------------------------------------
struct SharedSurfaceLayers {
  doc::LayerImage* baseLayer = nullptr;
  doc::LayerImage* textureLayer = nullptr;
  doc::LayerImage* depthLayer = nullptr;
};

static SharedSurfaceLayers findSharedLayers(doc::Sprite* spr)
{
  SharedSurfaceLayers shared;
  if (!spr) return shared;
  shared.baseLayer = findLayerByName(spr, "Surface_Base");
  shared.textureLayer = findLayerByName(spr, "Surface_Texture");
  shared.depthLayer = findLayerByName(spr, "Surface_DEPTH");
  return shared;
}

// Exact UV layout: the canvas (sheet) size is derived from the
// UV mapping, not the other way round. A column is exactly as
// wide as its widest face, a row exactly as tall as its tallest
// face, and the sheet is the sum of the columns/rows. Faces of
// different sizes therefore do NOT force every cell to the
// largest face size (the old uniform 3 x cellW, 2 x cellH grid
// padded every smaller face and, worse, derived cell origins
// from the sprite size, so a manual canvas resize shifted every
// UV rect without changing any face size). The only unavoidable
// padding is where two faces in one column/row differ in size:
// the smaller face's rect is its own exact size, the rest of
// its column/row belongs to its partner face.
// Layout: Top/Front/Right = row 0, Bottom/Back/Left = row 1,
// columns Top/Bottom, Front/Back, Right/Left (kFaceSpecs).
static void sharedFaceSizes(doc::Sprite* spr, int w[kFaceCount], int h[kFaceCount])
{
  const int fallbackW = spr ? std::max(1, spr->width() / 3) : 24;
  const int fallbackH = spr ? std::max(1, spr->height() / 2) : 24;
  for (int f = 0; f < kFaceCount; ++f) {
    w[f] = fallbackW;
    h[f] = fallbackH;
    if (spr) {
      if (doc::Slice* sl = findSliceForFace(spr, f)) {
        if (const auto* key = sl->getByFrame(0)) {
          if (!key->bounds().isEmpty()) {
            w[f] = key->bounds().w;
            h[f] = key->bounds().h;
          }
        }
      }
    }
  }
}

static void sharedLayout(const int w[kFaceCount], const int h[kFaceCount],
                         int colW[3], int rowH[2])
{
  for (int c = 0; c < 3; ++c) colW[c] = 1;
  for (int r = 0; r < 2; ++r) rowH[r] = 1;
  for (int f = 0; f < kFaceCount; ++f) {
    colW[kFaceSpecs[f].gridCol] = std::max(colW[kFaceSpecs[f].gridCol], w[f]);
    rowH[kFaceSpecs[f].gridRow] = std::max(rowH[kFaceSpecs[f].gridRow], h[f]);
  }
}

// UV rect of one face inside the exact layout above.
static gfx::Rect sharedUVRectExact(int face, const int colW[3], const int rowH[2],
                                   int faceW, int faceH)
{
  if (face < 0 || face >= kFaceCount) return gfx::Rect(0, 0, faceW, faceH);
  int x = 0, y = 0;
  for (int c = 0; c < kFaceSpecs[face].gridCol; ++c) x += colW[c];
  for (int r = 0; r < kFaceSpecs[face].gridRow; ++r) y += rowH[r];
  return gfx::Rect(x, y, faceW, faceH);
}

// UV rect for a face in the current document, from its slice size
// and the exact layout (not from the sprite size). This also
// repairs documents whose slices were left at (0, 0) by the old
// per-face layout: every face would otherwise sample the same
// top-left cell of the shared sheet.
static gfx::Rect sharedFaceRect(doc::Sprite* spr, int face)
{
  if (!spr || face < 0 || face >= kFaceCount)
    return gfx::Rect(0, 0, 0, 0);
  int w[kFaceCount], h[kFaceCount];
  sharedFaceSizes(spr, w, h);
  int colW[3], rowH[2];
  sharedLayout(w, h, colW, rowH);
  return sharedUVRectExact(face, colW, rowH, w[face], h[face]);
}

// Exact sheet size for the current face sizes (0, 0 if no sprite).
static gfx::Size sharedSheetSize(doc::Sprite* spr)
{
  if (!spr) return gfx::Size(0, 0);
  int w[kFaceCount], h[kFaceCount];
  sharedFaceSizes(spr, w, h);
  int colW[3], rowH[2];
  sharedLayout(w, h, colW, rowH);
  return gfx::Size(colW[0] + colW[1] + colW[2], rowH[0] + rowH[1]);
}

// Rebuild the three shared sheets from their old UV rects
// (taken from the current slice bounds, which are the rects the
// pixels actually sit in today) into new UV rects of the given
// new face sizes, and set the sprite to the exact sheet size
// for those faces. Face pixels are copied, never scaled: a
// larger new rect gets transparent padding, a smaller one is
// cropped at its bottom-right, exactly like a canvas resize.
// Used both for a face resize and for reconciling a manual
// canvas resize (see pollAndRender in the studio view).
static void rebuildSharedSheets(Doc* doc, const int newW[kFaceCount],
                                const int newH[kFaceCount])
{
  if (!doc || !doc->sprite()) return;
  doc::Sprite* spr = doc->sprite();
  SharedSurfaceLayers shared = findSharedLayers(spr);
  if (!shared.baseLayer && !shared.textureLayer) return;

  int oldW[kFaceCount], oldH[kFaceCount];
  sharedFaceSizes(spr, oldW, oldH);
  gfx::Rect oldRect[kFaceCount];
  for (int f = 0; f < kFaceCount; ++f) {
    oldRect[f] = gfx::Rect(0, 0, oldW[f], oldH[f]);
    if (doc::Slice* sl = findSliceForFace(spr, f)) {
      if (const auto* key = sl->getByFrame(0)) {
        if (!key->bounds().isEmpty())
          oldRect[f] = key->bounds();
      }
    }
  }
  int newColW[3], newRowH[2];
  sharedLayout(newW, newH, newColW, newRowH);
  const int newSheetW = newColW[0] + newColW[1] + newColW[2];
  const int newSheetH = newRowH[0] + newRowH[1];

  try {
    Tx tx(doc, "Resize Easymetric UV Sheet", ModifyDocument);
    DocApi api = doc->getApi(tx);
    auto rebuildSheet = [&](doc::LayerImage* lyr) {
      if (!lyr) return;
      for (doc::frame_t fr = doc::frame_t(0); fr <= spr->lastFrame(); ++fr) {
        doc::Cel* cel = lyr->cel(fr);
        if (!cel || !cel->image()) continue;
        doc::ImageRef resized(doc::Image::create(cel->image()->pixelFormat(),
                                                 newSheetW, newSheetH));
        resized->clear(0);
        for (int f = 0; f < kFaceCount; ++f) {
          const gfx::Rect newUv = sharedUVRectExact(f, newColW, newRowH,
                                                    newW[f], newH[f]);
          const int copyW = std::min(oldRect[f].w, newUv.w);
          const int copyH = std::min(oldRect[f].h, newUv.h);
          for (int v = 0; v < copyH; ++v) {
            const int srcY = oldRect[f].y + v;
            if (srcY < 0 || srcY >= cel->image()->height()) continue;
            for (int u = 0; u < copyW; ++u) {
              const int srcX = oldRect[f].x + u;
              if (srcX < 0 || srcX >= cel->image()->width()) continue;
              doc::put_pixel(resized.get(), newUv.x + u, newUv.y + v,
                             doc::get_pixel(cel->image(), srcX, srcY));
            }
          }
        }
        api.replaceImage(spr, cel->imageRef(), resized);
        api.setCelPosition(spr, cel, 0, 0);
      }
    };
    rebuildSheet(shared.baseLayer);
    rebuildSheet(shared.textureLayer);
    rebuildSheet(shared.depthLayer);
    api.setSpriteSize(spr, newSheetW, newSheetH);
    for (int f = 0; f < kFaceCount; ++f) {
      const gfx::Rect uv = sharedUVRectExact(f, newColW, newRowH, newW[f], newH[f]);
      if (doc::Slice* sl = findSliceForFace(spr, f))
        tx(new cmd::SetSliceKey(sl, 0, doc::SliceKey(uv)));
    }
    tx.commit();
    doc->notifyGeneralUpdate();
  }
  catch (const std::exception& e) {
    // Ignore: entering the studio will retry.
  }
}

// Migrate a document from per-face layers to the three shared sheets.
// Old per-face Base/Texture images are copied into their UV cells,
// the old per-face layers/groups are removed, slices are moved to
// their UV rects, and the sprite is resized to the sheet size.
// A document that already has the shared sheets is left alone, and
// a document with no Easymetric layers at all gets starter sheets.
// The old depth sheet cannot be migrated per face (it was written
// at (0, 0) for every face), so it is recreated blank; use
// "Render Editable Face Depths" to regenerate it from the UV cells.
static void setupSharedSurfaceLayers(Doc* doc, bool seedStarterIfBlank)
{
  if (!doc || !doc->sprite()) return;
  doc::Sprite* spr = doc->sprite();

  // Already shared: nothing to migrate, just make sure the slices
  // point at the UV rects. If the canvas was resized by hand
  // since the sheets were built, the sprite size no longer
  // matches the exact sheet size for the face sizes, so rebuild
  // the sheets to that exact size first (face pixels preserved,
  // see rebuildSharedSheets) instead of leaving padding/gaps
  // between the canvas and the UV mapping.
  SharedSurfaceLayers existing = findSharedLayers(spr);
  if (existing.baseLayer && existing.textureLayer) {
    const gfx::Size exact = sharedSheetSize(spr);
    if (exact.w > 0 && (spr->width() != exact.w || spr->height() != exact.h)) {
      int w[kFaceCount], h[kFaceCount];
      sharedFaceSizes(spr, w, h);
      rebuildSharedSheets(doc, w, h);
    }
    try {
      Tx tx(doc, "Setup Shared Easymetric Layers", ModifyDocument);
      for (int f = 0; f < kFaceCount; ++f) {
        const gfx::Rect uv = sharedFaceRect(spr, f);
        doc::Slice* sl = findSliceForFace(spr, f);
        if (!sl) {
          sl = new doc::Slice;
          sl->setName(kFaceSpecs[f].sliceName);
          sl->userData().setColor(
            doc::rgba(kFaceSpecs[f].r, kFaceSpecs[f].g, kFaceSpecs[f].b, 255));
          sl->insert(0, doc::SliceKey(uv));
          tx(new cmd::AddSlice(spr, sl));
        }
        else {
          tx(new cmd::SetSliceKey(sl, 0, doc::SliceKey(uv)));
        }
      }
      tx.commit();
      doc->notifyGeneralUpdate();
    }
    catch (const std::exception& e) {
      // Ignore: the shared layers themselves are already usable.
    }
    return;
  }

  // Capture the old per-face images before any layer is removed.
  struct FaceImage {
    doc::ImageRef baseImg;
    doc::ImageRef texImg;
    int w = 0;
    int h = 0;
  };
  std::array<FaceImage, kFaceCount> faces;
  // Do not start from the sprite size: after a partial migration the
  // sprite may already be sheet-sized, while the surviving per-face
  // images are still face-sized. Face sizes win; the sprite size is
  // only the fallback for a brand-new, empty document.
  int cellW = 0;
  int cellH = 0;
  for (int f = 0; f < kFaceCount; ++f) {
    SurfaceLayerPair sp = findSurfaceLayers(spr, f);
    auto capture = [](doc::LayerImage* lyr) -> doc::ImageRef {
      if (!lyr) return doc::ImageRef();
      if (doc::Cel* cel = lyr->cel(0)) {
        if (cel->image()) return doc::ImageRef(doc::Image::createCopy(cel->image()));
      }
      return doc::ImageRef();
    };
    faces[f].baseImg = capture(sp.baseLayer);
    faces[f].texImg = capture(sp.texLayer);
    const doc::Image* anyImg = faces[f].baseImg ? faces[f].baseImg.get() :
                               (faces[f].texImg ? faces[f].texImg.get() : nullptr);
    if (anyImg) {
      faces[f].w = anyImg->width();
      faces[f].h = anyImg->height();
    }
    else if (doc::Slice* sl = findSliceForFace(spr, f)) {
      if (const auto* key = sl->getByFrame(0)) {
        if (!key->bounds().isEmpty()) {
          faces[f].w = key->bounds().w;
          faces[f].h = key->bounds().h;
        }
      }
    }
    if (faces[f].w <= 0) faces[f].w = spr->width();
    if (faces[f].h <= 0) faces[f].h = spr->height();
    cellW = std::max(cellW, faces[f].w);
    cellH = std::max(cellH, faces[f].h);
  }
  if (cellW <= 0) cellW = spr->width();
  if (cellH <= 0) cellH = spr->height();
  // Exact layout, not a uniform grid: each column/row is sized
  // by its own faces, so the sheet is exactly the UV mapping.
  int faceW[kFaceCount], faceH[kFaceCount];
  for (int f = 0; f < kFaceCount; ++f) {
    faceW[f] = faces[f].w;
    faceH[f] = faces[f].h;
  }
  int colW[3], rowH[2];
  sharedLayout(faceW, faceH, colW, rowH);
  const int sheetW = colW[0] + colW[1] + colW[2];
  const int sheetH = rowH[0] + rowH[1];

  // Layers/groups to remove after their images have been copied.
  // The shared depth layer itself is kept: it is recreated below.
  std::vector<doc::Layer*> layersToRemove;
  for (int f = 0; f < kFaceCount; ++f) {
    const std::string faceName = kFaceSpecs[f].sliceName;
    SurfaceLayerPair sp = findSurfaceLayers(spr, f);
    if (sp.baseLayer) layersToRemove.push_back(sp.baseLayer);
    if (sp.texLayer) layersToRemove.push_back(sp.texLayer);
    for (doc::Layer* lyr : spr->allLayers()) {
      if (lyr->name() == ("Surface_" + faceName) ||
          lyr->name() == ("Surface_" + faceName + "_Group")) {
        layersToRemove.push_back(lyr);
      }
    }
  }

  try {
    Tx tx(doc, "Setup Shared Easymetric Layers (UV Sheets)", ModifyDocument);
    DocApi api = doc->getApi(tx);

    api.setSpriteSize(spr, sheetW, sheetH);

    auto buildSheet = [&](const char* kind,
                          doc::ImageRef FaceImage::*img) {
      doc::ImageRef sheet(doc::Image::create(spr->pixelFormat(), sheetW, sheetH));
      sheet->clear(0);
      bool hasPixels = false;
      for (int ff = 0; ff < kFaceCount; ++ff) {
        const doc::ImageRef& src = faces[ff].*img;
        if (!src) continue;
        const gfx::Rect uv = sharedUVRectExact(ff, colW, rowH, faces[ff].w, faces[ff].h);
        doc::copy_image(sheet.get(), src.get(), uv.x, uv.y);
        hasPixels = true;
      }
      if (!hasPixels && seedStarterIfBlank && std::string(kind) == "base") {
        for (int ff = 0; ff < kFaceCount; ++ff) {
          const gfx::Rect uv = sharedUVRectExact(ff, colW, rowH, faces[ff].w, faces[ff].h);
          for (int v = 0; v < faces[ff].h; ++v)
            for (int u = 0; u < faces[ff].w; ++u)
              doc::put_pixel(sheet.get(), uv.x + u, uv.y + v,
                             starterSurfacePixel(ff, u, v, faces[ff].w, faces[ff].h));
        }
      }
      return sheet;
    };

    struct SheetLayer {
      const char* name;
      doc::ImageRef img;
    };
    std::vector<SheetLayer> sheets;
    sheets.push_back({ "Surface_Base", buildSheet("base", &FaceImage::baseImg) });
    sheets.push_back({ "Surface_Texture", buildSheet("texture", &FaceImage::texImg) });
    // Depth is recreated blank (see comment above); keep the layer
    // so "Render Editable Face Depths" has somewhere to write to.
    {
      doc::ImageRef depthSheet(doc::Image::create(spr->pixelFormat(), sheetW, sheetH));
      depthSheet->clear(0);
      sheets.push_back({ "Surface_DEPTH", depthSheet });
    }

    for (const auto& sheetLayer : sheets) {
      doc::LayerImage* lyr = findOrCreateLayer(doc, api, spr, sheetLayer.name);
      if (doc::Cel* cel = lyr->cel(0)) {
        api.replaceImage(spr, cel->imageRef(), sheetLayer.img);
        api.setCelPosition(spr, cel, 0, 0);
      }
      else {
        api.addCel(lyr, doc::frame_t(0), sheetLayer.img);
      }
    }

    for (int f = 0; f < kFaceCount; ++f) {
      const gfx::Rect uv = sharedUVRectExact(f, colW, rowH, faces[f].w, faces[f].h);
      doc::Slice* sl = findSliceForFace(spr, f);
      if (!sl) {
        sl = new doc::Slice;
        sl->setName(kFaceSpecs[f].sliceName);
        sl->userData().setColor(
          doc::rgba(kFaceSpecs[f].r, kFaceSpecs[f].g, kFaceSpecs[f].b, 255));
        sl->insert(0, doc::SliceKey(uv));
        tx(new cmd::AddSlice(spr, sl));
      }
      else {
        tx(new cmd::SetSliceKey(sl, 0, doc::SliceKey(uv)));
      }
    }

    // Remove the old per-face layers/groups last, so a failure
    // above does not leave a document with neither old nor new.
    // Image layers first (groups become empty), then groups.
    std::sort(layersToRemove.begin(), layersToRemove.end(),
              [](doc::Layer* a, doc::Layer* b) {
                return a->isImage() && !b->isImage();
              });
    for (doc::Layer* lyr : layersToRemove) {
      // Skip anything that is already gone, or a shared layer.
      bool stillThere = false;
      for (doc::Layer* l : spr->allLayers())
        if (l == lyr) { stillThere = true; break; }
      if (!stillThere) continue;
      if (lyr->name() == "Surface_Base" || lyr->name() == "Surface_Texture" ||
          lyr->name() == "Surface_DEPTH")
        continue;
      api.removeLayer(lyr);
    }

    tx.commit();
    doc->notifyGeneralUpdate();
  }
  catch (const std::exception& e) {
    // Ignore: entering the studio will retry the migration.
  }
}

static void propagateLinkedDimensions(Doc* doc, SixSurfaceData& data, int srcFace, int newW, int newH)
{
  if (!doc || !doc->sprite()) return;
  doc::Sprite* spr = doc->sprite();

  // Shared sheets, exact layout: resizing one face changes
  // only that face's size. Its column/row may grow or shrink,
  // so the other faces in that column/row move to their new
  // UV origins, but their sizes and pixels do not change.
  // That is the independence the old uniform grid broke: it
  // forced every face to the resized face's size. The sheet
  // (canvas) becomes exactly the sum of the new column/row
  // sizes, via rebuildSharedSheets.
  {
    SharedSurfaceLayers shared = findSharedLayers(spr);
    if (shared.baseLayer || shared.textureLayer) {
      int targetW[kFaceCount], targetH[kFaceCount];
      for (int f = 0; f < kFaceCount; ++f) {
        targetW[f] = data.w[f];
        targetH[f] = data.h[f];
      }
      if (srcFace >= 0 && srcFace < kFaceCount) {
        targetW[srcFace] = std::clamp(newW, 4, 256);
        targetH[srcFace] = std::clamp(newH, 4, 256);
      }
      int oldW[kFaceCount], oldH[kFaceCount];
      sharedFaceSizes(spr, oldW, oldH);
      const gfx::Size exact = sharedSheetSize(spr);
      bool changed = false;
      for (int f = 0; f < kFaceCount; ++f)
        if (targetW[f] != oldW[f] || targetH[f] != oldH[f]) changed = true;
      if (spr->width() != exact.w || spr->height() != exact.h) changed = true;
      if (changed) {
        rebuildSharedSheets(doc, targetW, targetH);
        for (int f = 0; f < kFaceCount; ++f) {
          data.w[f] = targetW[f];
          data.h[f] = targetH[f];
        }
      }
      return;
    }
  }

  data.propagateDimensionLink(srcFace, newW, newH);

  try {
    Tx tx(doc, "Resize Easymetric Surface Dimensions", ModifyDocument);
    DocApi api = doc->getApi(tx);

    int maxW = spr->width();
    int maxH = spr->height();
    for (int f = 0; f < kFaceCount; ++f) {
      if (doc::Slice* sl = findSliceForFace(spr, f)) {
        for (auto it = sl->begin(); it != sl->end(); ++it) {
          maxW = std::max(maxW, it->value()->bounds().x + data.w[f]);
          maxH = std::max(maxH, it->value()->bounds().y + data.h[f]);
        }
      }
      maxW = std::max(maxW, data.w[f]);
      maxH = std::max(maxH, data.h[f]);
    }
    if (maxW > spr->width() || maxH > spr->height()) {
      api.setSpriteSize(spr, maxW, maxH);
    }

    std::set<doc::ObjectId> resizedImages;

    for (int f = 0; f < kFaceCount; ++f) {
      const int targetW = data.w[f];
      const int targetH = data.h[f];

      // Update slice bounds for all keys in slice
      doc::Slice* sl = findSliceForFace(spr, f);
      if (!sl) {
        sl = new doc::Slice;
        sl->setName(kFaceSpecs[f].sliceName);
        sl->userData().setColor(
          doc::rgba(kFaceSpecs[f].r, kFaceSpecs[f].g, kFaceSpecs[f].b, 255));
        sl->insert(0, doc::SliceKey(gfx::Rect(0, 0, targetW, targetH)));
        tx(new cmd::AddSlice(spr, sl));
      }
      else if (sl->empty()) {
        tx(new cmd::SetSliceKey(sl, 0, doc::SliceKey(gfx::Rect(0, 0, targetW, targetH))));
      }
      else {
        for (auto it = sl->begin(); it != sl->end(); ++it) {
          gfx::Rect curBounds = it->value()->bounds();
          curBounds.w = targetW;
          curBounds.h = targetH;
          tx(new cmd::SetSliceKey(sl, it->frame(),
            doc::SliceKey(curBounds, it->value()->center(), it->value()->pivot())));
        }
      }

      // Update layer cels
      SurfaceLayerPair sp = findSurfaceLayers(spr, f);
      auto resizeLayerCel = [&](doc::LayerImage* lyr) {
        if (!lyr) return;
        for (doc::frame_t fr = doc::frame_t(0); fr <= spr->lastFrame(); ++fr) {
          doc::Cel* cel = lyr->cel(fr);
          if (!cel || !cel->image()) continue;
          if (resizedImages.count(cel->image()->id())) continue;

          if (cel->image()->width() != targetW || cel->image()->height() != targetH) {
            doc::ImageRef resized(doc::Image::create(cel->image()->pixelFormat(), targetW, targetH));
            resized->clear(0);
            doc::copy_image(resized.get(), cel->image(), 0, 0);
            resizedImages.insert(resized->id());
            api.replaceImage(spr, cel->imageRef(), resized);
            api.setCelPosition(spr, cel, cel->x(), cel->y());
          }
        }
      };
      resizeLayerCel(sp.baseLayer);
      resizeLayerCel(sp.texLayer);
    }

    // Also update Surface_DEPTH if present
    if (doc::LayerImage* depthLyr = findLayerByName(spr, "Surface_DEPTH")) {
      const int maxFaceW = std::max({ data.w[kFaceTop], data.w[kFaceFront], data.w[kFaceRight] });
      const int maxFaceH = std::max({ data.h[kFaceTop], data.h[kFaceFront], data.h[kFaceRight] });
      for (doc::frame_t fr = doc::frame_t(0); fr <= spr->lastFrame(); ++fr) {
        doc::Cel* cel = depthLyr->cel(fr);
        if (!cel || !cel->image()) continue;
        if (resizedImages.count(cel->image()->id())) continue;
        if (cel->image()->width() != maxFaceW || cel->image()->height() != maxFaceH) {
          doc::ImageRef resized(doc::Image::create(cel->image()->pixelFormat(), maxFaceW, maxFaceH));
          resized->clear(0);
          doc::copy_image(resized.get(), cel->image(), 0, 0);
          resizedImages.insert(resized->id());
          api.replaceImage(spr, cel->imageRef(), resized);
          api.setCelPosition(spr, cel, cel->x(), cel->y());
        }
      }
    }

    tx.commit();
    doc->notifyGeneralUpdate();
  }
  catch (const std::exception& e) {
    // Ignore
  }
}

static void propagateLinkedDimensions(Doc* doc, int srcFace, int newW, int newH)
{
  if (!doc || !doc->sprite()) return;
  doc::Sprite* spr = doc->sprite();
  SixSurfaceData data;
  data.init(std::max(spr->width(), spr->height()));
  for (int f = 0; f < kFaceCount; ++f) {
    if (doc::Slice* sl = findSliceForFace(spr, f)) {
      if (const auto* key = sl->getByFrame(0)) {
        data.w[f] = key->bounds().w;
        data.h[f] = key->bounds().h;
        continue;
      }
    }
    SurfaceLayerPair sp = findSurfaceLayers(spr, f);
    doc::LayerImage* lyr = sp.baseLayer ? sp.baseLayer : sp.texLayer;
    if (lyr && lyr->cel(0) && lyr->cel(0)->image()) {
      data.w[f] = lyr->cel(0)->image()->width();
      data.h[f] = lyr->cel(0)->image()->height();
    }
    else {
      data.w[f] = spr->width();
      data.h[f] = spr->height();
    }
  }
  propagateLinkedDimensions(doc, data, srcFace, newW, newH);
}

static void setupTwoLayerStructure(Doc* doc, bool asSmartObjects)
{
  if (!doc || !doc->sprite()) return;
  doc::Sprite* spr = doc->sprite();

  // Shared-sheet model: there is no per-face 2-layer structure
  // to build any more. "Setup 2-layer" now means "make sure the
  // shared Base/Texture sheets exist", and "smart objects" means
  // converting the ONE shared Texture sheet, not six copies of it.
  // (Converting the shared sheet makes the whole UV sheet one
  // smart object; per-face smart-object links no longer exist.)
  setupSharedSurfaceLayers(doc, true);
  if (asSmartObjects) {
    SharedSurfaceLayers shared = findSharedLayers(spr);
    Context* ctx = UIContext::instance() ? static_cast<Context*>(UIContext::instance()) :
                                           App::instance()->context();
    if (ctx && shared.textureLayer && !shared.textureLayer->isSmartObject()) {
      ctx->setActiveLayer(shared.textureLayer);
      Command* cmd = Commands::instance()->byId(CommandId::ConvertLayer());
      if (cmd) {
        Params params;
        params.set("to", "smartobject");
        params.set("ui", "false");
        ctx->executeCommand(cmd, params);
      }
    }
  }
  doc->notifyGeneralUpdate();
}

static void convertSmartObjectsToGroups(Doc* doc)
{
  if (!doc || !doc->sprite()) return;
  doc::Sprite* spr = doc->sprite();

  Context* ctx = UIContext::instance() ? static_cast<Context*>(UIContext::instance()) :
                                         App::instance()->context();
  if (!ctx) return;

  bool convertedAny = false;
  while (true) {
    doc::Layer* target = nullptr;
    for (doc::Layer* lyr : spr->allLayers()) {
      if (lyr->isSmartObject()) {
        target = lyr;
        break;
      }
    }
    if (!target) break;

    ctx->setActiveLayer(target);
    Command* cmd = Commands::instance()->byId(CommandId::ConvertLayer());
    if (cmd) {
      Params params;
      params.set("to", "group");
      ctx->executeCommand(cmd, params);
      convertedAny = true;
    }
    else {
      break;
    }
  }

  if (convertedAny)
    doc->notifyGeneralUpdate();
}

class LinkToggleButton : public ui::Button {
public:
  LinkToggleButton()
    : ui::Button("")
    , m_linked(false)
  {
    setFocusStop(false);
  }

  bool isLinked() const { return m_linked; }

  void setLinked(bool linked)
  {
    if (m_linked != linked) {
      m_linked = linked;
      setSelected(linked);
      invalidate();
    }
  }

protected:
  void onSizeHint(ui::SizeHintEvent& ev) override
  {
    const int s = ui::guiscale();
    ev.setSizeHint(gfx::Size(20 * s, 14 * s));
  }

  void onPaint(ui::PaintEvent& ev) override
  {
    ui::Graphics* g = ev.graphics();
    const gfx::Rect rc = clientBounds();
    const int s = ui::guiscale();
    auto* theme = skin::SkinTheme::instance();

    gfx::Color bg, borderCol, iconCol;
    if (m_linked) {
      bg = hasMouse() ? gfx::rgba(14, 165, 233) : gfx::rgba(2, 132, 199);
      borderCol = gfx::rgba(56, 189, 248);
      iconCol = gfx::rgba(255, 255, 255);
    }
    else {
      bg = hasMouse() ? gfx::rgba(100, 116, 139) : gfx::rgba(64, 78, 94);
      borderCol = hasMouse() ? gfx::rgba(148, 163, 184) : gfx::rgba(45, 55, 66);
      iconCol = hasMouse() ? gfx::rgba(241, 245, 249) : gfx::rgba(180, 195, 210);
    }

    g->fillRect(bg, rc);
    g->drawRect(borderCol, rc);

    // Draw link icon from theme part
    os::Surface* icon = nullptr;
    if (theme) {
      const auto& part = theme->parts.iconAspectRatio();
      if (part)
        icon = part->bitmap(0);
    }

    if (icon) {
      const int ix = rc.x + (rc.w - icon->width()) / 2;
      const int iy = rc.y + (rc.h - icon->height()) / 2;
      g->drawColoredRgbaSurface(icon, iconCol, ix, iy);
    }
    else {
      const int cx = rc.x + rc.w / 2;
      const int cy = rc.y + rc.h / 2;
      g->drawRect(iconCol, gfx::Rect(cx - 5 * s, cy - 2 * s, 5 * s, 4 * s));
      g->drawRect(iconCol, gfx::Rect(cx - 1 * s, cy - 2 * s, 5 * s, 4 * s));
      g->drawHLine(iconCol, cx - 2 * s, cy, 3 * s);
    }
  }

private:
  bool m_linked;
};

// Folder-tab ear header bar matching reference UI
class PaneHeader : public ui::Widget {
public:
  PaneHeader(const std::string& title, gfx::Color accentCol = gfx::ColorNone)
    : ui::Widget(ui::kGenericWidget)
    , m_title(title)
    , m_accentCol(accentCol)
    , m_active(false)
    , m_linkBtn(nullptr)
  {
    setFocusStop(false);
  }

  void setTitle(const std::string& title)
  {
    m_title = title;
    invalidate();
  }

  const std::string& title() const { return m_title; }

  void setActive(bool active)
  {
    if (m_active != active) {
      m_active = active;
      invalidate();
    }
  }

  void setLinkButton(LinkToggleButton* btn)
  {
    m_linkBtn = btn;
    if (btn)
      addChild(btn);
  }

protected:
  void onSizeHint(ui::SizeHintEvent& ev) override
  {
    ev.setSizeHint(gfx::Size(64 * ui::guiscale(), 16 * ui::guiscale()));
  }

  void onResize(ui::ResizeEvent& ev) override
  {
    setBoundsQuietly(ev.bounds());
    if (m_linkBtn) {
      const gfx::Rect rc = childrenBounds();
      const int s = ui::guiscale();
      const int btnW = 20 * s;
      const int btnH = 14 * s;
      m_linkBtn->setBounds(gfx::Rect(rc.x + rc.w - btnW - 3 * s,
                                     rc.y + (rc.h - btnH) / 2,
                                     btnW,
                                     btnH));
    }
  }

  void onPaint(ui::PaintEvent& ev) override
  {
    ui::Graphics* g = ev.graphics();
    const gfx::Rect rc = clientBounds();
    const int s = ui::guiscale();

    const gfx::Size textSz = g->measureText(m_title);
    const int tabW = std::min(rc.w, std::max(textSz.w + 14 * s, 48 * s));
    const gfx::Rect tabRc(rc.x, rc.y, tabW, rc.h);

    // Slate blue header bar background
    g->fillRect(gfx::rgba(92, 108, 122), rc);
    // Raised folder tab ear
    g->fillRect(m_active ? gfx::rgba(125, 142, 160) : gfx::rgba(105, 122, 138), tabRc);
    g->drawRect(gfx::rgba(28, 35, 43), rc);

    if (m_accentCol != gfx::ColorNone) {
      g->fillRect(m_accentCol, gfx::Rect(tabRc.x + 1, tabRc.y + 1, 3 * s, tabRc.h - 2));
    }

    // Grip dots ":::"
    const int rightReserve = (m_linkBtn ? 26 * s : 4 * s);
    if (rc.w > tabW + rightReserve + 16 * s) {
      const int availableSpace = rc.w - tabW - rightReserve;
      const int gx = rc.x + tabW + availableSpace / 2 - 4 * s;
      const int gy = rc.y + rc.h / 2 - 1 * s;
      const gfx::Color dotCol = gfx::rgba(45, 55, 66);
      for (int dx = 0; dx < 3; ++dx) {
        g->fillRect(dotCol, gfx::Rect(gx + dx * 3 * s, gy, s, s));
        g->fillRect(dotCol, gfx::Rect(gx + dx * 3 * s, gy + 2 * s, s, s));
      }
    }

    g->drawText(m_title,
                gfx::rgba(255, 255, 255),
                gfx::ColorNone,
                gfx::Point(tabRc.x + (m_accentCol != gfx::ColorNone ? 6 * s : 4 * s),
                           tabRc.y + (rc.h - textSz.h) / 2));
  }

private:
  std::string m_title;
  gfx::Color m_accentCol;
  bool m_active;
  LinkToggleButton* m_linkBtn;
};

class FacePaneBox : public ui::VBox {
public:
  FacePaneBox(int faceIndex, DocView* docView, Editor* editor)
    : m_faceIndex(faceIndex)
    , m_docView(docView)
    , m_editor(editor)
  {
  }

protected:
  bool onProcessMessage(ui::Message* msg) override
  {
    if (msg->type() == ui::kMouseDownMessage) {
      if (m_docView && m_docView->document() && m_docView->document()->sprite()) {
        doc::Sprite* spr = m_docView->document()->sprite();
        // Shared sheets: every panel edits the same Texture (or
        // Base) layer; only the editor's clip rect (its UV rect,
        // set in initLayersAndFraming) differs per panel.
        SharedSurfaceLayers shared = findSharedLayers(spr);
        doc::LayerImage* lyr = nullptr;
        if (shared.textureLayer && !shared.textureLayer->isSmartObject())
          lyr = shared.textureLayer;
        else if (shared.baseLayer)
          lyr = shared.baseLayer;
        else if (shared.textureLayer)
          lyr = shared.textureLayer;
        if (!lyr) {
          SurfaceLayerPair sp = findSurfaceLayers(spr, m_faceIndex);
          if (sp.texLayer && !sp.texLayer->isSmartObject())
            lyr = sp.texLayer;
          else if (sp.baseLayer)
            lyr = sp.baseLayer;
          else if (sp.texLayer)
            lyr = sp.texLayer;
          if (!lyr) {
            const std::string lyrName = std::string("Surface_") + kFaceSpecs[m_faceIndex].sliceName;
            lyr = findLayerByName(spr, lyrName);
          }
        }
        if (lyr) {
          m_editor->setLayer(lyr);
          if (m_docView->editor() != m_editor) {
            m_docView->setActivePaneEditor(m_editor);
          }
          if (UIContext* ctx = UIContext::instance())
            ctx->setActiveView(m_docView);
        }
      }
    }
    return ui::VBox::onProcessMessage(msg);
  }

private:
  int m_faceIndex;
  DocView* m_docView;
  Editor* m_editor;
};

// Checkerboard helper
static void drawCheckerboardPattern(ui::Graphics* g, const gfx::Rect& rc)
{
  const int tile = 12 * ui::guiscale();
  const gfx::Color c1 = gfx::rgba(128, 128, 128);
  const gfx::Color c2 = gfx::rgba(192, 192, 192);
  g->fillRect(c1, rc);
  for (int y = rc.y; y < rc.y2(); y += tile) {
    const int row = (y - rc.y) / tile;
    const int h = std::min(tile, rc.y2() - y);
    for (int x = rc.x; x < rc.x2(); x += tile) {
      const int col = (x - rc.x) / tile;
      if (((row + col) & 1) == 1) {
        const int w = std::min(tile, rc.x2() - x);
        g->fillRect(c2, gfx::Rect(x, y, w, h));
      }
    }
  }
}

// 3D Preview Widget (Top of Column 4)
class EasymetricPreviewWidget : public ui::Widget {
public:
  EasymetricPreviewWidget()
    : ui::Widget(ui::kGenericWidget)
  {
    setExpansive(true);
  }

  obs::signal<void(int, int)> OrbitDrag;
  obs::signal<void(double)> ZoomChanged;

  // Zoom of the rendered model, relative to the fit-to-pane size.
  // 1.0 = fit, 0.25 = quarter size, 8.0 = 8x. Wheel, +/- buttons and
  // double-click (reset) all go through setZoom() so the value is
  // always clamped and observers always see the same value.
  double zoom() const { return m_zoom; }

  void setZoom(double zoom)
  {
    const double clamped = std::clamp(zoom, 0.25, 8.0);
    if (std::abs(clamped - m_zoom) < 1e-9)
      return;
    m_zoom = clamped;
    ZoomChanged(m_zoom);
    invalidate();
  }

  void zoomIn() { setZoom(m_zoom * 1.25); }
  void zoomOut() { setZoom(m_zoom / 1.25); }

  void resetZoom()
  {
    m_panX = 0;
    m_panY = 0;
    setZoom(1.0);
    // setZoom() skips invalidate() when already at 1.0, but the pan
    // reset above still needs a repaint in that case.
    invalidate();
  }

  void setSurfaces(const os::SurfaceRef& diffSurf,
                   const os::SurfaceRef& normSurf,
                   const os::SurfaceRef& deptSurf,
                   int yawDeg,
                   int pitchDeg,
                   int pivScreenX,
                   int pivScreenY,
                   bool showDiff,
                   bool showNorm,
                   bool showDept)
  {
    m_diffSurf = diffSurf;
    m_normSurf = normSurf;
    m_deptSurf = deptSurf;
    m_yawDeg = yawDeg;
    m_pitchDeg = pitchDeg;
    m_pivScreenX = pivScreenX;
    m_pivScreenY = pivScreenY;
    m_showDiff = showDiff;
    m_showNorm = showNorm;
    m_showDept = showDept;
    invalidate();
  }

protected:
  bool onProcessMessage(ui::Message* msg) override
  {
    switch (msg->type()) {
      case ui::kMouseDownMessage: {
        auto* mouseMsg = static_cast<ui::MouseMessage*>(msg);
        if (mouseMsg->left()) {
          m_dragging = true;
          m_dragStart = mouseMsg->position();
          m_startYaw = m_yawDeg;
          m_startPitch = m_pitchDeg;
          captureMouse();
          return true;
        }
        if (mouseMsg->right() || mouseMsg->middle()) {
          // Pan the zoomed rendering; left-drag stays reserved for orbit.
          m_panning = true;
          m_panStart = mouseMsg->position();
          m_startPanX = m_panX;
          m_startPanY = m_panY;
          captureMouse();
          return true;
        }
        break;
      }
      case ui::kMouseMoveMessage: {
        if (m_panning) {
          auto* mouseMsg = static_cast<ui::MouseMessage*>(msg);
          const gfx::Point pos = mouseMsg->position();
          m_panX = m_startPanX + (pos.x - m_panStart.x);
          m_panY = m_startPanY + (pos.y - m_panStart.y);
          invalidate();
          return true;
        }
        if (m_dragging) {
          auto* mouseMsg = static_cast<ui::MouseMessage*>(msg);
          const gfx::Point pos = mouseMsg->position();
          const int s = std::max(1, ui::guiscale());
          const int dYaw = (pos.x - m_dragStart.x) / s;
          const int dPitch = (pos.y - m_dragStart.y) / s;
          int newYaw = m_startYaw + dYaw;
          while (newYaw > 180) newYaw -= 360;
          while (newYaw < -180) newYaw += 360;
          int newPitch = std::clamp(m_startPitch + dPitch, -75, 75);
          OrbitDrag(newYaw, newPitch);
          return true;
        }
        break;
      }
      case ui::kMouseUpMessage: {
        if (m_dragging || m_panning) {
          m_dragging = false;
          m_panning = false;
          releaseMouse();
          return true;
        }
        break;
      }
      case ui::kMouseWheelMessage: {
        auto* mouseMsg = static_cast<ui::MouseMessage*>(msg);
        const gfx::Point delta = mouseMsg->wheelDelta();
        // Shift+wheel scrolls (pans) instead of zooming, so the
        // preview can be scrolled as well as zoomed.
        if (mouseMsg->shiftPressed()) {
          m_panX += delta.x * 8;
          m_panY -= delta.y * 8;
          invalidate();
          return true;
        }
        // Same sign convention as palette_view: delta.x - delta.y > 0
        // means wheel-up, which zooms in.
        const int steps = delta.x - delta.y;
        if (steps != 0) {
          setZoom(m_zoom * std::pow(1.25, steps));
          return true;
        }
        break;
      }
      case ui::kDoubleClickMessage: {
        resetZoom();
        return true;
      }
    }
    return ui::Widget::onProcessMessage(msg);
  }

  void onPaint(ui::PaintEvent& ev) override
  {
    ui::Graphics* g = ev.graphics();
    const gfx::Rect rc = clientBounds();
    const int s = ui::guiscale();

    drawCheckerboardPattern(g, rc);
    g->drawRect(gfx::rgba(28, 35, 43), rc);

    std::vector<os::SurfaceRef> activeSurfs;
    std::vector<const char*> activeLabels;
    if (m_showDiff && m_diffSurf) {
      activeSurfs.push_back(m_diffSurf);
      activeLabels.push_back("Diffuse");
    }
    if (m_showNorm && m_normSurf) {
      activeSurfs.push_back(m_normSurf);
      activeLabels.push_back("Normal");
    }
    if (m_showDept && m_deptSurf) {
      activeSurfs.push_back(m_deptSurf);
      activeLabels.push_back("Depth");
    }
    if (activeSurfs.empty() && m_diffSurf) {
      activeSurfs.push_back(m_diffSurf);
      activeLabels.push_back("Diffuse");
    }

    if (!activeSurfs.empty()) {
      const int count = int(activeSurfs.size());
      const int cellW = rc.w / count;

      for (int i = 0; i < count; ++i) {
        const gfx::Rect cellRc(rc.x + i * cellW, rc.y, cellW, rc.h);
        if (i > 0)
          g->drawVLine(gfx::rgba(28, 35, 43, 180), cellRc.x, cellRc.y, cellRc.h);

        const int sw = activeSurfs[i]->width();
        const int sh = activeSurfs[i]->height();
        if (sw > 0 && sh > 0) {
          const double scaleX = double(cellRc.w - 8 * s) / double(sw);
          const double scaleY = double(cellRc.h - 8 * s) / double(sh);
          const double scale = std::max(0.5, std::min(scaleX, scaleY)) * m_zoom;
          const int dw = std::max(1, int(sw * scale));
          const int dh = std::max(1, int(sh * scale));
          const int dx = cellRc.x + (cellRc.w - dw) / 2 + m_panX;
          const int dy = cellRc.y + (cellRc.h - dh) / 2 + m_panY;

          // Keep a zoomed rendering inside its own cell instead of
          // painting over the neighbouring map cells.
          ui::IntersectClip clip(g, cellRc);
          if (clip) {
            g->drawSurface(activeSurfs[i].get(),
                           gfx::Rect(0, 0, sw, sh),
                           gfx::Rect(dx, dy, dw, dh),
                           os::Sampling(os::Sampling::Filter::Nearest),
                           nullptr);

            if (i == 0) {
              const int px = dx + int(m_pivScreenX * scale);
              const int py = dy + int(m_pivScreenY * scale);
              if (cellRc.contains(gfx::Point(px, py))) {
                const gfx::Color pivCol = gfx::rgba(250, 204, 21, 230);
                g->drawHLine(pivCol, px - 4 * s, py, 9 * s);
                g->drawVLine(pivCol, px, py - 4 * s, 9 * s);
              }
            }
          }
        }

        const gfx::Size lblSz = g->measureText(activeLabels[i]);
        const gfx::Rect badgeRc(cellRc.x + 3 * s, cellRc.y + 3 * s, lblSz.w + 6 * s, lblSz.h + 2 * s);
        g->fillRect(gfx::rgba(28, 35, 43, 195), badgeRc);
        g->drawText(activeLabels[i],
                    gfx::rgba(255, 255, 255),
                    gfx::ColorNone,
                    gfx::Point(badgeRc.x + 3 * s, badgeRc.y + 1 * s));
      }
    }

    if (std::abs(m_zoom - 1.0) > 1e-9) {
      const std::string zoomText = fmt::format("{}%", int(std::round(m_zoom * 100.0)));
      const gfx::Size zoomSz = g->measureText(zoomText);
      const gfx::Rect zoomRc(rc.x + rc.w - zoomSz.w - 9 * s, rc.y + 3 * s,
                             zoomSz.w + 6 * s, zoomSz.h + 2 * s);
      g->fillRect(gfx::rgba(28, 35, 43, 195), zoomRc);
      g->drawText(zoomText,
                  gfx::rgba(255, 255, 255),
                  gfx::ColorNone,
                  gfx::Point(zoomRc.x + 3 * s, zoomRc.y + 1 * s));
    }
  }

private:
  os::SurfaceRef m_diffSurf;
  os::SurfaceRef m_normSurf;
  os::SurfaceRef m_deptSurf;
  int m_yawDeg = 45;
  int m_pitchDeg = 30;
  int m_pivScreenX = 40;
  int m_pivScreenY = 40;
  bool m_showDiff = true;
  bool m_showNorm = false;
  bool m_showDept = false;
  bool m_dragging = false;
  gfx::Point m_dragStart;
  int m_startYaw = 45;
  int m_startPitch = 30;
  double m_zoom = 1.0;
  int m_panX = 0;
  int m_panY = 0;
  bool m_panning = false;
  gfx::Point m_panStart;
  int m_startPanX = 0;
  int m_startPanY = 0;
};

// Sleek dark-slate container matching reference UI controls
class FieldsPanelBox : public ui::VBox {
public:
  FieldsPanelBox()
  {
    setExpansive(true);
  }
protected:
  void onPaint(ui::PaintEvent& ev) override
  {
    ui::Graphics* g = ev.graphics();
    const gfx::Rect rc = clientBounds();
    g->fillRect(gfx::rgba(45, 55, 66), rc);
    g->drawRect(gfx::rgba(28, 35, 43), rc);
  }
};

static void setupSixSurfacesOnDoc(Doc* doc, bool seedStarterIfBlank)
{
  // Entering the studio now sets up the shared UV sheets, not
  // per-face layers. setupSharedSurfaceLayers migrates any existing
  // per-face Base/Texture images into their UV cells and removes
  // the old layers, so opening an old document upgrades it in
  // place instead of leaving 12+ layers behind.
  setupSharedSurfaceLayers(doc, seedStarterIfBlank);
}

static bool extractSixSurfaces(
  Doc* doc,
  doc::frame_t frame,
  SixSurfaceData& out,
  bool useFaceDepth = true,
  DocView* docView = nullptr,
  const std::array<bool, 3>* colLinked = nullptr)
{
  if (!doc || !doc->sprite()) return false;
  doc::Sprite* spr = doc->sprite();
  const int sw = spr->width();
  const int sh = spr->height();
  out.init(std::max(sw, sh));

  doc::ImageRef depthSrcImg;
  if (useFaceDepth) {
    if (doc::LayerImage* depthLyr = findLayerByName(spr, "Surface_DEPTH")) {
      if (const doc::Cel* dCel = depthLyr->cel(frame)) {
        if (dCel->image() && dCel->image()->pixelFormat() == doc::IMAGE_RGB) {
          depthSrcImg.reset(doc::Image::create(doc::IMAGE_RGB, sw, sh));
          depthSrcImg->clear(0);
          doc::copy_image(depthSrcImg.get(), dCel->image(), dCel->x(), dCel->y());
          out.hasCustomDepthLayer = true;
        }
      }
    }
  }

  ExtraCelRef extra = doc->extraCel();

  // Shared sheets, if present, decide both the layers and the rect:
  // each face samples its own UV rect of the same Base/Texture
  // sheets, instead of its own per-face layers.
  const SharedSurfaceLayers shared = findSharedLayers(spr);
  const bool useShared = (shared.baseLayer || shared.textureLayer);

  for (int f = 0; f < kFaceCount; ++f) {
    doc::Slice* slice = findSliceForFace(spr, f);
    gfx::Rect faceRect(0, 0, sw, sh);
    if (slice) {
      const auto* key = slice->getByFrame(frame);
      if (key && !key->bounds().isEmpty()) {
        faceRect = key->bounds();
      }
    }
    if (useShared) {
      // Recompute from the grid so a slice left at (0, 0) by the
      // old layout does not make every face sample the same cell.
      // sharedFaceRect keeps the slice's face size, only the
      // origin moves to the face's UV cell.
      faceRect = sharedFaceRect(spr, f);
    }
    const int fw = faceRect.w;
    const int fh = faceRect.h;
    out.bounds[f] = faceRect;
    out.setFaceSize(f, fw, fh);
    out.hasSlices = true;

    doc::LayerImage* baseLyr = shared.baseLayer;
    doc::LayerImage* texLyr = shared.textureLayer;
    if (!useShared) {
      SurfaceLayerPair sp = findSurfaceLayers(spr, f);
      baseLyr = sp.baseLayer;
      texLyr = sp.texLayer;
    }

    // If no dedicated surface layer found, check shared layer
    if (!baseLyr && !texLyr) {
      for (doc::Layer* l : spr->allLayers()) {
        if (l->isImage() && l->isVisible()) {
          baseLyr = static_cast<doc::LayerImage*>(l);
          break;
        }
      }
    }

    const doc::Cel* baseCel = baseLyr ? baseLyr->cel(frame) : nullptr;
    const doc::Cel* texCel = texLyr ? texLyr->cel(frame) : nullptr;
    const doc::Image* baseImg = baseCel ? baseCel->image() : nullptr;
    const doc::Image* texImg = texCel ? texCel->image() : nullptr;

    const int baseOx = baseCel ? baseCel->x() : 0;
    const int baseOy = baseCel ? baseCel->y() : 0;
    const int texOx = texCel ? texCel->x() : 0;
    const int texOy = texCel ? texCel->y() : 0;

    auto getRgbaPixel = [&](const doc::Image* img, int x, int y) -> uint32_t {
      if (!img || x < 0 || x >= img->width() || y < 0 || y >= img->height())
        return 0;
      switch (img->pixelFormat()) {
        case doc::IMAGE_RGB:
          return doc::get_pixel_fast<doc::RgbTraits>(img, x, y);
        case doc::IMAGE_GRAYSCALE: {
          const doc::color_t c = doc::get_pixel_fast<doc::GrayscaleTraits>(img, x, y);
          const int val = doc::graya_getv(c);
          return doc::rgba(val, val, val, doc::graya_geta(c));
        }
        case doc::IMAGE_INDEXED: {
          const doc::color_t idx = doc::get_pixel_fast<doc::IndexedTraits>(img, x, y);
          if (idx == spr->transparentColor())
            return 0;
          if (const doc::Palette* pal = spr->palette(frame))
            return pal->getEntry(idx);
          return 0;
        }
        default:
          return 0;
      }
    };

    bool hasPixels = false;

    for (int v = 0; v < fh; ++v) {
      const int canvasY = faceRect.y + v;
      for (int u = 0; u < fw; ++u) {
        const int canvasX = faceRect.x + u;

        uint32_t baseCol = getRgbaPixel(baseImg, canvasX - baseOx, canvasY - baseOy);
        uint32_t texCol = getRgbaPixel(texImg, canvasX - texOx, canvasY - texOy);

        uint32_t finalCol = 0;
        if (baseImg && texImg) {
          // Two-layer clipping mask & smart object structure:
          // Base layer defines alpha clipping mask.
          // When base alpha > 16: texture layer overrides base content.
          // When texture is transparent: falls back to base content.
          // When texture is semi-transparent: alpha-blends over base content.
          // When base alpha <= 16: masked out (transparent).
          if (doc::rgba_geta(baseCol) > 16) {
            const int aT = doc::rgba_geta(texCol);
            if (aT == 255) {
              finalCol = texCol;
            }
            else if (aT > 0) {
              const int invA = 255 - aT;
              finalCol = doc::rgba(
                (doc::rgba_getr(texCol) * aT + doc::rgba_getr(baseCol) * invA) / 255,
                (doc::rgba_getg(texCol) * aT + doc::rgba_getg(baseCol) * invA) / 255,
                (doc::rgba_getb(texCol) * aT + doc::rgba_getb(baseCol) * invA) / 255,
                doc::rgba_geta(baseCol));
            }
            else {
              finalCol = baseCol;
            }
          }
        }
        else if (texImg) {
          finalCol = texCol;
        }
        else if (baseImg) {
          finalCol = baseCol;
        }

        if (doc::rgba_geta(finalCol) > 0) {
          out.pixels[f][v * fw + u] = finalCol;
          hasPixels = true;
        }
      }
    }

    // Check if extra cel applies to this face editor
    if (extra && extra->type() != render::ExtraType::NONE &&
        extra->purpose() != ExtraCel::Purpose::BrushPreview &&
        extra->image() && extra->cel() &&
        docView && docView->editor() &&
        (docView->editor()->layer() == baseLyr || docView->editor()->layer() == texLyr)) {
      const doc::Image* eImg = extra->image();
      const int ox = extra->cel()->x();
      const int oy = extra->cel()->y();
      const bool editingTex = (docView->editor()->layer() == texLyr && baseImg);

      for (int v = 0; v < fh; ++v) {
        const int canvasY = faceRect.y + v;
        const int sy = canvasY - oy;
        if (sy < 0 || sy >= eImg->height()) continue;
        for (int u = 0; u < fw; ++u) {
          const int canvasX = faceRect.x + u;
          const int sx = canvasX - ox;
          if (sx < 0 || sx >= eImg->width()) continue;
          const uint32_t ec = getRgbaPixel(eImg, sx, sy);
          if (doc::rgba_geta(ec) > 0) {
            if (editingTex) {
              const int bx = canvasX - baseOx;
              const int by = canvasY - baseOy;
              uint32_t bc = getRgbaPixel(baseImg, bx, by);
              if (doc::rgba_geta(bc) > 16) {
                const int aT = doc::rgba_geta(ec);
                if (aT == 255) {
                  out.pixels[f][v * fw + u] = ec;
                }
                else if (aT > 0) {
                  const int invA = 255 - aT;
                  out.pixels[f][v * fw + u] = doc::rgba(
                    (doc::rgba_getr(ec) * aT + doc::rgba_getr(bc) * invA) / 255,
                    (doc::rgba_getg(ec) * aT + doc::rgba_getg(bc) * invA) / 255,
                    (doc::rgba_getb(ec) * aT + doc::rgba_getb(bc) * invA) / 255,
                    doc::rgba_geta(bc));
                }
                else {
                  out.pixels[f][v * fw + u] = bc;
                }
                hasPixels = true;
              }
            }
            else {
              out.pixels[f][v * fw + u] = ec;
              hasPixels = true;
            }
          }
        }
      }
    }

    // If custom depth layer is present, extract depth for this face
    if (depthSrcImg) {
      for (int v = 0; v < fh; ++v) {
        const int canvasY = faceRect.y + v;
        if (canvasY < 0 || canvasY >= depthSrcImg->height()) continue;
        for (int u = 0; u < fw; ++u) {
          const int canvasX = faceRect.x + u;
          if (canvasX < 0 || canvasX >= depthSrcImg->width()) continue;
          const uint32_t dc = doc::get_pixel_fast<doc::RgbTraits>(depthSrcImg.get(), canvasX, canvasY);
          out.depthPixels[f][v * fw + u] = clampByte(
            doc::rgba_getr(dc) * 0.299f + doc::rgba_getg(dc) * 0.587f + doc::rgba_getb(dc) * 0.114f);
        }
      }
    }

    if (!hasPixels) {
      for (int v = 0; v < fh; ++v) {
        for (int u = 0; u < fw; ++u) {
          out.pixels[f][v * fw + u] = starterSurfacePixel(f, u, v, fw, fh);
        }
      }
    }
  }

  // Unify linked opposing face pairs (Top/Down, Front/Back, Right/Left)
  if (colLinked) {
    for (int c = 0; c < 3; ++c) {
      out.colLinked[c] = (*colLinked)[c];
      if ((*colLinked)[c]) {
        const int topFace = c * 2;
        const int btmFace = c * 2 + 1;
        out.pixels[btmFace] = out.pixels[topFace];
        out.depthPixels[btmFace] = out.depthPixels[topFace];
        out.w[btmFace] = out.w[topFace];
        out.h[btmFace] = out.h[topFace];
        out.bounds[btmFace] = out.bounds[topFace];
      }
    }
  }

  return true;
}

struct Render3DResult {
  doc::ImageRef diffImg;
  doc::ImageRef normImg;
  doc::ImageRef deptImg;
  int outW = 0;
  int outH = 0;
  float cxScreen = 0.0f;
  float cyScreen = 0.0f;
};

static Render3DResult compute3DPreviewImages(
  const SixSurfaceData& data,
  int yawDeg,
  int pitchDeg,
  int pivX,
  int pivY,
  int pivZ,
  int renderStyle = 0,
  bool addOutline = true,
  bool allowCutouts = true)
{
  Render3DResult res;
  const float halfX = std::max({ data.w[kFaceFront], data.w[kFaceBack], data.w[kFaceTop], data.w[kFaceDown] }) * 0.5f;
  const float halfY = std::max({ data.h[kFaceFront], data.h[kFaceBack], data.h[kFaceLeft], data.h[kFaceRight] }) * 0.5f;
  const float halfZ = std::max({ data.w[kFaceLeft], data.w[kFaceRight], data.h[kFaceTop], data.h[kFaceDown] }) * 0.5f;
  const int maxDim = std::max({ int(halfX * 2.0f), int(halfY * 2.0f), int(halfZ * 2.0f), 24 });
  const int outW = maxDim * 2 + 16;
  const int outH = maxDim * 2 + 16;

  res.outW = outW;
  res.outH = outH;

  constexpr float kDegToRad = 3.14159265358979323846f / 180.0f;
  const float yaw = yawDeg * kDegToRad;
  const float pitch = pitchDeg * kDegToRad;
  const float cy = std::cos(yaw), sy = std::sin(yaw);
  const float cp = std::cos(pitch), sp = std::sin(pitch);

  const float rxX = cy,      rxY = 0.0f, rzX = -sy;
  const float ryX = sy * sp, ryY = cp,   ryZ = cy * sp;
  const float vdX = sy * cp, vdY = -sp,  vdZ = cy * cp;

  const float pivOffX = ((pivX - 50) * 0.01f) * (halfX * 2.0f);
  const float pivOffY = ((pivY - 50) * 0.01f) * (halfY * 2.0f);
  const float pivOffZ = ((pivZ - 50) * 0.01f) * (halfZ * 2.0f);

  const float cxScreen = outW * 0.5f;
  const float cyScreen = outH * 0.5f;
  res.cxScreen = cxScreen;
  res.cyScreen = cyScreen;

  res.diffImg.reset(doc::Image::create(doc::IMAGE_RGB, outW, outH));
  res.normImg.reset(doc::Image::create(doc::IMAGE_RGB, outW, outH));
  res.deptImg.reset(doc::Image::create(doc::IMAGE_RGB, outW, outH));
  res.diffImg->clear(0);
  res.normImg->clear(0);
  res.deptImg->clear(0);

  SculptedVoxelVolume vol = buildSculptedVolume(data, pivX, pivY, pivZ);

  std::vector<int> faceMap(outW * outH, -1);
  const float maxDepthRange = float(maxDim) * 1.732f;
  const float Lx = -0.45f, Ly = -0.60f, Lz = 0.66f;

  const float camDist = maxDepthRange * 2.5f;
  const float nearPlane = 1.0f;
  const float farPlane = camDist * 2.0f + maxDepthRange;

  for (int py = 0; py < outH; ++py) {
    const float camY = (py - cyScreen);
    uint32_t* __restrict diffRow =
      reinterpret_cast<uint32_t*>(res.diffImg->getPixelAddress(0, py));
    uint32_t* __restrict normRow =
      reinterpret_cast<uint32_t*>(res.normImg->getPixelAddress(0, py));
    uint32_t* __restrict deptRow =
      reinterpret_cast<uint32_t*>(res.deptImg->getPixelAddress(0, py));

    for (int px = 0; px < outW; ++px) {
      const float camX = (px - cxScreen);
      const float ox = (camX * rxX + camY * ryX + pivOffX) - camDist * vdX;
      const float oy = (camX * rxY + camY * ryY + pivOffY) - camDist * vdY;
      const float oz = (camX * rzX + camY * ryZ + pivOffZ) - camDist * vdZ;

      float t0 = -1e9f, t1 = 1e9f;
      auto clipAxis = [&](float orig, float dir, float bMin, float bMax) -> bool {
        if (std::fabs(dir) > 1e-5f) {
          float invD = 1.0f / dir;
          float ta = (bMin - orig) * invD;
          float tb = (bMax - orig) * invD;
          if (ta > tb) std::swap(ta, tb);
          if (ta > t0) t0 = ta;
          if (tb < t1) t1 = tb;
          return t0 <= t1;
        }
        return (orig >= bMin && orig <= bMax);
      };

      if (!clipAxis(ox, vdX, -halfX, halfX) ||
          !clipAxis(oy, vdY, -halfY, halfY) ||
          !clipAxis(oz, vdZ, -halfZ, halfZ) ||
          t1 < nearPlane || t0 > farPlane || t0 > t1) {
        continue;
      }

      const float stepSize = std::min({ (2.0f * halfX) / float(vol.vx),
                                        (2.0f * halfY) / float(vol.vy),
                                        (2.0f * halfZ) / float(vol.vz) }) * 0.45f;
      float tStart = std::max(nearPlane, t0);
      float tEnd = std::min(farPlane, t1);
      float tHit = -1.0f;
      int hitVx = -1, hitVy = -1, hitVz = -1;

      for (float t = tStart; t <= tEnd; t += stepSize) {
        const float pxW = ox + t * vdX;
        const float pyW = oy + t * vdY;
        const float pzW = oz + t * vdZ;

        int vx = int((pxW + halfX) / (2.0f * halfX) * float(vol.vx));
        int vy = int((pyW + halfY) / (2.0f * halfY) * float(vol.vy));
        int vz = int((pzW + halfZ) / (2.0f * halfZ) * float(vol.vz));

        if (vol.isSolid(vx, vy, vz)) {
          float ta = t - stepSize;
          float tb = t;
          for (int b = 0; b < 3; ++b) {
            float tm = (ta + tb) * 0.5f;
            int mx = int((ox + tm * vdX + halfX) / (2.0f * halfX) * float(vol.vx));
            int my = int((oy + tm * vdY + halfY) / (2.0f * halfY) * float(vol.vy));
            int mz = int((oz + tm * vdZ + halfZ) / (2.0f * halfZ) * float(vol.vz));
            if (vol.isSolid(mx, my, mz)) {
              tb = tm;
              vx = mx; vy = my; vz = mz;
            }
            else {
              ta = tm;
            }
          }
          tHit = tb;
          hitVx = vx; hitVy = vy; hitVz = vz;
          break;
        }
      }

      if (tHit < 0.0f) continue;

      float gX = float(vol.isSolid(hitVx - 1, hitVy, hitVz)) - float(vol.isSolid(hitVx + 1, hitVy, hitVz));
      float gY = float(vol.isSolid(hitVx, hitVy - 1, hitVz)) - float(vol.isSolid(hitVx, hitVy + 1, hitVz));
      float gZ = float(vol.isSolid(hitVx, hitVy, hitVz - 1)) - float(vol.isSolid(hitVx, hitVy, hitVz + 1));
      float len = std::sqrt(gX * gX + gY * gY + gZ * gZ);
      if (len > 1e-4f) {
        gX /= len; gY /= len; gZ /= len;
      }
      else {
        gX = (ox + tHit * vdX) / halfX;
        gY = (oy + tHit * vdY) / halfY;
        gZ = (oz + tHit * vdZ) / halfZ;
        len = std::sqrt(gX * gX + gY * gY + gZ * gZ);
        if (len > 1e-4f) { gX /= len; gY /= len; gZ /= len; }
        else { gX = 0.0f; gY = 0.0f; gZ = 1.0f; }
      }

      int face = kFaceFront;
      if (std::fabs(gZ) >= std::fabs(gX) && std::fabs(gZ) >= std::fabs(gY)) {
        face = (gZ > 0.0f ? kFaceFront : kFaceBack);
      }
      else if (std::fabs(gY) >= std::fabs(gX) && std::fabs(gY) >= std::fabs(gZ)) {
        face = (gY < 0.0f ? kFaceTop : kFaceDown);
      }
      else {
        face = (gX > 0.0f ? kFaceRight : kFaceLeft);
      }

      const float hx = ox + tHit * vdX;
      const float hy = oy + tHit * vdY;
      const float hz = oz + tHit * vdZ;

      const int fw = data.w[face];
      const int fh = data.h[face];
      int u = 0, v = 0;
      switch (face) {
        case kFaceFront:
          u = std::clamp(int((hx + halfX) / (2.0f * halfX) * fw), 0, fw - 1);
          v = std::clamp(int((hy + halfY) / (2.0f * halfY) * fh), 0, fh - 1);
          break;
        case kFaceBack:
          u = std::clamp(int((halfX - hx) / (2.0f * halfX) * fw), 0, fw - 1);
          v = std::clamp(int((hy + halfY) / (2.0f * halfY) * fh), 0, fh - 1);
          break;
        case kFaceLeft:
          u = std::clamp(int((hz + halfZ) / (2.0f * halfZ) * fw), 0, fw - 1);
          v = std::clamp(int((hy + halfY) / (2.0f * halfY) * fh), 0, fh - 1);
          break;
        case kFaceRight:
          u = std::clamp(int((halfZ - hz) / (2.0f * halfZ) * fw), 0, fw - 1);
          v = std::clamp(int((hy + halfY) / (2.0f * halfY) * fh), 0, fh - 1);
          break;
        case kFaceTop:
          u = std::clamp(int((hx + halfX) / (2.0f * halfX) * fw), 0, fw - 1);
          v = std::clamp(int((hz + halfZ) / (2.0f * halfZ) * fh), 0, fh - 1);
          break;
        case kFaceDown:
          u = std::clamp(int((hx + halfX) / (2.0f * halfX) * fw), 0, fw - 1);
          v = std::clamp(int((halfZ - hz) / (2.0f * halfZ) * fh), 0, fh - 1);
          break;
      }

      const uint32_t texCol = data.sampleSymmetric(face, u, v);
      const uint8_t a = doc::rgba_geta(texCol);
      if (a <= 16 && renderStyle != kRenderColoredFaces) {
        if (allowCutouts) continue;
      }

      const float dC = data.sampleDepthNorm(face, u, v);
      const float dL = data.sampleDepthNorm(face, u - 1, v);
      const float dR = data.sampleDepthNorm(face, u + 1, v);
      const float dU = data.sampleDepthNorm(face, u, v - 1);
      const float dD = data.sampleDepthNorm(face, u, v + 1);
      const float gradScale = data.hasCustomDepthLayer ? 1.45f : 0.65f;
      const float du = (dL - dR) * gradScale;
      const float dv = (dD - dU) * gradScale;

      float cnx = gX * rxX + gY * ryX + gZ * rzX + du;
      float cny = -(gX * ryX + gY * ryY + gZ * ryZ) + dv;
      float cnz = std::fabs(gX * vdX + gY * vdY + gZ * vdZ) + 0.15f;
      const float nLen = std::sqrt(cnx * cnx + cny * cny + cnz * cnz);
      if (nLen > 1e-4f) {
        cnx /= nLen; cny /= nLen; cnz /= nLen;
      }

      const uint8_t nr = clampByte((cnx * 0.5f + 0.5f) * 255.0f);
      const uint8_t ng = clampByte((cny * 0.5f + 0.5f) * 255.0f);
      const uint8_t nb = clampByte((cnz * 0.5f + 0.5f) * 255.0f);
      const uint32_t normPixel = doc::rgba(nr, ng, nb, 255);
      normRow[px] = normPixel;

      const float tRel = tHit - camDist;
      const float normZ = std::clamp((tRel + maxDepthRange * 0.5f) / maxDepthRange, 0.0f, 1.0f);
      const float faceRelief = (dC - 0.5f) * (data.hasCustomDepthLayer ? 0.35f : 0.14f);
      const uint8_t dByte = clampByte(35.0f + std::clamp(normZ + faceRelief, 0.0f, 1.0f) * 215.0f);
      const uint32_t deptPixel = doc::rgba(dByte, dByte, dByte, 255);
      deptRow[px] = deptPixel;

      const float ndotl = std::max(0.0f, cnx * Lx + (-cny) * Ly + cnz * Lz);
      const float shade = std::clamp(0.84f + ndotl * 0.16f, 0.84f, 1.0f);

      switch (renderStyle) {
        case kRenderTexturedFlat:
          diffRow[px] = doc::rgba(doc::rgba_getr(texCol),
                                  doc::rgba_getg(texCol),
                                  doc::rgba_getb(texCol), 255);
          break;
        case kRenderColoredFaces: {
          const auto& spec = kFaceSpecs[face];
          const float fShade = std::clamp(0.75f + ndotl * 0.25f, 0.75f, 1.0f);
          diffRow[px] = doc::rgba(clampByte(spec.r * fShade),
                                  clampByte(spec.g * fShade),
                                  clampByte(spec.b * fShade), 255);
          break;
        }
        case kRenderNormalShaded:
          diffRow[px] = normPixel;
          break;
        case kRenderDepthShaded:
          diffRow[px] = deptPixel;
          break;
        case kRenderWireframeTex: {
          bool isVoxelEdge = (hitVx == 0 || hitVy == 0 || hitVz == 0 ||
                              hitVx == vol.vx - 1 || hitVy == vol.vy - 1 || hitVz == vol.vz - 1 ||
                              !vol.isSolid(hitVx - 1, hitVy, hitVz) || !vol.isSolid(hitVx + 1, hitVy, hitVz) ||
                              !vol.isSolid(hitVx, hitVy - 1, hitVz) || !vol.isSolid(hitVx, hitVy + 1, hitVz) ||
                              !vol.isSolid(hitVx, hitVy, hitVz - 1) || !vol.isSolid(hitVx, hitVy, hitVz + 1));
          if (isVoxelEdge) {
            diffRow[px] = doc::rgba(14, 165, 233, 255);
          }
          else {
            diffRow[px] = doc::rgba(clampByte(doc::rgba_getr(texCol) * shade * 0.75f),
                                    clampByte(doc::rgba_getg(texCol) * shade * 0.75f),
                                    clampByte(doc::rgba_getb(texCol) * shade * 0.75f), 255);
          }
          break;
        }
        case kRenderTexturedLit:
        default:
          diffRow[px] = doc::rgba(clampByte(doc::rgba_getr(texCol) * shade),
                                  clampByte(doc::rgba_getg(texCol) * shade),
                                  clampByte(doc::rgba_getb(texCol) * shade), 255);
          break;
      }
      faceMap[py * outW + px] = face;
    }
  }

  if (addOutline) {
    const uint32_t outlineCol = doc::rgba(15, 23, 42, 255);
    const uint32_t outNormCol = doc::rgba(128, 128, 255, 255);
    const uint32_t outDeptCol = doc::rgba(28, 28, 28, 255);

    for (int y = 1; y < outH - 1; ++y) {
      uint32_t* __restrict diffRow =
        reinterpret_cast<uint32_t*>(res.diffImg->getPixelAddress(0, y));
      uint32_t* __restrict normRow =
        reinterpret_cast<uint32_t*>(res.normImg->getPixelAddress(0, y));
      uint32_t* __restrict deptRow =
        reinterpret_cast<uint32_t*>(res.deptImg->getPixelAddress(0, y));

      for (int x = 1; x < outW - 1; ++x) {
        const int idx = y * outW + x;
        const int f = faceMap[idx];
        if (f < 0) {
          if (faceMap[idx - 1] >= 0 || faceMap[idx + 1] >= 0 ||
              faceMap[idx - outW] >= 0 || faceMap[idx + outW] >= 0) {
            diffRow[x] = outlineCol;
            normRow[x] = outNormCol;
            deptRow[x] = outDeptCol;
          }
        }
        else if (f == kFaceTop &&
                 (faceMap[idx + outW] >= 0 && faceMap[idx + outW] != kFaceTop)) {
          const uint32_t cur = diffRow[x];
          diffRow[x] = doc::rgba(
            clampByte(doc::rgba_getr(cur) * 0.68f + 253.0f * 0.32f),
            clampByte(doc::rgba_getg(cur) * 0.68f + 230.0f * 0.32f),
            clampByte(doc::rgba_getb(cur) * 0.68f + 138.0f * 0.32f),
            255);
        }
      }
    }
  }

  return res;
}

static void bake3DToDoc(Doc* doc, int yawDeg = 45, int pitchDeg = 30)
{
  if (!doc || !doc->sprite()) return;
  SixSurfaceData data;
  extractSixSurfaces(doc, doc::frame_t(0), data, true);
  Render3DResult res = compute3DPreviewImages(data, yawDeg, pitchDeg, 50, 50, 50, 0, true, true);
  if (!res.diffImg || !res.normImg || !res.deptImg)
    return;

  doc::Sprite* spr = doc->sprite();
  const doc::frame_t fr = doc::frame_t(0);
  try {
    Tx tx(doc, "Render Easymetric 3D + Normal + Depth Layers", ModifyDocument);
    DocApi api = doc->getApi(tx);

    auto writeLayerImg = [&](const char* layerName, const doc::Image* srcImg) {
      doc::LayerImage* lyr = findOrCreateLayer(doc, api, spr, layerName);
      doc::ImageRef copyImg(doc::Image::createCopy(srcImg));
      if (doc::Cel* cel = lyr->cel(fr))
        api.replaceImage(spr, cel->imageRef(), copyImg);
      else
        api.addCel(lyr, fr, copyImg);
    };

    writeLayerImg("Iso_Depth", res.deptImg.get());
    writeLayerImg("Iso_Normal", res.normImg.get());
    writeLayerImg("Iso_Diffuse", res.diffImg.get());

    tx.commit();
    doc->notifyGeneralUpdate();
  }
  catch (const std::exception& e) {
    // Ignore
  }
}

// Integrated 4-Column 2-Row Workspace Multi-Pane Widget
// Panel editor state: wheel = zoom, never View scroll, regardless
// of the global "zoom with wheel" preference or a customised
// mouse-wheel shortcut table (both of which made the old
// preference-forcing fix unreliable in practice: wheel over a
// surface panel still scrolled the panel and did not zoom).
// StandbyState is kept as the base, so drawing, tools and the
// temporary moving/drawing states all work as before; only
// onMouseWheel is overridden. Zoom is anchored at the mouse
// (not at the centre), so the View scroll only follows the
// zoom anchor instead of jumping, and Shift+wheel (or a
// horizontal wheel) is still horizontal scroll, so scrolling
// is still possible on purpose. Alt+wheel keeps the base
// colour-change behaviour.
class EasymetricWheelZoomState : public StandbyState {
public:
  bool onMouseWheel(Editor* editor, ui::MouseMessage* msg) override
  {
    if (msg->altPressed())
      return StandbyState::onMouseWheel(editor, msg);
    gfx::Point delta = msg->wheelDelta();
    if (msg->shiftPressed() || delta.x != 0) {
      processWheelAction(editor, WheelAction::HScroll, msg->position(),
                         gfx::Point(delta.x, 0), double(delta.x),
                         ScrollBigSteps::Off,
                         (msg->preciseWheel() ? PreciseWheel::On : PreciseWheel::Off),
                         FromMouseWheel::On);
      return true;
    }
    double dz = double(delta.y);
    if (msg->preciseWheel()) {
      dz /= 1.5;
      if (dz < -1.0) dz = -1.0;
      else if (dz > 1.0) dz = 1.0;
    }
    render::Zoom zoom = render::Zoom::fromLinearScale(
      editor->zoom().linearScale() - int(dz));
    editor->setZoomAndCenterInMouse(zoom, msg->position(),
                                    Editor::ZoomBehavior::MOUSE);
    return true;
  }
};

class EasymetricMultiPaneView : public ui::Widget {
public:
  EasymetricMultiPaneView(DocView* docView)
    : ui::Widget(ui::kGenericWidget)
    , m_docView(docView)
    , m_timer(33, this)
  {
    // Wheel = zoom, not scroll, while the studio is open.
    // The preference-forcing below is kept as a fallback for
    // any editor that is not a surface panel, but the surface
    // panels themselves now use EasymetricWheelZoomState (set
    // below), which forces wheel = zoom regardless of the
    // preference or a customised shortcut table. Save the old
    // value and restore it when the studio closes (destructor
    // and the Exit button both restore, idempotently), so normal
    // editing outside Easymetric is untouched. The Preview
    // widget already zooms on wheel and returns the message as
    // handled, so it never scrolls the UI either; Shift+wheel
    // still pans it.
    m_savedZoomWithWheel = Preferences::instance().editor.zoomWithWheel();
    if (!m_savedZoomWithWheel) {
      Preferences::instance().editor.zoomWithWheel(true);
      m_restoreZoomWithWheel = true;
    }

    setExpansive(true);

    // Remember the document selection so the per-face mask
    // confinement in pollAndRender can be undone on exit.
    if (Doc* doc0 = m_docView->document()) {
      if (doc0->mask() && !doc0->mask()->isEmpty())
        m_savedMask.reset(new doc::Mask(*doc0->mask()));
    }

    Doc* doc = m_docView->document();
    doc::Sprite* spr = doc ? doc->sprite() : nullptr;

    // 1. Create the 6 Surface Editors (Top, Bottom, Front, Back, Right, Left)
    for (int f = 0; f < kFaceCount; ++f) {
      m_headers[f] = new PaneHeader(kFaceSpecs[f].displayName,
                                    gfx::rgba(kFaceSpecs[f].r, kFaceSpecs[f].g, kFaceSpecs[f].b));
      Editor* ed = nullptr;
      ui::View* view = m_docView->createPaneEditorView(&ed);
      // Scrollbars on: each surface panel must be scrollable
      // horizontally and vertically on its own, instead of only
      // fitting the whole sprite into the pane. Hiding the bars
      // hid that affordance without disabling it, so show them.
      // The wheel is reserved for zooming (see above), so
      // scrolling needs the bars and drag, not the wheel.
      // fitting the whole sprite into the pane. The editor itself
      // already zooms/scrolls on wheel input; hiding the bars hid
      // that affordance without disabling it, so show them.
      view->showScrollBars();
      view->setExpansive(true);

      m_editors[f] = ed;
      m_views[f] = view;
      // Wheel = zoom in this panel, never View scroll (see
      // EasymetricWheelZoomState). Set after createPaneEditorView
      // so the custom state is the base state the temporary
      // drawing/moving states return to.
      ed->setState(EditorStatePtr(new EasymetricWheelZoomState));

      if (spr) {
        SharedSurfaceLayers shared = findSharedLayers(spr);
        if (shared.baseLayer || shared.textureLayer) {
          // One sheet, one UV window per panel.
          ed->setClipSpriteBounds(sharedFaceRect(spr, f));
        }
        else if (doc::Slice* sl = findSliceForFace(spr, f)) {
          if (sl->getByFrame(0) && !sl->getByFrame(0)->bounds().isEmpty()) {
            ed->setClipSpriteBounds(sl->getByFrame(0)->bounds());
          }
          else {
            ed->setClipSpriteBounds(gfx::Rect(0, 0, spr->width(), spr->height()));
          }
        }
        else {
          ed->setClipSpriteBounds(gfx::Rect(0, 0, spr->width(), spr->height()));
        }
        doc::LayerImage* lyr = shared.textureLayer ? shared.textureLayer : shared.baseLayer;
        if (!lyr) {
          SurfaceLayerPair sp = findSurfaceLayers(spr, f);
          lyr = sp.texLayer ? sp.texLayer : sp.baseLayer;
          if (!lyr) {
            const std::string lyrName = std::string("Surface_") + kFaceSpecs[f].sliceName;
            lyr = findLayerByName(spr, lyrName);
          }
        }
        if (lyr)
          ed->setLayer(lyr);
      }
      ed->setEditorFlags(Editor::EditorFlags((ed->editorFlags() | Editor::kShowOutside) & ~Editor::kShowSlices));

      m_panes[f] = new FacePaneBox(f, m_docView, ed);
      m_panes[f]->addChild(m_headers[f]);
      m_panes[f]->addChild(m_views[f]);
      addChild(m_panes[f]);
    }

    // Attach link toggle buttons to the top face header of each column (Col 0: Top, Col 1: Front, Col 2: Right)
    for (int c = 0; c < 3; ++c) {
      m_linkButtons[c] = new LinkToggleButton();
      m_linkButtons[c]->Click.connect([this, c] { toggleColumnLink(c); });
    }
    m_headers[0]->setLinkButton(m_linkButtons[0]);
    m_headers[2]->setLinkButton(m_linkButtons[1]);
    m_headers[4]->setLinkButton(m_linkButtons[2]);

    // 2. Column 4 Row 0: Preview Pane
    m_headers[kFaceCount] = new PaneHeader("Preview");
    m_previewWidget = new EasymetricPreviewWidget;
    m_previewWidget->setExpansive(true);

    m_panes[kFaceCount] = new ui::VBox;
    m_panes[kFaceCount]->addChild(m_headers[kFaceCount]);
    m_panes[kFaceCount]->addChild(m_previewWidget);
    addChild(m_panes[kFaceCount]);

    // 3. Column 4 Row 1: Fields Pane, scrollable. Previously the
    // Fields box was added directly to the pane, so when its
    // controls grew taller than the pane the bottom ones were
    // simply hidden with no way to reach them. Wrapping it in a
    // View with scrollbars (and wheel scrolling, since this is a
    // control panel, not a canvas) makes every field reachable.
    m_headers[kFaceCount + 1] = new PaneHeader("Fields");
    m_fieldsBox = new FieldsPanelBox;
    m_fieldsView = new ui::View;
    m_fieldsView->showScrollBars();
    m_fieldsView->setExpansive(true);
    m_fieldsView->attachToView(m_fieldsBox);

    buildFieldsControls();

    m_panes[kFaceCount + 1] = new ui::VBox;
    m_panes[kFaceCount + 1]->addChild(m_headers[kFaceCount + 1]);
    m_panes[kFaceCount + 1]->addChild(m_fieldsView);
    addChild(m_panes[kFaceCount + 1]);

    // Timer for real-time 3D model update
    m_timer.Tick.connect([this] { pollAndRender(false); });
    m_timer.start();

    m_lastHash = 0;
    pollAndRender(true);
  }

  ~EasymetricMultiPaneView() override
  {
    m_timer.stop();
    restoreWheelZoomPref();
    if (Doc* doc = m_docView->document()) {
      if (m_savedMask) {
        doc->setMask(m_savedMask.get());
        doc->setMaskVisible(true);
      }
      else {
        doc::Mask empty;
        doc->setMask(&empty);
        doc->setMaskVisible(false);
      }
      doc->notifySelectionChanged();
    }
  }

  void restoreWheelZoomPref()
  {
    if (m_restoreZoomWithWheel) {
      Preferences::instance().editor.zoomWithWheel(m_savedZoomWithWheel);
      m_restoreZoomWithWheel = false;
    }
  }

  std::vector<Editor*> surfaceEditors() const
  {
    return std::vector<Editor*>(m_editors.begin(), m_editors.end());
  }

  void syncOpposingCels(int srcFace, int dstFace)
  {
    Doc* doc = m_docView->document();
    if (!doc || !doc->sprite()) return;
    doc::Sprite* spr = doc->sprite();

    SharedSurfaceLayers shared = findSharedLayers(spr);
    if (shared.baseLayer || shared.textureLayer) {
      // Shared sheets: opposing faces are different UV rects of
      // the SAME layer, so syncing means copying the source rect
      // into the destination rect, not replacing the whole sheet
      // (which would also overwrite the source itself).
      try {
        Tx tx(doc, "Sync Opposing Faces", ModifyDocument);
        DocApi api = doc->getApi(tx);
        const gfx::Rect srcRect = sharedFaceRect(spr, srcFace);
        const gfx::Rect dstRect = sharedFaceRect(spr, dstFace);
        auto syncRect = [&](doc::LayerImage* lyr) {
          if (!lyr) return;
          for (doc::frame_t fr = doc::frame_t(0); fr <= spr->lastFrame(); ++fr) {
            doc::Cel* cel = lyr->cel(fr);
            if (!cel || !cel->image()) continue;
            doc::ImageRef copyImg(doc::Image::createCopy(cel->image()));
            for (int v = 0; v < srcRect.h && v < dstRect.h; ++v)
              for (int u = 0; u < srcRect.w && u < dstRect.w; ++u)
                doc::put_pixel(copyImg.get(), dstRect.x + u, dstRect.y + v,
                               doc::get_pixel(copyImg.get(), srcRect.x + u, srcRect.y + v));
            api.replaceImage(spr, cel->imageRef(), copyImg);
          }
        };
        syncRect(shared.baseLayer);
        syncRect(shared.textureLayer);
        syncRect(shared.depthLayer);
        tx.commit();
        doc->notifyGeneralUpdate();
      }
      catch (const std::exception& e) {
        Console::showException(e);
      }
      return;
    }

    SurfaceLayerPair srcPair = findSurfaceLayers(spr, srcFace);
    SurfaceLayerPair dstPair = findSurfaceLayers(spr, dstFace);

    const std::string srcName = std::string("Surface_") + kFaceSpecs[srcFace].sliceName;
    const std::string dstName = std::string("Surface_") + kFaceSpecs[dstFace].sliceName;
    doc::LayerImage* srcLyr = srcPair.baseLayer ? srcPair.baseLayer : findLayerByName(spr, srcName);
    doc::LayerImage* dstLyr = dstPair.baseLayer ? dstPair.baseLayer : findLayerByName(spr, dstName);
    if (!srcLyr || !dstLyr) return;

    try {
      Tx tx(doc, "Sync Opposing Faces", ModifyDocument);
      DocApi api = doc->getApi(tx);

      auto syncLayers = [&](doc::LayerImage* sL, doc::LayerImage* dL) {
        if (!sL || !dL) return;
        for (doc::frame_t fr = doc::frame_t(0); fr <= spr->lastFrame(); ++fr) {
          doc::Cel* srcCel = sL->cel(fr);
          doc::Cel* dstCel = dL->cel(fr);
          if (srcCel && srcCel->image()) {
            doc::ImageRef copyImg(doc::Image::createCopy(srcCel->image()));
            if (dstCel) {
              api.replaceImage(spr, dstCel->imageRef(), copyImg);
              api.setCelPosition(spr, dstCel, srcCel->x(), srcCel->y());
            }
            else {
              api.addCel(dL, fr, copyImg);
            }
          }
        }
      };

      syncLayers(srcLyr, dstLyr);
      if (srcPair.texLayer && dstPair.texLayer)
        syncLayers(srcPair.texLayer, dstPair.texLayer);

      tx.commit();
      doc->notifyGeneralUpdate();
    }
    catch (const std::exception& e) {
      Console::showException(e);
    }
  }

  void syncAllLinkedCels()
  {
    for (int c = 0; c < 3; ++c) {
      if (m_colLinked[c]) {
        syncOpposingCels(c * 2, c * 2 + 1);
      }
    }
  }

  void toggleColumnLink(int c)
  {
    if (c < 0 || c >= 3) return;
    const int topFace = c * 2;
    const int btmFace = c * 2 + 1;

    m_colLinked[c] = !m_colLinked[c];
    const bool linked = m_colLinked[c];

    if (m_linkButtons[c])
      m_linkButtons[c]->setLinked(linked);

    Doc* doc = m_docView->document();
    doc::Sprite* spr = doc ? doc->sprite() : nullptr;

    if (linked) {
      if (Editor::activeEditor() == m_editors[btmFace]) {
        Editor::_setActiveEditor(m_editors[topFace]);
      }
      if (m_docView->editor() == m_editors[btmFace]) {
        m_docView->setActivePaneEditor(m_editors[topFace]);
      }
      if (spr) {
        SharedSurfaceLayers shared = findSharedLayers(spr);
        if (shared.textureLayer)
          m_editors[topFace]->setLayer(shared.textureLayer);
        else {
          const std::string topLyrName = std::string("Surface_") + kFaceSpecs[topFace].sliceName;
          if (doc::LayerImage* lyr = findLayerByName(spr, topLyrName))
            m_editors[topFace]->setLayer(lyr);
        }
      }

      syncOpposingCels(topFace, btmFace);

      std::string unifiedTitle;
      switch (c) {
        case 0: unifiedTitle = "Top / Bottom"; break;
        case 1: unifiedTitle = "Front / Back"; break;
        case 2: unifiedTitle = "Right / Left"; break;
      }
      m_headers[topFace]->setTitle(unifiedTitle);
    }
    else {
      m_headers[topFace]->setTitle(kFaceSpecs[topFace].displayName);
      m_headers[btmFace]->setTitle(kFaceSpecs[btmFace].displayName);
      syncOpposingCels(topFace, btmFace);
    }

    layout();
    for (int f = 0; f < kFaceCount; ++f) {
      if (m_editors[f] && m_panes[f]->isVisible())
        m_editors[f]->fitClipSpriteBounds();
    }
    invalidate();
    m_lastHash = 0;
    pollAndRender(true);
  }

  void initLayersAndFraming()
  {
    Doc* doc = m_docView->document();
    doc::Sprite* spr = doc ? doc->sprite() : nullptr;
    if (!spr) return;

    for (int f = 0; f < kFaceCount; ++f) {
      if (m_editors[f]) {
        SharedSurfaceLayers shared = findSharedLayers(spr);
        doc::LayerImage* lyr = shared.textureLayer ? shared.textureLayer : shared.baseLayer;
        if (!lyr) {
          SurfaceLayerPair sp = findSurfaceLayers(spr, f);
          lyr = sp.texLayer ? sp.texLayer : sp.baseLayer;
          if (!lyr) {
            const std::string lyrName = std::string("Surface_") + kFaceSpecs[f].sliceName;
            lyr = findLayerByName(spr, lyrName);
          }
        }
        if (lyr)
          m_editors[f]->setLayer(lyr);

        if (shared.baseLayer || shared.textureLayer) {
          m_editors[f]->setClipSpriteBounds(sharedFaceRect(spr, f));
        }
        else if (doc::Slice* sl = findSliceForFace(spr, f)) {
          if (sl->getByFrame(0) && !sl->getByFrame(0)->bounds().isEmpty()) {
            m_editors[f]->setClipSpriteBounds(sl->getByFrame(0)->bounds());
          }
          else {
            m_editors[f]->setClipSpriteBounds(gfx::Rect(0, 0, spr->width(), spr->height()));
          }
        }
        else {
          m_editors[f]->setClipSpriteBounds(gfx::Rect(0, 0, spr->width(), spr->height()));
        }
        m_editors[f]->fitClipSpriteBounds();
      }
    }
  }

protected:
  // One layout source for onResize, onPaint and splitter hit-testing,
  // so the painted handles always line up with the dragged ones.
  // colW[3] is the remainder, so the four columns always fill rc.
  // Splitter gaps are 6*guiscale wide (was 4*): the gap itself
  // is the only area that actually reaches this widget (the panes
  // cover everything else), so a 4*guiscale gap was easy to miss
  // and its cursor indication flickered at the edges. 6*guiscale
  // is the grab area, not just the painted bar.
  // Columns are stored in PIXELS, not fractions of totalW.
  // That is what makes them independent: with fractions, making
  // the window wider (or dragging the Preview boundary, whose
  // width was the remainder of a fraction sum) scaled every
  // column at once, so stretching Preview shrank/grew all
  // three surface columns. With pixel widths, the three
  // surface columns keep exactly their stored widths when
  // the window is resized, and only the Preview column
  // (col 3, the remainder) absorbs the difference. Dragging
  // a boundary changes only the two columns either side of
  // it (see onProcessMessage). The top row height is stored
  // the same way; the Fields row absorbs window height
  // changes. First layout: quarter / half, as before.
  void computePaneLayout(const gfx::Rect& rc, int colX[4], int colW[4],
                         int rowY[2], int rowH[2]) const
  {
    const int s = std::max(1, ui::guiscale());
    const int bw = 6 * s;
    const int bh = 6 * s;
    const int totalW = std::max(16, rc.w - 3 * bw);
    const int totalH = std::max(16, rc.h - bh);
    const int minCol = std::min(64 * s, totalW / 4);
    const int minRow = std::min(64 * s, totalH / 2);

    if (!m_colInit) {
      m_colWpx[0] = totalW / 4;
      m_colWpx[1] = totalW / 4;
      m_colWpx[2] = totalW / 4;
      m_rowHpx = totalH / 2;
      m_colInit = true;
    }

    colW[0] = std::clamp(m_colWpx[0], minCol, std::max(minCol, totalW - 3 * minCol));
    colW[1] = std::clamp(m_colWpx[1], minCol,
                         std::max(minCol, totalW - colW[0] - 2 * minCol));
    colW[2] = std::clamp(m_colWpx[2], minCol,
                         std::max(minCol, totalW - colW[0] - colW[1] - minCol));
    // Narrow window: surface columns alone already exceed the
    // width, so shrink them from the rightmost backwards
    // (Preview keeps its minimum) instead of overflowing.
    int excess = colW[0] + colW[1] + colW[2] + minCol - totalW;
    for (int c = 2; excess > 0 && c >= 0; --c) {
      const int shrink = std::min(excess, colW[c] - minCol);
      colW[c] -= shrink;
      excess -= shrink;
    }
    colW[3] = std::max(minCol, totalW - colW[0] - colW[1] - colW[2]);

    colX[0] = rc.x;
    colX[1] = rc.x + colW[0] + bw;
    colX[2] = rc.x + colW[0] + bw + colW[1] + bw;
    colX[3] = rc.x + colW[0] + bw + colW[1] + bw + colW[2] + bw;

    rowH[0] = std::clamp(m_rowHpx, minRow, std::max(minRow, totalH - minRow));
    rowH[1] = std::max(minRow, totalH - rowH[0]);
    rowY[0] = rc.y;
    rowY[1] = rc.y + rowH[0] + bh;
  }

  // Splitter under the cursor, or 0: 1..3 = vertical boundary before
  // column c, 4 = horizontal boundary between the two rows.
  int splitterAt(const gfx::Point& pos) const
  {
    const gfx::Rect rc = clientBounds();
    const int s = std::max(1, ui::guiscale());
    const int bw = 6 * s;
    const int bh = 6 * s;
    int colX[4], colW[4], rowY[2], rowH[2];
    computePaneLayout(rc, colX, colW, rowY, rowH);
    if (!rc.contains(pos))
      return 0;
    // Vertical gap for column c is [colX[c]-bw, colX[c]), so its
    // centre is colX[c]-bw/2, not colX[c] (the old code centred
    // the hit area on the gap's right edge, so the resize cursor
    // appeared up to half a bar inside the next pane and missed
    // the left half of the painted bar).
    for (int c = 1; c < 4; ++c) {
      if (std::abs(pos.x - (colX[c] - bw / 2)) <= bw / 2 + s)
        return c;
    }
    const int midY = rowY[1] - bh / 2;
    if (std::abs(pos.y - midY) <= bh / 2 + s)
      return 4;
    return 0;
  }

  bool onProcessMessage(ui::Message* msg) override
  {
    switch (msg->type()) {
      case ui::kSetCursorMessage: {
        if (m_dragSplitter != 0) {
          ui::set_mouse_cursor(m_dragSplitter == 4 ? ui::kSizeNSCursor : ui::kSizeWECursor);
          return true;
        }
        auto* mouseMsg = static_cast<ui::MouseMessage*>(msg);
        const int hit = splitterAt(mouseMsg->position());
        if (hit != 0) {
          ui::set_mouse_cursor(hit == 4 ? ui::kSizeNSCursor : ui::kSizeWECursor);
          return true;
        }
        break;
      }
      case ui::kMouseDownMessage: {
        auto* mouseMsg = static_cast<ui::MouseMessage*>(msg);
        const int hit = splitterAt(mouseMsg->position());
        if (hit != 0 && mouseMsg->left()) {
          m_dragSplitter = hit;
          m_dragStart = mouseMsg->position();
          {
            int cx[4], cw[4], ry[2], rh[2];
            computePaneLayout(clientBounds(), cx, cw, ry, rh);
            for (int i = 0; i < 3; ++i)
              m_dragColWpx[i] = cw[i];
            m_dragPreviewW = cw[3];
            m_dragRowHpx = rh[0];
          }
          captureMouse();
          invalidate();
          return true;
        }
        break;
      }
      case ui::kMouseMoveMessage: {
        auto* mouseMsg = static_cast<ui::MouseMessage*>(msg);
        if (m_dragSplitter != 0) {
          const gfx::Rect rc = clientBounds();
          const int s = std::max(1, ui::guiscale());
          const int bw = 6 * s;
          const int bh = 6 * s;
          const int totalW = std::max(16, rc.w - 3 * bw);
          const int totalH = std::max(16, rc.h - bh);
          // Pixel drag, adjacent pair only: the other columns'
          // stored pixel widths are not touched, so stretching
          // Preview (boundary 3) changes only Right/Left + Preview,
          // never the other two surface columns, and a window
          // resize (which only changes totalW) changes only
          // Preview via computePaneLayout.
          const int minColPx = std::min(64 * s, totalW / 4);
          const int minRowPx = std::min(64 * s, totalH / 2);
          if (m_dragSplitter < 4) {
            const int deltaPx = mouseMsg->position().x - m_dragStart.x;
            const int left = m_dragSplitter - 1;
            int pair = 0;
            if (m_dragSplitter < 3)
              pair = m_dragColWpx[left] + m_dragColWpx[m_dragSplitter];
            else
              pair = m_dragColWpx[left] + m_dragPreviewW;
            const int newLeft = std::clamp(m_dragColWpx[left] + deltaPx,
                                           minColPx, pair - minColPx);
            m_colWpx[left] = newLeft;
            if (m_dragSplitter < 3)
              m_colWpx[m_dragSplitter] = pair - newLeft;
            // Boundary 3: Preview is the remainder, so it follows
            // automatically from the new left width.
          }
          else {
            const int deltaPx = mouseMsg->position().y - m_dragStart.y;
            m_rowHpx = std::clamp(m_dragRowHpx + deltaPx,
                                  minRowPx, totalH - minRowPx);
          }
          layout();
          invalidate();
          return true;
        }
        const int hit = splitterAt(mouseMsg->position());
        if (hit != m_hoverSplitter) {
          m_hoverSplitter = hit;
          invalidate();
        }
        break;
      }
      case ui::kMouseUpMessage: {
        if (m_dragSplitter != 0) {
          m_dragSplitter = 0;
          releaseMouse();
          invalidate();
          return true;
        }
        break;
      }
      case ui::kMouseLeaveMessage: {
        if (m_hoverSplitter != 0) {
          m_hoverSplitter = 0;
          invalidate();
        }
        break;
      }
    }
    return ui::Widget::onProcessMessage(msg);
  }

  void onResize(ui::ResizeEvent& ev) override
  {
    setBoundsQuietly(ev.bounds());
    const gfx::Rect rc = childrenBounds();
    const int s = ui::guiscale();
    const int bh = 6 * s;
    int colX[4], colW[4], rowY[2], rowH[2];
    computePaneLayout(rc, colX, colW, rowY, rowH);
    const int totalH = std::max(16, rc.h - bh);

    // Cols 0..2: Opposing face pairs
    for (int c = 0; c < 3; ++c) {
      const int topFace = c * 2;
      const int btmFace = c * 2 + 1;
      if (m_colLinked[c]) {
        m_panes[btmFace]->setVisible(false);
        m_panes[topFace]->setVisible(true);
        m_panes[topFace]->setBounds(gfx::Rect(colX[c], rc.y, colW[c], totalH + bh));
      }
      else {
        m_panes[topFace]->setVisible(true);
        m_panes[btmFace]->setVisible(true);
        m_panes[topFace]->setBounds(gfx::Rect(colX[c], rowY[0], colW[c], rowH[0]));
        m_panes[btmFace]->setBounds(gfx::Rect(colX[c], rowY[1], colW[c], rowH[1]));
      }
    }

    // Col 3: Preview (6), Fields (7)
    m_panes[6]->setVisible(true);
    m_panes[7]->setVisible(true);
    m_panes[6]->setBounds(gfx::Rect(colX[3], rowY[0], colW[3], rowH[0]));
    m_panes[7]->setBounds(gfx::Rect(colX[3], rowY[1], colW[3], rowH[1]));

    // NOTE: deliberately no fitClipSpriteBounds() here. Fitting on
    // every resize reset each surface editor's zoom/scroll whenever
    // a splitter was dragged or the window was resized, which is
    // what made the panels feel unscrollable/unzoomable. Initial
    // fitting still happens in initLayersAndFraming().
  }

  void onPaint(ui::PaintEvent& ev) override
  {
    ui::Graphics* g = ev.graphics();
    const gfx::Rect rc = clientBounds();
    const int s = ui::guiscale();
    const int bw = 6 * s;
    const int bh = 6 * s;
    int colX[4], colW[4], rowY[2], rowH[2];
    computePaneLayout(rc, colX, colW, rowY, rowH);
    const int midY = rowY[1] - bh / 2;

    // Dark charcoal surround & separator lines
    g->fillRect(gfx::rgba(28, 35, 43), rc);

    const gfx::Color sepCol = gfx::rgba(45, 55, 66);
    const gfx::Color gripCol = gfx::rgba(80, 95, 110);
    const gfx::Color activeCol = gfx::rgba(120, 170, 220);

    // Horizontal separators only for columns that are NOT linked (and Col 3)
    for (int c = 0; c < 4; ++c) {
      if (c < 3 && m_colLinked[c])
        continue;
      const bool active = (m_hoverSplitter == 4 || m_dragSplitter == 4);
      g->fillRect(active ? activeCol : sepCol, gfx::Rect(colX[c], midY - bh / 2, colW[c], bh));
    }

    // Vertical column separators, highlighted when hovered/dragged
    for (int c = 1; c < 4; ++c) {
      const int sepX = colX[c] - bw / 2;
      const bool active = (m_hoverSplitter == c || m_dragSplitter == c);
      g->fillRect(active ? activeCol : sepCol, gfx::Rect(sepX - bw / 2, rc.y, bw, rc.h));
      g->fillRect(active ? activeCol : gripCol, gfx::Rect(sepX - 2 * s, midY - 6 * s, 4 * s, s));
      g->fillRect(active ? activeCol : gripCol, gfx::Rect(sepX - 2 * s, midY + 5 * s, 4 * s, s));
    }
  }

private:
  void buildFieldsControls()
  {
    const int s = ui::guiscale();
    m_fieldsBox->setBorder(gfx::Border(4 * s));
    m_fieldsBox->setChildSpacing(3 * s);

    // Row 1: Rotation on Y-axis + Cam Angle
    auto* rotRow = new ui::HBox;
    rotRow->setChildSpacing(4 * s);
    m_yawSlider = new ui::Slider(-180, 180, 45);
    m_pitchSlider = new ui::Slider(-75, 75, 30);
    m_yawSlider->setExpansive(true);
    m_pitchSlider->setExpansive(true);
    rotRow->addChild(new ui::Label("Y-Axis Rot:"));
    rotRow->addChild(m_yawSlider);
    rotRow->addChild(new ui::Label("Cam Angle:"));
    rotRow->addChild(m_pitchSlider);
    m_fieldsBox->addChild(rotRow);

    // Row 2: Pivot X/Y/Z + Center + Base
    auto* pivRow = new ui::HBox;
    pivRow->setChildSpacing(4 * s);
    m_pivXSlider = new ui::Slider(0, 100, 50);
    m_pivYSlider = new ui::Slider(0, 100, 50);
    m_pivZSlider = new ui::Slider(0, 100, 50);
    m_pivXSlider->setExpansive(true);
    m_pivYSlider->setExpansive(true);
    m_pivZSlider->setExpansive(true);
    auto* pivCenterBtn = new ui::Button("Center");
    auto* pivBaseBtn = new ui::Button("Base");
    pivRow->addChild(new ui::Label("Pivot X/Y/Z:"));
    pivRow->addChild(m_pivXSlider);
    pivRow->addChild(m_pivYSlider);
    pivRow->addChild(m_pivZSlider);
    pivRow->addChild(pivCenterBtn);
    pivRow->addChild(pivBaseBtn);
    m_fieldsBox->addChild(pivRow);

    // Row 3: Tag / Angle Preset Dropdown + Save Tag
    auto* tagRow = new ui::HBox;
    tagRow->setChildSpacing(4 * s);
    m_tagCombo = new ui::ComboBox;
    m_tagCombo->setExpansive(true);
    populateTagCombo();
    auto* saveTagBtn = new ui::Button("Save Tag");
    tagRow->addChild(new ui::Label("Tag / Angle:"));
    tagRow->addChild(m_tagCombo);
    tagRow->addChild(saveTagBtn);
    m_fieldsBox->addChild(tagRow);

    // Row 4: Maps Toggles + Render Type Dropdown
    auto* mapRow = new ui::HBox;
    mapRow->setChildSpacing(4 * s);
    m_mapDiffuseCheck = new ui::CheckBox("Diffuse");
    m_mapNormalCheck = new ui::CheckBox("Normal");
    m_mapDepthCheck = new ui::CheckBox("Depth");
    m_mapDiffuseCheck->setSelected(true);

    m_renderTypeCombo = new ui::ComboBox;
    m_renderTypeCombo->addItem("Render: Textured (Lit)");
    m_renderTypeCombo->addItem("Render: Textured (Unlit)");
    m_renderTypeCombo->addItem("Render: Colored (Faces)");
    m_renderTypeCombo->addItem("Render: Normal Shaded");
    m_renderTypeCombo->addItem("Render: Depth Shaded");
    m_renderTypeCombo->addItem("Render: Wireframe + Tex");
    m_renderTypeCombo->setSelectedItemIndex(0);
    m_renderTypeCombo->setExpansive(true);

    mapRow->addChild(new ui::Label("Maps:"));
    mapRow->addChild(m_mapDiffuseCheck);
    mapRow->addChild(m_mapNormalCheck);
    mapRow->addChild(m_mapDepthCheck);
    mapRow->addChild(m_renderTypeCombo);
    m_fieldsBox->addChild(mapRow);

    // Row 5: Stroke Outline + Cutouts + Lock Borders + Symmetry X/Y
    auto* symRow = new ui::HBox;
    symRow->setChildSpacing(4 * s);
    m_strokeCheck = new ui::CheckBox("Stroke Outline");
    m_cutoutCheck = new ui::CheckBox("Cutouts");
    m_lockBordersCheck = new ui::CheckBox("Lock Borders");
    m_symXCheck = new ui::CheckBox("Sym X");
    m_symYCheck = new ui::CheckBox("Sym Y");
    auto* centerSymBtn = new ui::Button("Center Sym");
    m_strokeCheck->setSelected(true);
    m_cutoutCheck->setSelected(true);

    m_linkDimensionsCheck = new ui::CheckBox("Link Dims");
    m_linkDimensionsCheck->setSelected(true);

    symRow->addChild(m_strokeCheck);
    symRow->addChild(m_cutoutCheck);
    symRow->addChild(m_lockBordersCheck);
    symRow->addChild(m_linkDimensionsCheck);
    symRow->addChild(m_symXCheck);
    symRow->addChild(m_symYCheck);
    symRow->addChild(centerSymBtn);
    m_fieldsBox->addChild(symRow);

    // Row 5b: Linked Dimension Controls
    auto* dimRow = new ui::HBox;
    dimRow->setChildSpacing(4 * s);
    auto* faceDimCombo = new ui::ComboBox;
    faceDimCombo->addItem("Top (X/Z)");
    faceDimCombo->addItem("Bottom (X/Z)");
    faceDimCombo->addItem("Front (X/Y)");
    faceDimCombo->addItem("Back (X/Y)");
    faceDimCombo->addItem("Right (Z/Y)");
    faceDimCombo->addItem("Left (Z/Y)");
    faceDimCombo->setSelectedItemIndex(0);

    auto* dimWSlider = new ui::Slider(4, 128, 24);
    auto* dimHSlider = new ui::Slider(4, 128, 24);
    dimWSlider->setExpansive(true);
    dimHSlider->setExpansive(true);
    auto* applyDimBtn = new ui::Button("Apply Size");

    dimRow->addChild(new ui::Label("Size:"));
    dimRow->addChild(faceDimCombo);
    dimRow->addChild(dimWSlider);
    dimRow->addChild(dimHSlider);
    dimRow->addChild(applyDimBtn);
    m_fieldsBox->addChild(dimRow);

    applyDimBtn->Click.connect([this, faceDimCombo, dimWSlider, dimHSlider] {
      Doc* doc = m_docView->document();
      if (!doc || !doc->sprite()) return;
      int selFace = faceDimCombo->getSelectedItemIndex();
      int nw = dimWSlider->getValue();
      int nh = dimHSlider->getValue();
      SixSurfaceData data;
      extractSixSurfaces(doc, 0, data);
      propagateLinkedDimensions(doc, data, selFace, nw, nh);
      initLayersAndFraming();
      m_lastHash = 0;
      pollAndRender(true);
      if (auto* sb = StatusBar::instance())
        sb->showTip(1200, "Propagated linked dimensions across surfaces");
    });

    // Row 6: Per-Face Editable Depth Maps + Live Sync
    auto* depthRow = new ui::HBox;
    depthRow->setChildSpacing(4 * s);
    auto* genFaceDepthBtn = new ui::Button("Render Face Depths (Edit)");
    m_useFaceDepthCheck = new ui::CheckBox("Use Face Depth Layer");
    m_liveBakeCheck = new ui::CheckBox("Live Sync");
    m_useFaceDepthCheck->setSelected(true);

    depthRow->addChild(genFaceDepthBtn);
    depthRow->addChild(m_useFaceDepthCheck);
    depthRow->addChild(m_liveBakeCheck);
    m_fieldsBox->addChild(depthRow);

    // Row 6b: 2-Layer Smart Object & Clipping Structure
    auto* smartRow = new ui::HBox;
    smartRow->setChildSpacing(4 * s);
    auto* setupSmartObjectsBtn = new ui::Button("Setup 2-Layer Smart Objects");
    auto* convertToGroupBtn = new ui::Button("Convert to Groups");
    setupSmartObjectsBtn->setExpansive(true);
    convertToGroupBtn->setExpansive(true);
    smartRow->addChild(setupSmartObjectsBtn);
    smartRow->addChild(convertToGroupBtn);
    m_fieldsBox->addChild(smartRow);

    // Zoom row: buttons + label mirror the preview widget's zoom, so
    // the rendering can also be zoomed without the mouse wheel.
    auto* zoomRow = new ui::HBox;
    zoomRow->setChildSpacing(4 * s);
    auto* zoomOutBtn = new ui::Button("-");
    auto* zoomResetBtn = new ui::Button("100%");
    auto* zoomInBtn = new ui::Button("+");
    m_zoomLabel = new ui::Label("100%");
    zoomRow->addChild(new ui::Label("Preview Zoom:"));
    zoomRow->addChild(zoomOutBtn);
    zoomRow->addChild(zoomInBtn);
    zoomRow->addChild(zoomResetBtn);
    zoomRow->addChild(m_zoomLabel);
    m_fieldsBox->addChild(zoomRow);

    // Row 7: Action Buttons (Render, Export .OBJ, Exit)
    auto* actRow = new ui::HBox;
    actRow->setChildSpacing(4 * s);
    auto* renderBtn = new ui::Button("&Render");
    auto* exportObjBtn = new ui::Button("Export .&OBJ...");
    auto* exitBtn = new ui::Button("E&xit Studio");
    renderBtn->setExpansive(true);
    exportObjBtn->setExpansive(true);

    actRow->addChild(renderBtn);
    actRow->addChild(exportObjBtn);
    actRow->addChild(exitBtn);
    m_fieldsBox->addChild(actRow);

    // Event connections
    auto onParam = [this] {
      m_lastHash = 0;
      pollAndRender(true);
    };

    m_yawSlider->Change.connect(onParam);
    m_pitchSlider->Change.connect(onParam);
    m_pivXSlider->Change.connect(onParam);
    m_pivYSlider->Change.connect(onParam);
    m_pivZSlider->Change.connect(onParam);
    m_mapDiffuseCheck->Click.connect(onParam);
    m_mapNormalCheck->Click.connect(onParam);
    m_mapDepthCheck->Click.connect(onParam);
    m_renderTypeCombo->Change.connect(onParam);
    m_strokeCheck->Click.connect(onParam);
    m_cutoutCheck->Click.connect(onParam);
    m_useFaceDepthCheck->Click.connect(onParam);
    m_liveBakeCheck->Click.connect(onParam);

    m_previewWidget->OrbitDrag.connect([this](int newYaw, int newPitch) {
      m_yawSlider->setValue(newYaw);
      m_pitchSlider->setValue(newPitch);
      m_lastHash = 0;
      pollAndRender(true);
    });

    zoomOutBtn->Click.connect([this] { m_previewWidget->zoomOut(); });
    zoomInBtn->Click.connect([this] { m_previewWidget->zoomIn(); });
    zoomResetBtn->Click.connect([this] { m_previewWidget->resetZoom(); });
    m_previewWidget->ZoomChanged.connect([this](double zoom) {
      if (m_zoomLabel)
        m_zoomLabel->setText(fmt::format("{}%", int(std::round(zoom * 100.0))));
    });

    pivCenterBtn->Click.connect([this] {
      m_pivXSlider->setValue(50);
      m_pivYSlider->setValue(50);
      m_pivZSlider->setValue(50);
      m_lastHash = 0;
      pollAndRender(true);
    });

    pivBaseBtn->Click.connect([this] {
      m_pivXSlider->setValue(50);
      m_pivYSlider->setValue(100);
      m_pivZSlider->setValue(50);
      m_lastHash = 0;
      pollAndRender(true);
    });

    m_tagCombo->Change.connect([this] { onTagSelected(); });
    saveTagBtn->Click.connect([this] { saveCurrentAngleAsTag(); });

    m_lockBordersCheck->Click.connect([this] {
      const bool locked = m_lockBordersCheck->isSelected();
      Doc* doc = m_docView->document();
      if (doc && doc->sprite()) {
        for (int f = 0; f < kFaceCount; ++f) {
          if (doc::Slice* sl = findSliceForFace(doc->sprite(), f))
            sl->userData().setText(locked ? "locked" : "");
        }
        for (Editor* ed : m_editors) {
          if (ed) ed->invalidate();
        }
      }
      if (auto* sb = StatusBar::instance())
        sb->showTip(1200, locked ? "Surface borders locked" : "Surface borders unlocked");
    });

    m_linkDimensionsCheck->Click.connect([this] {
      if (auto* sb = StatusBar::instance()) {
        sb->showTip(1200, m_linkDimensionsCheck->isSelected() ?
                    "Linked surface dimensions active" :
                    "Linked surface dimensions disabled");
      }
    });

    setupSmartObjectsBtn->Click.connect([this] {
      Doc* doc = m_docView->document();
      if (doc) {
        setupTwoLayerStructure(doc, true);
        initLayersAndFraming();
        m_lastHash = 0;
        pollAndRender(true);
        if (auto* sb = StatusBar::instance())
          sb->showTip(1500, "Initialized 2-layer smart object surface hierarchy");
      }
    });

    convertToGroupBtn->Click.connect([this] {
      Doc* doc = m_docView->document();
      if (doc) {
        convertSmartObjectsToGroups(doc);
        initLayersAndFraming();
        m_lastHash = 0;
        pollAndRender(true);
        if (auto* sb = StatusBar::instance())
          sb->showTip(1500, "Converted smart objects to layer groups");
      }
    });

    auto applySymmetry = [this](bool recenter) {
      Doc* doc = m_docView->document();
      if (!doc || !doc->sprite()) return;
      auto& pref = Preferences::instance();
      auto& docPref = pref.document(doc);
      int mode = int(app::gen::SymmetryMode::NONE);
      if (m_symXCheck->isSelected()) mode |= int(app::gen::SymmetryMode::HORIZONTAL);
      if (m_symYCheck->isSelected()) mode |= int(app::gen::SymmetryMode::VERTICAL);
      if (mode != int(app::gen::SymmetryMode::NONE))
        pref.symmetryMode.enabled(true);
      docPref.symmetry.mode(static_cast<app::gen::SymmetryMode>(mode));

      Editor* curEd = m_docView->editor();
      int activeFace = 0;
      for (int f = 0; f < kFaceCount; ++f) {
        if (m_editors[f] == curEd) { activeFace = f; break; }
      }
      const int sprW = doc->sprite()->width();
      const int sprH = doc->sprite()->height();
      if (recenter ||
          docPref.symmetry.xAxis() < 0 || docPref.symmetry.xAxis() > sprW ||
          docPref.symmetry.yAxis() < 0 || docPref.symmetry.yAxis() > sprH) {
        docPref.symmetry.xAxis(sprW * 0.5);
        docPref.symmetry.yAxis(sprH * 0.5);
      }
      for (Editor* ed : m_editors) {
        if (ed) ed->invalidate();
      }
    };

    m_symXCheck->Click.connect([applySymmetry] { applySymmetry(false); });
    m_symYCheck->Click.connect([applySymmetry] { applySymmetry(false); });
    centerSymBtn->Click.connect([applySymmetry] { applySymmetry(true); });

    genFaceDepthBtn->Click.connect([this] { generateEditableFaceDepthLayer(); });
    renderBtn->Click.connect([this] { bakeToActiveDocLayers(); });
    exportObjBtn->Click.connect([this] { exportToWavefrontObj(); });
    exitBtn->Click.connect([this] {
      syncAllLinkedCels();
      restoreWheelZoomPref();
      m_docView->exitMultiPaneMode();
    });
  }

  struct PresetItem {
    std::string label;
    int yaw;
    int pitch;
    int frame;
  };
  std::vector<PresetItem> m_presets;
  bool m_updatingCombo = false;

  void populateTagCombo()
  {
    m_updatingCombo = true;
    m_tagCombo->deleteAllItems();
    m_presets.clear();

    auto add = [&](const std::string& lbl, int yaw, int pitch, int fr = -1) {
      m_presets.push_back({ lbl, yaw, pitch, fr });
      m_tagCombo->addItem(lbl);
    };

    add("Fast Rotation", 45, 30);
    add("Iso 2:1 (Yaw 45°, Cam 30°)", 45, 30);
    add("Front View (Yaw 0°, Cam 0°)", 0, 0);
    add("Back View (Yaw 180°, Cam 0°)", 180, 0);
    add("Right View (Yaw -90°, Cam 0°)", -90, 0);
    add("Left View (Yaw 90°, Cam 0°)", 90, 0);
    add("Top View (Yaw 45°, Cam 60°)", 45, 60);
    add("Bottom View (Yaw 45°, Cam -60°)", 45, -60);
    add("Dimetric (Yaw 30°, Cam 26°)", 30, 26);
    add("Trimetric (Yaw 60°, Cam 35°)", 60, 35);

    Doc* doc = m_docView->document();
    if (doc && doc->sprite()) {
      for (const doc::Tag* tag : doc->sprite()->tags()) {
        int ty = 45, tp = 30;
        const std::string& udata = tag->userData().text();
        if (std::sscanf(udata.c_str(), "easymetric:yaw=%d,pitch=%d", &ty, &tp) != 2) {
          std::sscanf(tag->name().c_str(), "Yaw%d_Cam%d", &ty, &tp);
        }
        add(fmt::format("Tag: {} [f{}..{}] ({}°, {}°)",
                        tag->name(), tag->fromFrame() + 1, tag->toFrame() + 1, ty, tp),
            ty, tp, int(tag->fromFrame()));
      }
    }

    m_tagCombo->setSelectedItemIndex(0);
    m_updatingCombo = false;
  }

  void onTagSelected()
  {
    if (m_updatingCombo) return;
    const int idx = m_tagCombo->getSelectedItemIndex();
    if (idx < 0 || idx >= int(m_presets.size())) return;
    const auto& p = m_presets[idx];
    m_yawSlider->setValue(p.yaw);
    m_pitchSlider->setValue(p.pitch);
    if (p.frame >= 0 && m_docView->editor())
      m_docView->editor()->setFrame(doc::frame_t(p.frame));
    m_lastHash = 0;
    pollAndRender(true);
  }

  void saveCurrentAngleAsTag()
  {
    Doc* doc = m_docView->document();
    if (!doc || !doc->sprite()) return;
    const doc::frame_t fr = (m_docView->editor() ? m_docView->editor()->frame() : doc::frame_t(0));
    const int yaw = m_yawSlider->getValue();
    const int pitch = m_pitchSlider->getValue();
    const std::string tagName = fmt::format("Yaw{}_Cam{}", yaw, pitch);
    const std::string meta = fmt::format("easymetric:yaw={},pitch={}", yaw, pitch);

    try {
      Tx tx(doc, "Save Camera Tag", ModifyDocument);
      doc::Tag* tag = new doc::Tag(fr, fr);
      tag->setName(tagName);
      tag->userData().setText(meta);
      tx(new cmd::AddTag(doc->sprite(), tag));
      tx.commit();
      doc->notifyGeneralUpdate();
      populateTagCombo();
      m_updatingCombo = true;
      m_tagCombo->setSelectedItemIndex(int(m_presets.size()) - 1);
      m_updatingCombo = false;
      if (auto* sb = StatusBar::instance())
        sb->showTip(1500, fmt::format("Saved tag '{}'", tagName));
    }
    catch (const std::exception& e) {
      Console::showException(e);
    }
  }

  void generateEditableFaceDepthLayer()
  {
    Doc* doc = m_docView->document();
    if (!doc || !doc->sprite()) return;
    doc::Sprite* spr = doc->sprite();
    const doc::frame_t fr = (m_docView->editor() ? m_docView->editor()->frame() : doc::frame_t(0));

    SixSurfaceData data;
    extractSixSurfaces(doc, fr, data);

    try {
      Tx tx(doc, "Render Editable Face Depths", ModifyDocument);
      DocApi api = doc->getApi(tx);

      doc::LayerImage* depthLayer = findOrCreateLayer(doc, api, spr, "Surface_DEPTH");
      depthLayer->setVisible(true);
      depthLayer->userData().setColor(doc::rgba(148, 163, 184, 255));

      doc::ImageRef depthImg(doc::Image::create(doc::IMAGE_RGB, spr->width(), spr->height()));
      depthImg->clear(0);

      for (int f = 0; f < kFaceCount; ++f) {
        const int fw = data.w[f];
        const int fh = data.h[f];
        if (fw <= 0 || fh <= 0) continue;

        const float cx = (fw - 1) * 0.5f;
        const float cy = (fh - 1) * 0.5f;
        const float maxR = std::max(1.0f, std::sqrt(cx * cx + cy * cy));

        // Shared sheets: write each face into its own UV cell
        // (data.bounds[f] is that cell when shared layers exist),
        // instead of writing every face at (0, 0) where each
        // face would overwrite the previous one.
        const int ox = (data.bounds[f].w > 0) ? data.bounds[f].x : 0;
        const int oy = (data.bounds[f].w > 0) ? data.bounds[f].y : 0;
        for (int v = 0; v < fh; ++v) {
          if (oy + v < 0 || oy + v >= depthImg->height()) continue;
          for (int u = 0; u < fw; ++u) {
            if (ox + u < 0 || ox + u >= depthImg->width()) continue;
            const uint32_t col = data.sample(f, u, v);
            if (doc::rgba_geta(col) <= 16) continue;

            const float luma = pixelLuma(col);
            const float dist = std::sqrt((u - cx) * (u - cx) + (v - cy) * (v - cy)) / maxR;
            const float dome = std::clamp(1.0f - dist * 0.35f, 0.0f, 1.0f);
            const uint8_t hByte = clampByte((0.35f + luma * 0.45f + dome * 0.20f) * 255.0f);
            doc::put_pixel(depthImg.get(), ox + u, oy + v, doc::rgba(hByte, hByte, hByte, 255));
          }
        }
      }

      if (doc::Cel* cel = depthLayer->cel(fr))
        api.replaceImage(spr, cel->imageRef(), depthImg);
      else
        api.addCel(depthLayer, fr, depthImg);

      tx.commit();
      doc->notifyGeneralUpdate();
      if (auto* sb = StatusBar::instance())
        sb->showTip(2000, "Rendered face depths to 'Surface_DEPTH' layer");
    }
    catch (const std::exception& e) {
      Console::showException(e);
    }
    m_lastHash = 0;
    pollAndRender(true);
  }

  bool extractSixSurfaces(Doc* doc, doc::frame_t frame, SixSurfaceData& out)
  {
    const bool useDepth = (m_useFaceDepthCheck && m_useFaceDepthCheck->isSelected());
    return app::extractSixSurfaces(doc, frame, out, useDepth, m_docView, &m_colLinked);
  }

  void pollAndRender(bool force)
  {
    if (!isVisible()) return;
    Doc* doc = m_docView->document();
    const doc::frame_t frame = (m_docView->editor() ? m_docView->editor()->frame() : doc::frame_t(0));

    // Canvas <-> UV reconciliation, and per-face editing
    // confinement (see below). Both run before extraction so
    // the Preview samples the reconciled UV layout.
    if (doc && doc->sprite()) {
      doc::Sprite* spr = doc->sprite();
      SharedSurfaceLayers shared = findSharedLayers(spr);
      if (shared.baseLayer || shared.textureLayer) {
        // Canvas resize -> UV mapping: if the canvas was
        // resized by hand (sprite size no longer equals the
        // exact sheet size for the current face sizes), scale
        // each face size by the same ratio and rebuild the
        // sheets, so the canvas ends up exactly the sum of
        // the new UV rects (rounding may move it a few pixels
        // from the hand-resized size). If the canvas already
        // matches the exact sheet size, nothing happens here,
        // so a face resize via the dimension controls is not
        // double-applied.
        const gfx::Size exact = sharedSheetSize(spr);
        if (exact.w > 0 && (spr->width() != exact.w || spr->height() != exact.h)) {
          int oldW[kFaceCount], oldH[kFaceCount];
          sharedFaceSizes(spr, oldW, oldH);
          const double sx = (exact.w > 0 && spr->width() != exact.w)
                              ? double(spr->width()) / double(exact.w) : 1.0;
          const double sy = (exact.h > 0 && spr->height() != exact.h)
                              ? double(spr->height()) / double(exact.h) : 1.0;
          int newW[kFaceCount], newH[kFaceCount];
          for (int f = 0; f < kFaceCount; ++f) {
            newW[f] = std::clamp(int(std::round(oldW[f] * sx)), 4, 256);
            newH[f] = std::clamp(int(std::round(oldH[f] * sy)), 4, 256);
          }
          rebuildSharedSheets(doc, newW, newH);
          initLayersAndFraming();
          force = true;
        }

        // Per-face independence: when the active panel changes
        // to a different surface (and that panel is in its
        // standby state, not mid-stroke/mid-move), set the
        // document selection to that face's UV rect, without
        // adding an undo step. Brush painting and a Move are
        // then confined to that face's pixels: moving Top's
        // content does not move Front's, Back's, etc., because
        // the other faces' pixels are outside the selection.
        // It is set only on a face change, so a user selection
        // inside a face (e.g. a sub-region to move) is not
        // overwritten every poll. Limitation, stated honestly:
        // a sub-selection dragged far enough to cross into a
        // neighbouring UV rect will still land in that rect's
        // sampled area, because the sheet is one image; what
        // is guaranteed is that the other faces' pixels do
        // not move with it. On studio exit the original
        // selection is restored (destructor).
        Editor* curEd = m_docView->editor();
        int activeFace = -1;
        for (int f = 0; f < kFaceCount; ++f)
          if (curEd == m_editors[f]) activeFace = f;
        if (activeFace != m_lastActiveFace) {
          if (activeFace >= 0 && curEd &&
              dynamic_cast<StandbyState*>(curEd->getState().get())) {
            doc::Mask mask;
            mask.replace(sharedFaceRect(spr, activeFace));
            doc->setMask(&mask);
            doc->setMaskVisible(true);
            doc->notifySelectionChanged();
            m_lastActiveFace = activeFace;
          }
          else if (activeFace < 0) {
            m_lastActiveFace = -1;
          }
        }
      }
    }

    SixSurfaceData data;
    if (!extractSixSurfaces(doc, frame, data)) {
      data.init(24);
      for (int f = 0; f < kFaceCount; ++f)
        for (int v = 0; v < 24; ++v)
          for (int u = 0; u < 24; ++u)
            data.pixels[f][v * 24 + u] = starterSurfacePixel(f, u, v, 24, 24);
    }

    // Check if any face slice dimensions changed and Link Dims is active
    if (m_linkDimensionsCheck && m_linkDimensionsCheck->isSelected()) {
      int changedFace = -1;
      for (int f = 0; f < kFaceCount; ++f) {
        if (m_lastFaceW[f] > 0 && (data.w[f] != m_lastFaceW[f] || data.h[f] != m_lastFaceH[f])) {
          changedFace = f;
          break;
        }
      }
      if (changedFace >= 0) {
        propagateLinkedDimensions(doc, data, changedFace, data.w[changedFace], data.h[changedFace]);
        initLayersAndFraming();
        for (int f = 0; f < kFaceCount; ++f) {
          m_lastFaceW[f] = data.w[f];
          m_lastFaceH[f] = data.h[f];
        }
        force = true;
      }
      else {
        for (int f = 0; f < kFaceCount; ++f) {
          m_lastFaceW[f] = data.w[f];
          m_lastFaceH[f] = data.h[f];
        }
      }
    }
    else {
      for (int f = 0; f < kFaceCount; ++f) {
        m_lastFaceW[f] = data.w[f];
        m_lastFaceH[f] = data.h[f];
      }
    }

    const int yawDeg = m_yawSlider->getValue();
    const int pitchDeg = m_pitchSlider->getValue();
    const int pivX = m_pivXSlider->getValue();
    const int pivY = m_pivYSlider->getValue();
    const int pivZ = m_pivZSlider->getValue();
    const int renderStyle = std::max(0, m_renderTypeCombo->getSelectedItemIndex());
    const bool addOutline = m_strokeCheck->isSelected();
    const bool allowCutouts = m_cutoutCheck->isSelected();
    const bool showDiff = m_mapDiffuseCheck->isSelected();
    const bool showNorm = m_mapNormalCheck->isSelected();
    const bool showDept = m_mapDepthCheck->isSelected();

    uint64_t h = 14695981039346656037ULL;
    auto mix = [&h](uint64_t v) {
      h ^= v;
      h *= 1099511628211ULL;
    };
    mix(uint64_t(yawDeg + 360));
    mix(uint64_t(pitchDeg + 360));
    mix(uint64_t(pivX));
    mix(uint64_t(pivY));
    mix(uint64_t(pivZ));
    mix(uint64_t(renderStyle));
    mix(uint64_t(addOutline ? 1 : 0));
    mix(uint64_t(allowCutouts ? 1 : 0));
    mix(uint64_t(showDiff ? 1 : 0));
    mix(uint64_t(showNorm ? 1 : 0));
    mix(uint64_t(showDept ? 1 : 0));

    for (int f = 0; f < kFaceCount; ++f) {
      mix(uint64_t(data.w[f]));
      mix(uint64_t(data.h[f]));
      const uint32_t* p = data.pixels[f].data();
      const size_t n = data.pixels[f].size();
      for (size_t i = 0; i < n; ++i)
        mix(p[i]);
      if (data.hasCustomDepthLayer) {
        const uint8_t* dp = data.depthPixels[f].data();
        for (size_t i = 0; i < n; ++i)
          mix(dp[i]);
      }
    }

    if (!force && h == m_lastHash)
      return;
    m_lastHash = h;

    render3DModel(data, yawDeg, pitchDeg, pivX, pivY, pivZ,
                  renderStyle, addOutline, allowCutouts,
                  showDiff, showNorm, showDept);

    if (m_liveBakeCheck->isSelected() && doc && doc->sprite())
      bakeToActiveDocLayers();
  }

  void render3DModel(const SixSurfaceData& data,
                     int yawDeg,
                     int pitchDeg,
                     int pivX,
                     int pivY,
                     int pivZ,
                     int renderStyle,
                     bool addOutline,
                     bool allowCutouts,
                     bool showDiff,
                     bool showNorm,
                     bool showDept)
  {
    Render3DResult res = compute3DPreviewImages(data, yawDeg, pitchDeg, pivX, pivY, pivZ,
                                                renderStyle, addOutline, allowCutouts);
    m_lastDiffImg = res.diffImg;
    m_lastNormImg = res.normImg;
    m_lastDeptImg = res.deptImg;

    if (!m_previewWidget)
      return;

    auto makeSurface = [](const doc::Image* img) -> os::SurfaceRef {
      if (!img) return nullptr;
      const int w = img->width();
      const int h = img->height();
      os::SurfaceRef surf = os::System::instance()->makeRgbaSurface(w, h);
      convert_image_to_surface(img, nullptr, surf.get(), 0, 0, 0, 0, w, h);
      return surf;
    };

    m_previewWidget->setSurfaces(
      makeSurface(m_lastDiffImg.get()),
      makeSurface(m_lastNormImg.get()),
      makeSurface(m_lastDeptImg.get()),
      yawDeg,
      pitchDeg,
      int(res.cxScreen),
      int(res.cyScreen),
      showDiff,
      showNorm,
      showDept);
  }

  void bakeToActiveDocLayers()
  {
    Doc* doc = m_docView->document();
    if (!doc || !doc->sprite() || !m_lastDiffImg || !m_lastNormImg || !m_lastDeptImg)
      return;

    doc::Sprite* spr = doc->sprite();
    const doc::frame_t fr = (m_docView->editor() ? m_docView->editor()->frame() : doc::frame_t(0));

    try {
      Tx tx(doc, "Render Easymetric 3D + Normal + Depth Layers", ModifyDocument);
      DocApi api = doc->getApi(tx);

      auto writeLayerImg = [&](const char* layerName, const doc::Image* srcImg) {
        doc::LayerImage* lyr = findOrCreateLayer(doc, api, spr, layerName);
        doc::ImageRef copyImg(doc::Image::createCopy(srcImg));
        if (doc::Cel* cel = lyr->cel(fr))
          api.replaceImage(spr, cel->imageRef(), copyImg);
        else
          api.addCel(lyr, fr, copyImg);
      };

      const bool anyChecked = m_mapDiffuseCheck->isSelected() ||
                              m_mapNormalCheck->isSelected() ||
                              m_mapDepthCheck->isSelected();
      if (!anyChecked || m_mapDepthCheck->isSelected())
        writeLayerImg("Iso_Depth", m_lastDeptImg.get());
      if (!anyChecked || m_mapNormalCheck->isSelected())
        writeLayerImg("Iso_Normal", m_lastNormImg.get());
      if (!anyChecked || m_mapDiffuseCheck->isSelected())
        writeLayerImg("Iso_Diffuse", m_lastDiffImg.get());

      for (int c = 0; c < 3; ++c) {
        if (m_colLinked[c]) {
          const int srcFace = c * 2;
          const int dstFace = c * 2 + 1;
          const std::string srcName = std::string("Surface_") + kFaceSpecs[srcFace].sliceName;
          const std::string dstName = std::string("Surface_") + kFaceSpecs[dstFace].sliceName;
          doc::LayerImage* srcLyr = findLayerByName(spr, srcName);
          doc::LayerImage* dstLyr = findLayerByName(spr, dstName);
          if (srcLyr && dstLyr) {
            doc::Cel* srcCel = srcLyr->cel(fr);
            doc::Cel* dstCel = dstLyr->cel(fr);
            if (srcCel && srcCel->image()) {
              doc::ImageRef copyImg(doc::Image::createCopy(srcCel->image()));
              if (dstCel) {
                api.replaceImage(spr, dstCel->imageRef(), copyImg);
                api.setCelPosition(spr, dstCel, srcCel->x(), srcCel->y());
              }
              else {
                api.addCel(dstLyr, fr, copyImg);
              }
            }
          }
        }
      }

      tx.commit();
      doc->notifyGeneralUpdate();
      if (auto* sb = StatusBar::instance())
        sb->showTip(1500, "Rendered 3D model to Iso_Diffuse / Iso_Normal / Iso_Depth layers");
    }
    catch (const std::exception& e) {
      Console::showException(e);
    }
  }

  void exportToWavefrontObj()
  {
    Doc* doc = m_docView->document();
    if (!doc || !doc->sprite()) return;

    std::string defaultPath = doc->filename();
    if (defaultPath.empty())
      defaultPath = "easymetric_model.obj";
    else
      defaultPath = base::replace_extension(defaultPath, "obj");

    base::paths exts = { "obj" };
    base::paths selected;
    if (!show_file_selector("Export Easymetric 3D Model as Wavefront .OBJ",
                            defaultPath,
                            exts,
                            FileSelectorType::Save,
                            selected) ||
        selected.empty()) {
      return;
    }

    const std::string objPath = base::replace_extension(selected.front(), "obj");
    const std::string mtlPath = base::replace_extension(objPath, "mtl");
    const std::string texPath = base::replace_extension(objPath, "png");
    const std::string mtlFileName = base::get_file_name(mtlPath);
    const std::string texFileName = base::get_file_name(texPath);

    const doc::frame_t fr = (m_docView->editor() ? m_docView->editor()->frame() : doc::frame_t(0));
    SixSurfaceData data;
    extractSixSurfaces(doc, fr, data);

    // Texture atlas
    {
      doc::FramesSequence seq;
      seq.insert(fr);
      std::unique_ptr<FileOp> fop(FileOp::createSaveDocumentOperation(
        UIContext::instance(),
        FileOpROI(doc, doc->sprite()->bounds(), "", "", seq, false),
        texPath,
        "",
        false));
      if (fop && !fop->hasError()) {
        fop->operate();
        fop->done();
      }
    }

    // MTL file
    {
      std::ofstream mtl(mtlPath);
      if (mtl.is_open()) {
        mtl << "# Aseprite 2.0 Easymetric Material\n"
            << "newmtl EasymetricMat\n"
            << "Ka 1.000 1.000 1.000\n"
            << "Kd 1.000 1.000 1.000\n"
            << "Ks 0.000 0.000 0.000\n"
            << "d 1.0\n"
            << "illum 1\n"
            << "map_Kd " << texFileName << "\n";
      }
    }

    // OBJ file
    {
      std::ofstream obj(objPath);
      if (!obj.is_open()) return;

      const int pivX = m_pivXSlider ? m_pivXSlider->getValue() : 50;
      const int pivY = m_pivYSlider ? m_pivYSlider->getValue() : 50;
      const int pivZ = m_pivZSlider ? m_pivZSlider->getValue() : 50;
      SculptedVoxelVolume vol = buildSculptedVolume(data, pivX, pivY, pivZ);

      const float sprW = std::max(1.0f, float(doc->sprite()->width()));
      const float sprH = std::max(1.0f, float(doc->sprite()->height()));
      const float halfX = std::max({ data.w[kFaceFront], data.w[kFaceBack], data.w[kFaceTop], data.w[kFaceDown] }) * 0.5f;
      const float halfY = std::max({ data.h[kFaceFront], data.h[kFaceBack], data.h[kFaceLeft], data.h[kFaceRight] }) * 0.5f;
      const float halfZ = std::max({ data.w[kFaceLeft], data.w[kFaceRight], data.h[kFaceTop], data.h[kFaceDown] }) * 0.5f;

      const float cellSizeX = (2.0f * halfX) / float(vol.vx);
      const float cellSizeY = (2.0f * halfY) / float(vol.vy);
      const float cellSizeZ = (2.0f * halfZ) / float(vol.vz);

      obj << "# Aseprite 2.0 Easymetric Sculpted 3D Model\n"
          << "mtllib " << mtlFileName << "\n"
          << "usemtl EasymetricMat\n"
          << "o EasymetricSculptedModel\n";

      int vIdx = 1, vtIdx = 1, vnIdx = 1;

      auto emitQuad = [&](float x0, float y0, float z0,
                          float x1, float y1, float z1,
                          float x2, float y2, float z2,
                          float x3, float y3, float z3,
                          float nx, float ny, float nz,
                          float u0, float v0, float u1, float v1) {
        obj << "v " << x0 << " " << y0 << " " << z0 << "\n"
            << "v " << x1 << " " << y1 << " " << z1 << "\n"
            << "v " << x2 << " " << y2 << " " << z2 << "\n"
            << "v " << x3 << " " << y3 << " " << z3 << "\n";
        obj << "vt " << u0 << " " << v0 << "\n"
            << "vt " << u1 << " " << v0 << "\n"
            << "vt " << u1 << " " << v1 << "\n"
            << "vt " << u0 << " " << v1 << "\n";
        obj << "vn " << nx << " " << ny << " " << nz << "\n";
        obj << "f "
            << (vIdx + 0) << "/" << (vtIdx + 0) << "/" << vnIdx << " "
            << (vIdx + 1) << "/" << (vtIdx + 1) << "/" << vnIdx << " "
            << (vIdx + 2) << "/" << (vtIdx + 2) << "/" << vnIdx << " "
            << (vIdx + 3) << "/" << (vtIdx + 3) << "/" << vnIdx << "\n";
        vIdx += 4;
        vtIdx += 4;
        vnIdx += 1;
      };

      for (int vy = 0; vy < vol.vy; ++vy) {
        const float yMax = halfY - float(vy) * cellSizeY;
        const float yMin = halfY - float(vy + 1) * cellSizeY;

        for (int vz = 0; vz < vol.vz; ++vz) {
          const float zMin = -halfZ + float(vz) * cellSizeZ;
          const float zMax = -halfZ + float(vz + 1) * cellSizeZ;

          for (int vx = 0; vx < vol.vx; ++vx) {
            if (!vol.isSolid(vx, vy, vz)) continue;

            const float xMin = -halfX + float(vx) * cellSizeX;
            const float xMax = -halfX + float(vx + 1) * cellSizeX;

            // 1. Front face (+Z)
            if (vz + 1 >= vol.vz || !vol.isSolid(vx, vy, vz + 1)) {
              const float u0 = float(vx) / float(vol.vx);
              const float u1 = float(vx + 1) / float(vol.vx);
              const float v0 = 1.0f - float(vy + 1) / float(vol.vy);
              const float v1 = 1.0f - float(vy) / float(vol.vy);
              emitQuad(xMin, yMin, zMax,
                       xMax, yMin, zMax,
                       xMax, yMax, zMax,
                       xMin, yMax, zMax,
                       0.0f, 0.0f, 1.0f,
                       u0, v0, u1, v1);
            }

            // 2. Back face (-Z)
            if (vz - 1 < 0 || !vol.isSolid(vx, vy, vz - 1)) {
              const float u0 = float(vol.vx - 1 - vx) / float(vol.vx);
              const float u1 = float(vol.vx - vx) / float(vol.vx);
              const float v0 = 1.0f - float(vy + 1) / float(vol.vy);
              const float v1 = 1.0f - float(vy) / float(vol.vy);
              emitQuad(xMax, yMin, zMin,
                       xMin, yMin, zMin,
                       xMin, yMax, zMin,
                       xMax, yMax, zMin,
                       0.0f, 0.0f, -1.0f,
                       u0, v0, u1, v1);
            }

            // 3. Top face (+Y)
            if (vy - 1 < 0 || !vol.isSolid(vx, vy - 1, vz)) {
              const float u0 = float(vx) / float(vol.vx);
              const float u1 = float(vx + 1) / float(vol.vx);
              const float v0 = 1.0f - float(vz + 1) / float(vol.vz);
              const float v1 = 1.0f - float(vz) / float(vol.vz);
              emitQuad(xMin, yMax, zMax,
                       xMax, yMax, zMax,
                       xMax, yMax, zMin,
                       xMin, yMax, zMin,
                       0.0f, 1.0f, 0.0f,
                       u0, v0, u1, v1);
            }

            // 4. Bottom face (-Y)
            if (vy + 1 >= vol.vy || !vol.isSolid(vx, vy + 1, vz)) {
              const float u0 = float(vx) / float(vol.vx);
              const float u1 = float(vx + 1) / float(vol.vx);
              const float v0 = 1.0f - float(vol.vz - vz) / float(vol.vz);
              const float v1 = 1.0f - float(vol.vz - 1 - vz) / float(vol.vz);
              emitQuad(xMin, yMin, zMin,
                       xMax, yMin, zMin,
                       xMax, yMin, zMax,
                       xMin, yMin, zMax,
                       0.0f, -1.0f, 0.0f,
                       u0, v0, u1, v1);
            }

            // 5. Right face (+X)
            if (vx + 1 >= vol.vx || !vol.isSolid(vx + 1, vy, vz)) {
              const float u0 = float(vol.vz - 1 - vz) / float(vol.vz);
              const float u1 = float(vol.vz - vz) / float(vol.vz);
              const float v0 = 1.0f - float(vy + 1) / float(vol.vy);
              const float v1 = 1.0f - float(vy) / float(vol.vy);
              emitQuad(xMax, yMin, zMax,
                       xMax, yMin, zMin,
                       xMax, yMax, zMin,
                       xMax, yMax, zMax,
                       1.0f, 0.0f, 0.0f,
                       u0, v0, u1, v1);
            }

            // 6. Left face (-X)
            if (vx - 1 < 0 || !vol.isSolid(vx - 1, vy, vz)) {
              const float u0 = float(vz) / float(vol.vz);
              const float u1 = float(vz + 1) / float(vol.vz);
              const float v0 = 1.0f - float(vy + 1) / float(vol.vy);
              const float v1 = 1.0f - float(vy) / float(vol.vy);
              emitQuad(xMin, yMin, zMin,
                       xMin, yMin, zMax,
                       xMin, yMax, zMax,
                       xMin, yMax, zMin,
                       -1.0f, 0.0f, 0.0f,
                       u0, v0, u1, v1);
            }
          }
        }
      }
    }

    if (auto* sb = StatusBar::instance())
      sb->showTip(2500, fmt::format("Exported 3D Wavefront OBJ to {}", objPath));
  }

  DocView* m_docView;
  std::array<PaneHeader*, 8> m_headers;
  std::array<ui::VBox*, 8> m_panes;

  // 6 Surface Editors
  std::array<Editor*, kFaceCount> m_editors;
  std::array<ui::View*, kFaceCount> m_views;
  std::array<LinkToggleButton*, 3> m_linkButtons;
  std::array<bool, 3> m_colLinked = { false, false, false };

  // Preview & Fields
  EasymetricPreviewWidget* m_previewWidget;
  FieldsPanelBox* m_fieldsBox;
  ui::View* m_fieldsView = nullptr;
  bool m_savedZoomWithWheel = true;
  bool m_restoreZoomWithWheel = false;

  // Fields controls
  ui::Slider* m_yawSlider;
  ui::Slider* m_pitchSlider;
  ui::Slider* m_pivXSlider;
  ui::Slider* m_pivYSlider;
  ui::Slider* m_pivZSlider;
  ui::ComboBox* m_tagCombo;
  ui::CheckBox* m_mapDiffuseCheck;
  ui::CheckBox* m_mapNormalCheck;
  ui::CheckBox* m_mapDepthCheck;
  ui::ComboBox* m_renderTypeCombo;
  ui::Label* m_zoomLabel = nullptr;
  ui::CheckBox* m_strokeCheck;
  ui::CheckBox* m_cutoutCheck;
  ui::CheckBox* m_lockBordersCheck;
  ui::CheckBox* m_symXCheck;
  ui::CheckBox* m_symYCheck;
  ui::CheckBox* m_useFaceDepthCheck;
  ui::CheckBox* m_liveBakeCheck;
  ui::CheckBox* m_linkDimensionsCheck;

  int m_lastFaceW[kFaceCount] = { 0, 0, 0, 0, 0, 0 };
  int m_lastFaceH[kFaceCount] = { 0, 0, 0, 0, 0, 0 };

  // Splitter state: pixel widths for cols 0..2 (col 3 is the
  // remainder), top-row pixel height, and hover/drag tracking.
  // 0 in m_hoverSplitter/m_dragSplitter means no splitter.
  // m_colWpx/m_rowHpx are mutable because computePaneLayout
  // (const, called from paint/hit-testing) lazily initialises
  // them on first layout; after that only a drag changes them.
  mutable int m_colWpx[3] = { 0, 0, 0 };
  mutable int m_rowHpx = 0;
  mutable bool m_colInit = false;
  int m_hoverSplitter = 0;
  int m_dragSplitter = 0;
  gfx::Point m_dragStart;
  int m_dragColWpx[3] = { 0, 0, 0 };
  int m_dragPreviewW = 0;
  int m_dragRowHpx = 0;
  int m_lastActiveFace = -1;
  doc::MaskRef m_savedMask;

  ui::Timer m_timer;
  uint64_t m_lastHash = 0;
  doc::ImageRef m_lastDiffImg;
  doc::ImageRef m_lastNormImg;
  doc::ImageRef m_lastDeptImg;
};

class EasymetricStudioCommand : public Command {
public:
  EasymetricStudioCommand() : Command(CommandId::EasymetricStudio()) {}

protected:
  void onLoadParams(const Params& params) override
  {
    m_params = params;
  }

  bool onEnabled(Context* context) override
  {
    return context && context->activeDocument();
  }

  bool onChecked(Context* context) override
  {
    if (!UIContext::instance())
      return false;
    DocView* view = UIContext::instance()->activeView();
    return view && view->isMultiPane();
  }

  void onExecute(Context* context) override
  {
    Doc* doc = context ? static_cast<Doc*>(context->activeDocument()) : nullptr;
    if (!doc || !doc->sprite())
      return;

    // Headless / Parameterized Execution
    if (m_params.has_param("action")) {
      const std::string action = m_params.get("action");
      if (action == "linkDims") {
        std::string faceStr = m_params.has_param("face") ? m_params.get("face") : "top";
        int w = m_params.has_param("width") ? std::stoi(m_params.get("width")) : 32;
        int h = m_params.has_param("height") ? std::stoi(m_params.get("height")) : 32;
        int faceIdx = kFaceTop;
        if (faceStr == "top" || faceStr == "TOP") faceIdx = kFaceTop;
        else if (faceStr == "down" || faceStr == "bottom" || faceStr == "DOWN") faceIdx = kFaceDown;
        else if (faceStr == "front" || faceStr == "FRONT") faceIdx = kFaceFront;
        else if (faceStr == "back" || faceStr == "BACK") faceIdx = kFaceBack;
        else if (faceStr == "right" || faceStr == "RIGHT") faceIdx = kFaceRight;
        else if (faceStr == "left" || faceStr == "LEFT") faceIdx = kFaceLeft;

        propagateLinkedDimensions(doc, faceIdx, w, h);
      }
      else if (action == "setupSmartObjects") {
        setupTwoLayerStructure(doc, true);
      }
      else if (action == "convertToGroups") {
        convertSmartObjectsToGroups(doc);
      }
      else if (action == "setupSurfaces") {
        setupSixSurfacesOnDoc(doc, true);
      }
      else if (action == "setupGroups") {
        setupTwoLayerStructure(doc, false);
      }
      else if (action == "bake3D" || action == "render3D") {
        int yaw = m_params.has_param("yaw") ? std::stoi(m_params.get("yaw")) : 45;
        int pitch = m_params.has_param("pitch") ? std::stoi(m_params.get("pitch")) : 30;
        bake3DToDoc(doc, yaw, pitch);
      }
      return;
    }

    if (!context->isUIAvailable())
      return;

    UIContext* uiCtx = UIContext::instance();
    DocView* docView = uiCtx ? uiCtx->activeView() : nullptr;
    if (!docView || !docView->document())
      return;

    if (docView->isMultiPane()) {
      if (auto* multiPane = dynamic_cast<EasymetricMultiPaneView*>(docView->multiPaneLayout())) {
        multiPane->syncAllLinkedCels();
      }
      docView->exitMultiPaneMode();
      if (auto* sb = StatusBar::instance())
        sb->showTip(1200, "Easymetric 6-Surface Studio: Switched to standard view");
    }
    else {
      setupSixSurfacesOnDoc(docView->document(), true);
      auto* multiPane = new EasymetricMultiPaneView(docView);
      std::vector<Editor*> editors = multiPane->surfaceEditors();
      docView->enterMultiPaneMode(multiPane, editors);
      multiPane->initLayersAndFraming();
      if (auto* sb = StatusBar::instance())
        sb->showTip(1500, "Easymetric 6-Surface Studio: 4-Column Workspace Grid Active");
    }
  }

private:
  Params m_params;
};

} // anonymous namespace

Command* CommandFactory::createEasymetricStudioCommand()
{
  return new EasymetricStudioCommand;
}

} // namespace app
