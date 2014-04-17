/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef RASTER_PALETTE_H_INCLUDED
#define RASTER_PALETTE_H_INCLUDED
#pragma once

#include "raster/color.h"
#include "raster/frame_number.h"
#include "raster/object.h"

#include <vector>

namespace raster {

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

    std::string getFilename() const { return m_filename; }
    void setFilename(const std::string& filename) {
      m_filename = filename;
    }

    int getModifications() const { return m_modifications; }

    FrameNumber getFrame() const { return m_frame; }
    void setFrame(FrameNumber frame);

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

    static Palette* load(const char *filename);
    bool save(const char *filename) const;

    int findBestfit(int r, int g, int b) const;

  private:
    FrameNumber m_frame;
    std::vector<color_t> m_colors;
    int m_modifications;
    std::string m_filename; // If the palette is associated with a file.
  };

} // namespace raster

#endif
