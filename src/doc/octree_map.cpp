// Aseprite
// Copyright (c) 2019 Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/octree_map.h"

#include "doc/palette.h"

#define MID_VALUE_COLOR ((rgba_r_mask / 2) + 1)

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
      int currentBranchColorAdd = octetToBranchColor(i, level);
      int midColorAdd = MID_VALUE_COLOR >> (level + 1);
      midColorAdd = (midColorAdd << rgba_r_shift)
                  + (midColorAdd << rgba_g_shift)
                  + (midColorAdd << rgba_b_shift);
      color_t branchColorMed = rgba_a_mask
                             + upstreamBranchColor
                             + currentBranchColorAdd
                             + midColorAdd;
      int indexMed = palette->findBestfit(rgba_getr(branchColorMed),
                                          rgba_getg(branchColorMed),
                                          rgba_getb(branchColorMed),
                                          255, 0);
      (*m_children)[i].paletteIndex(indexMed);
    }
  }
}

int OctreeNode::mapColor(int r, int g, int b, int a,
                         int mask_index, int level)
{
  int indexLevel = (  (b >> (7 - level)) & 1) * 4
                   + ((g >> (7 - level)) & 1) * 2
                   + ((r >> (7 - level)) & 1);
  if ((*m_children)[indexLevel].m_children)
    return (*m_children)[indexLevel].mapColor(r, g, b, a,
                                              mask_index, level+1);
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

bool OctreeMap::makePalette(Palette* palette, int colorCount, int levelDeep)
{

  if (root()->children()) {
    // We create paletteIndex to get a "global like" variable, in collectLeafNodes
    // function, the purpose is having a incremental variable in the stack memory
    // sharend between all recursive calls of collectLeafNodes.
    int paletteIndex = 0;
    root()->collectLeafNodes(m_leavesVector.get(), paletteIndex);
  }

  // If we can improve the octree accuracy, makePalette returns false, then
  // outside from this function we must re-construct the octreeMap all again with
  // deep level equal to 8.
  if (levelDeep == 7 && m_leavesVector->size() < colorCount)
    return false;


  std::vector<OctreeNode*> auxLeavesVector; // auxiliary collapsed node accumulator
  bool keepReducingMap = true;

  for (int level = levelDeep; level > -1; level--) {
    for (int i=m_leavesVector->size()-1; i>=0; i--) {
      if (m_leavesVector->size() + auxLeavesVector.size() <= colorCount) {
        for (int j=0; j < auxLeavesVector.size(); j++)
          m_leavesVector->push_back(auxLeavesVector[auxLeavesVector.size() - 1 - j]);
        keepReducingMap = false;
        break;
      }
      else if (m_leavesVector->size() == 0) {
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
                m_leavesVector->push_back(sortedVector[k]);
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

      m_leavesVector->back()->parent()->removeLeaves(auxLeavesVector, *m_leavesVector);
    }
    if (keepReducingMap) {
      // Copy collapsed leaves to m_leavesVector
      int auxLeavesVectorSize = auxLeavesVector.size();
      for (int i=0; i<auxLeavesVectorSize; i++)
        m_leavesVector->push_back(auxLeavesVector[auxLeavesVector.size() - 1 - i]);
      auxLeavesVector.clear();
    }
    else
      break;
  }
  int leafCount = m_leavesVector->size();
  palette->resize(leafCount);
  for (int i=0; i<leafCount; i++)
    palette->setEntry(i, (*m_leavesVector)[i]->getLeafColor().normalizeColor().LeafColorToColor());

  // At this point we must complete the orphans nodes (color map holes)
  // "orphans nodes": nodes which m_color.pixelCount() is 0.
  // These nodes are not part of m_leavesVector
  fillOrphansNodes(palette);
  return true;
}

void OctreeMap::fillOrphansNodes(Palette* palette)
{
  m_root->fillOrphansNodes(palette, 0, 0);
}

void OctreeMap::feedWithImage(Image* image, bool withAlpha, int levelDeep)
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
}

int OctreeMap::mapColor(int r, int g, int b, int a, int mask_index) const
{
  if (m_root->children())
    return m_root->mapColor(r, g, b, a, mask_index, 0);
  return 0;
}

void OctreeMap::regenerate(const Palette* palette, int maskIndex)
{
  if (palette) {
    if (m_modifications != palette->getModifications()) {
      for (int i=0; i<palette->size(); i++)
        m_root->addColor(palette->entry(i), 0, m_root.get(), i, 8);
      m_root->fillOrphansNodes(palette, 0, 0);
      m_modifications = palette->getModifications();
    }
  }
}

} // namespace doc
