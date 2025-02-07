// Aseprite
// Copyright (c) 2020-2024 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_OCTREEMAP_H_INCLUDED
#define DOC_OCTREEMAP_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/rgbmap_base.h"

#include <array>
#include <memory>
#include <vector>

// When this DOC_OCTREE_IS_OPAQUE 'color' is asociated with
// some variable which represents a mask color, it tells us that
// there isn't any transparent color in the sprite, i.e.
// there is a background layer in the sprite.
#define DOC_OCTREE_IS_OPAQUE 0x00FFFFFF

namespace doc {

class OctreeNode;
class OctreeMap;

using OctreeNodes = std::vector<OctreeNode*>;

class OctreeNode {
private:
  class LeafColor {
  public:
    LeafColor() : m_r(0), m_g(0), m_b(0), m_a(0), m_pixelCount(0) {}

    LeafColor(int r, int g, int b, int a, size_t pixelCount)
      : m_r((double)r)
      , m_g((double)g)
      , m_b((double)b)
      , m_a((double)a)
      , m_pixelCount(pixelCount)
    {
    }

    void add(color_t c)
    {
      m_r += rgba_getr(c);
      m_g += rgba_getg(c);
      m_b += rgba_getb(c);
      m_a += rgba_geta(c);
      ++m_pixelCount;
    }

    void add(LeafColor leafColor)
    {
      m_r += leafColor.m_r;
      m_g += leafColor.m_g;
      m_b += leafColor.m_b;
      m_a += leafColor.m_a;
      m_pixelCount += leafColor.m_pixelCount;
    }

    color_t rgbaColor() const
    {
      int auxR = (((int)m_r) % m_pixelCount > m_pixelCount / 2) ? 1 : 0;
      int auxG = (((int)m_g) % m_pixelCount > m_pixelCount / 2) ? 1 : 0;
      int auxB = (((int)m_b) % m_pixelCount > m_pixelCount / 2) ? 1 : 0;
      int auxA = (((int)m_a) % m_pixelCount > m_pixelCount / 2) ? 1 : 0;
      return rgba(int(m_r / m_pixelCount + auxR),
                  int(m_g / m_pixelCount + auxG),
                  int(m_b / m_pixelCount + auxB),
                  int(m_a / m_pixelCount + auxA));
    }

    size_t pixelCount() const { return m_pixelCount; }

  private:
    double m_r;
    double m_g;
    double m_b;
    double m_a;
    size_t m_pixelCount;
  };

public:
  OctreeNode* parent() const { return m_parent; }
  bool hasChildren() const { return m_children != nullptr; }
  LeafColor leafColor() const { return m_leafColor; }

  void addColor(color_t c, int level, OctreeNode* parent, int paletteIndex = 0, int levelDeep = 7);

  int mapColor(int r,
               int g,
               int b,
               int a,
               int mask_index,
               const Palette* palette,
               int level,
               const OctreeMap* octree) const;

  void collectLeafNodes(OctreeNodes& leavesVector, int& paletteIndex);

  // removeLeaves(): remove leaves from a common parent
  // auxParentVector: i/o addreess of an auxiliary parent leaf Vector from outside.
  // rootLeavesVector: i/o address of the m_root->m_leavesVector
  int removeLeaves(OctreeNodes& auxParentVector, OctreeNodes& rootLeavesVector);

private:
  bool isLeaf() { return m_leafColor.pixelCount() > 0; }
  void paletteIndex(int index) { m_paletteIndex = index; }

  static int getHextet(color_t c, int level);
  static int getHextet(int r, int g, int b, int a, int level);
  static color_t hextetToBranchColor(int hextet, int level);

  LeafColor m_leafColor;
  mutable int m_paletteIndex = -1;
  mutable std::unique_ptr<std::array<OctreeNode, 16>> m_children;
  OctreeNode* m_parent = nullptr;
};

class OctreeMap : public RgbMapBase {
public:
  void addColor(color_t color, int levelDeep = 7)
  {
    m_root.addColor(color, 0, &m_root, 0, levelDeep);
  }

  // makePalette returns true if a 7 level octreeDeep is OK, and false
  // if we can add ONE level deep.
  bool makePalette(Palette* palette, int colorCount, const int levelDeep = 7);

  void feedWithImage(const Image* image,
                     const bool withAlpha,
                     const color_t maskColor,
                     const int levelDeep = 7);

  // RgbMap impl
  void regenerateMap(const Palette* palette,
                     const int maskIndex,
                     const FitCriteria fitCriteria) override;
  void regenerateMap(const Palette* palette, const int maskIndex) override
  {
    regenerateMap(palette, maskIndex, m_fitCriteria);
  }

  int mapColor(color_t rgba) const override;

  RgbMapAlgorithm rgbmapAlgorithm() const override { return RgbMapAlgorithm::OCTREE; }

private:
  OctreeNode m_root;
  OctreeNodes m_leavesVector;
  color_t m_maskColor = 0;
};

} // namespace doc
#endif
