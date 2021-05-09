// Aseprite
// Copyright (c) 2020-2021 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_OCTREEMAP_H_INCLUDED
#define DOC_OCTREEMAP_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "doc/image_impl.h"
#include "doc/palette.h"
#include "doc/rgbmap.h"

#include <array>
#include <memory>
#include <vector>

namespace doc {

class OctreeNode;
using OctreeNodes = std::vector<OctreeNode*>;

class OctreeNode {
private:
  class LeafColor {
  public:
    LeafColor() :
      m_r(0),
      m_g(0),
      m_b(0),
      m_pixelCount(0) {
    }

    LeafColor(int r, int g, int b, size_t pixelCount) :
      m_r((double)r),
      m_g((double)g),
      m_b((double)b),
      m_pixelCount(pixelCount) {
    }

    void add(color_t c) {
      m_r += rgba_getr(c);
      m_g += rgba_getg(c);
      m_b += rgba_getb(c);
      ++m_pixelCount;
    }

    void add(LeafColor leafColor) {
      m_r += leafColor.m_r;
      m_g += leafColor.m_g;
      m_b += leafColor.m_b;
      m_pixelCount += leafColor.m_pixelCount;
    }

    color_t rgbaColor() const {
      int auxR = (((int)m_r) % m_pixelCount > m_pixelCount / 2) ? 1: 0;
      int auxG = (((int)m_g) % m_pixelCount > m_pixelCount / 2) ? 1: 0;
      int auxB = (((int)m_b) % m_pixelCount > m_pixelCount / 2) ? 1: 0;
      return rgba(int(m_r / m_pixelCount + auxR),
                  int(m_g / m_pixelCount + auxG),
                  int(m_b / m_pixelCount + auxB), 255);
    }

    size_t pixelCount() const { return m_pixelCount; }

private:
    double m_r;
    double m_g;
    double m_b;
    size_t m_pixelCount;
  };

public:
  OctreeNode* parent() const { return m_parent; }
  bool hasChildren() const { return m_children != nullptr; }
  LeafColor leafColor() const { return m_leafColor; }

  void addColor(color_t c, int level, OctreeNode* parent,
                int paletteIndex = 0, int levelDeep = 7);

  void fillOrphansNodes(const Palette* palette,
                        const color_t upstreamBranchColor,
                        const int level);

  void fillMostSignificantNodes(int level);

  int mapColor(int r, int g, int b, int level) const;

  void collectLeafNodes(OctreeNodes& leavesVector, int& paletteIndex);

  // removeLeaves(): remove leaves from a common parent
  // auxParentVector: i/o addreess of an auxiliary parent leaf Vector from outside.
  // rootLeavesVector: i/o address of the m_root->m_leavesVector
  int removeLeaves(OctreeNodes& auxParentVector,
                   OctreeNodes& rootLeavesVector,
                   int octreeDeep = 7);

private:
  bool isLeaf() { return m_leafColor.pixelCount() > 0; }
  void paletteIndex(int index) { m_paletteIndex = index; }

  static int getOctet(color_t c, int level);
  static color_t octetToBranchColor(int octet, int level);

  LeafColor m_leafColor;
  int m_paletteIndex = 0;
  std::unique_ptr<std::array<OctreeNode, 8>> m_children;
  OctreeNode* m_parent = nullptr;
};

class OctreeMap : public RgbMap {
public:
  void addColor(color_t color, int levelDeep = 7) {
    m_root.addColor(color, 0, &m_root, 0, levelDeep);
  }

  // makePalette returns true if a 7 level octreeDeep is OK, and false
  // if we can add ONE level deep.
  bool makePalette(Palette* palette,
                   const int colorCount,
                   const int levelDeep = 7);

  void feedWithImage(const Image* image,
                     const color_t maskColor,
                     const int levelDeep = 7);

  // RgbMap impl
  void regenerateMap(const Palette* palette, const int maskIndex) override;
  int mapColor(color_t rgba) const override;

  int moodifications() const { return m_modifications; };

private:
  void fillOrphansNodes(const Palette* palette);

  OctreeNode m_root;
  OctreeNodes m_leavesVector;
  const Palette* m_palette = nullptr;
  int m_modifications = 0;
  int m_maskIndex = 0;
  color_t m_maskColor = 0;
};

} // namespace doc
#endif
