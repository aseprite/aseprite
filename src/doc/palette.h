// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_PALETTE_H_INCLUDED
#define DOC_PALETTE_H_INCLUDED
#pragma once

#include "doc/color.h"
#include "doc/frame_number.h"
#include "doc/object.h"

#include <vector>
#include <string>

namespace doc {

  class SortPalette {
  public:
    enum Channel {
      RGB_Red,
      RGB_Green,
      RGB_Blue,
      HSV_Hue,
      HSV_Saturation,
      HSV_Value,
      HSL_Lightness,
      YUV_Luma,
    };

    SortPalette(Channel channel, bool ascending);
    ~SortPalette();

    void addChain(SortPalette* chain);

    bool operator()(color_t c1, color_t c2);

  private:
    Channel m_channel;
    bool m_ascending;
    SortPalette* m_chain;
  };

  class Palette : public Object {
  public:
    enum { MaxColors = 256 };

    Palette(FrameNumber frame, int ncolors);
    Palette(const Palette& palette);
    ~Palette();

    static Palette* createGrayscale();

    int size() const { return m_colors.size(); }
    void resize(int ncolors);

    std::string filename() const { return m_filename; }
    void setFilename(const std::string& filename) {
      m_filename = filename;
    }

    int getModifications() const { return m_modifications; }

    FrameNumber frame() const { return m_frame; }
    void setFrame(FrameNumber frame);

    color_t entry(int i) const {
      ASSERT(i >= 0 && i < size());
      return m_colors[i];
    }
    color_t getEntry(int i) const {
      ASSERT(i >= 0 && i < size());
      return m_colors[i];
    }
    void setEntry(int i, color_t color);
    void addEntry(color_t color);

    void copyColorsTo(Palette* dst) const;

    int countDiff(const Palette* other, int* from, int* to) const;

    // Returns true if the palette is completelly black.
    bool isBlack() const;
    void makeBlack();

    void makeHorzRamp(int from, int to);
    void makeVertRamp(int from, int to, int columns);
    void makeRectRamp(int from, int to, int columns);
    void sort(int from, int to, SortPalette* sort_palette, std::vector<int>& mapping);

    int findExactMatch(int r, int g, int b) const;
    int findBestfit(int r, int g, int b, int mask_index = 0) const;

  private:
    FrameNumber m_frame;
    std::vector<color_t> m_colors;
    int m_modifications;
    std::string m_filename; // If the palette is associated with a file.
  };

} // namespace doc

#endif
