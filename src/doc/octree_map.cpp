// Aseprite
// Copyright (c) 2020 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/octree_map.h"

#include "doc/palette.h"

#define MID_VALUE_COLOR ((rgba_r_mask / 2) + 1)
#define MIN_LEVEL_OCTREE_DEEP 3

namespace doc {

void OctreeNode::addColor(color_t c, int level, OctreeNode* parent,
                          int paletteIndex, int levelDeep)
{
  m_parent = parent;
  if (level >= levelDeep) {
    m_leafColor.add(c);
    m_paletteIndex = paletteIndex;
    return;
  }
  int index = getOctet(c, level);
  if (!m_children) {
    m_children.reset(new std::array<OctreeNode, 8>());
  }
  (*m_children)[index].addColor(c, level + 1, this, paletteIndex, levelDeep);
}

void OctreeNode::fillOrphansNodes(const Palette* palette,
                                  color_t upstreamBranchColor, int level)
{
  for (int i=0; i<8; i++) {
    if ((*m_children)[i].m_children)
      (*m_children)[i].fillOrphansNodes(
        palette,
        upstreamBranchColor + octetToBranchColor(i, level),
        level + 1);
    else if (!((*m_children)[i].isLeaf())) {
      // Here the node IS NOT a Leaf and HAS NOT children
      // So, we must assign palette index to the current node
      // to fill the "map holes" (i.e "death tree branches")
      // BUT, if the level is low (a few bits to identify a color)
      // 0, 1, 2, or 3, we need to create branchs/Leaves until
      // the desired minimum color MSB bits.
      if (level < MIN_LEVEL_OCTREE_DEEP) {
        (*m_children)[i].fillMostSignificantNodes(level);
        i--;
        continue;
      }
      int currentBranchColorAdd = octetToBranchColor(i, level);
      int midColorAdd = MID_VALUE_COLOR >> (level + 1);
      midColorAdd = ((midColorAdd) << rgba_r_shift)
                  + ((midColorAdd) << rgba_g_shift)
                  + ((midColorAdd) << rgba_b_shift);
      color_t branchColorMed = rgba_a_mask
                             + upstreamBranchColor
                             + currentBranchColorAdd
                             + midColorAdd;
      int indexMed = palette->findBestfit2(rgba_getr(branchColorMed),
                                           rgba_getg(branchColorMed),
                                           rgba_getb(branchColorMed));
      (*m_children)[i].paletteIndex(indexMed);
    }
  }
}

void OctreeNode::fillMostSignificantNodes(int level)
{
  if (level < MIN_LEVEL_OCTREE_DEEP) {
    m_children.reset(new std::array<OctreeNode, 8>());
    level++;
    for (int i=0; i<8; i++)
      (*m_children)[i].fillMostSignificantNodes(level);
  }
}

int OctreeNode::mapColor(int r, int g, int b, int level) const
{
  int indexLevel = (  (b >> (7 - level)) & 1) * 4
                   + ((g >> (7 - level)) & 1) * 2
                   + ((r >> (7 - level)) & 1);
  if ((*m_children)[indexLevel].m_children)
    return (*m_children)[indexLevel].mapColor(r, g, b, level+1);
  return (*m_children)[indexLevel].m_paletteIndex;
}

void OctreeNode::collectLeafNodes(std::vector<OctreeNode*>* leavesVector, int& paletteIndex)
{
  for (int i=0; i<8; i++) {
    if ((*m_children)[i].isLeaf()) {
      (*m_children)[i].paletteIndex(paletteIndex);
      leavesVector->push_back(&(*m_children)[i]);
      paletteIndex++;
    }
    else if ((*m_children)[i].m_children)
      (*m_children)[i].collectLeafNodes(leavesVector, paletteIndex);
  }
}

// removeLeaves(): remove leaves from a common parent
// auxParentVector: i/o addreess of an auxiliary parent leaf Vector from outside this function.
// rootLeavesVector: i/o address of the m_root->m_leavesVector
int OctreeNode::removeLeaves(std::vector<OctreeNode*>& auxParentVector,
                             std::vector<OctreeNode*>& rootLeavesVector,
                             int octreeDeep)
{
  // Apply to OctreeNode which has children which are leaf nodes
  int result = 0;
  for (int i=octreeDeep; i>=0; i--) {
    if ((*m_children)[i].isLeaf()) {
      m_leafColor.add((*m_children)[i].getLeafColor());
      result++;
      if (rootLeavesVector[rootLeavesVector.size()-1] == &((*m_children)[i]))
        rootLeavesVector.pop_back();
    }
  }
  auxParentVector.push_back(this);
  return result - 1;
}

OctreeMap::OctreeMap()
  : m_palette(nullptr)
  , m_modifications(0)
{
}

bool OctreeMap::makePalette(Palette* palette, int colorCount, int levelDeep)
{
  if (m_root.children()) {
    // We create paletteIndex to get a "global like" variable, in collectLeafNodes
    // function, the purpose is having a incremental variable in the stack memory
    // sharend between all recursive calls of collectLeafNodes.
    int paletteIndex = 0;
    m_root.collectLeafNodes(&m_leavesVector, paletteIndex);
  }

  // If we can improve the octree accuracy, makePalette returns false, then
  // outside from this function we must re-construct the octreeMap all again with
  // deep level equal to 8.
  if (levelDeep == 7 && m_leavesVector.size() < colorCount)
    return false;


  std::vector<OctreeNode*> auxLeavesVector; // auxiliary collapsed node accumulator
  bool keepReducingMap = true;

  for (int level = levelDeep; level > -1; level--) {
    for (int i=m_leavesVector.size()-1; i>=0; i--) {
      if (m_leavesVector.size() + auxLeavesVector.size() <= colorCount) {
        for (int j=0; j < auxLeavesVector.size(); j++)
          m_leavesVector.push_back(auxLeavesVector[auxLeavesVector.size() - 1 - j]);
        keepReducingMap = false;
        break;
      }
      else if (m_leavesVector.size() == 0) {
        // When colorCount is < 8, auxLeavesVector->size() could reach the 8 size,
        // if this is true and we don't stop the regular removeLeaves algorithm,
        // the 8 remains colors will collapse in one.
        // So, we have to reduce color with other method:
        // Sort colors by pixelCount (most pixelCount on front of sortedVector),
        // then:
        // Blend in pairs from the least pixelCount colors.
        if (auxLeavesVector.size() <= 8 && colorCount < 8 && colorCount > 0) {
          // Sort colors:
          std::vector<OctreeNode*> sortedVector;
          int auxVectorSize = auxLeavesVector.size();
          for (int k=0; k < auxVectorSize; k++) {
            int maximumCount = -1;
            int maximumIndex = -1;
            for (int j=0; j < auxLeavesVector.size(); j++) {
              if (auxLeavesVector[j]->getLeafColor().pixelCount() > maximumCount) {
                maximumCount = auxLeavesVector[j]->getLeafColor().pixelCount();
                maximumIndex = j;
              }
            }
            sortedVector.push_back(auxLeavesVector[maximumIndex]);
            auxLeavesVector.erase(auxLeavesVector.begin() + maximumIndex);
          }
          // End Sort colors.
          // Blend colors:
          for(;;) {
            if (sortedVector.size() <= colorCount) {
              for (int k=0; k<sortedVector.size(); k++)
                m_leavesVector.push_back(sortedVector[k]);
              break;
            }
            sortedVector[sortedVector.size()-2]->getLeafColor()
              .add(sortedVector[sortedVector.size()-1]->getLeafColor());
            sortedVector.pop_back();
          }
          // End Blend colors:
          keepReducingMap = false;
          break;
        }
        else
          break;
      }

      m_leavesVector.back()->parent()->removeLeaves(auxLeavesVector, m_leavesVector);
    }
    if (keepReducingMap) {
      // Copy collapsed leaves to m_leavesVector
      int auxLeavesVectorSize = auxLeavesVector.size();
      for (int i=0; i<auxLeavesVectorSize; i++)
        m_leavesVector.push_back(auxLeavesVector[auxLeavesVector.size() - 1 - i]);
      auxLeavesVector.clear();
    }
    else
      break;
  }
  int leafCount = m_leavesVector.size();
  int aux = 0;
  if (m_maskColor == 0x00FFFFFF)
    palette->resize(leafCount);
  else{
    palette->resize(leafCount + 1);
    palette->setEntry(0, m_maskColor);
    aux = 1;
  }

  for (int i=0; i<leafCount; i++)
    palette->setEntry(i+aux, m_leavesVector[i]->getLeafColor().normalizeColor().LeafColorToColor());

  return true;
}

void OctreeMap::fillOrphansNodes(Palette* palette)
{
  m_root.fillOrphansNodes(palette, 0, 0);
}

void OctreeMap::feedWithImage(Image* image, color_t maskColor, int levelDeep)
{
  ASSERT(image);
  ASSERT(image->pixelFormat() == IMAGE_RGB);
  uint32_t color;
  const LockImageBits<RgbTraits> bits(image);
  LockImageBits<RgbTraits>::const_iterator it = bits.begin(), end = bits.end();

  for (; it != end; ++it) {
    color = *it;
    if (rgba_geta(color) > 0)
      addColor(color, levelDeep);
  }

  m_maskColor = maskColor;
}

int OctreeMap::mapColor(color_t rgba) const
{
  if (rgba_geta(rgba) == 0 && m_maskColor >= 0)
    return m_maskColor;
  else if (m_root.children())
    return m_root.mapColor(rgba_getr(rgba),
                           rgba_getg(rgba),
                           rgba_getb(rgba), 0);
  else
    return 0;
}

void OctreeMap::regenerateMap(const Palette* palette, const int maskIndex)
{
  ASSERT(palette);
  if (!palette)
    return;

  // Skip useless regenerations
  if (m_palette == palette &&
      m_modifications == palette->getModifications() &&
      m_maskIndex == maskIndex)
    return;

  m_root = OctreeNode();
  m_leavesVector.clear();
  for (int i=0; i<palette->size(); i++)
    m_root.addColor(palette->entry(i), 0, &m_root, i, 8);
  m_root.fillOrphansNodes(palette, 0, 0);

  m_palette = palette;
  m_modifications = palette->getModifications();
  m_maskIndex = maskIndex;
  if (maskIndex < 0)
    m_maskColor = 0x00ffffff;
  else
    m_maskColor = palette->getEntry(maskIndex);
}

} // namespace doc
