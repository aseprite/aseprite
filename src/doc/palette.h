// Aseprite Document Library
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_PALETTE_H_INCLUDED
#define DOC_PALETTE_H_INCLUDED
#pragma once

#include "base/debug.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/object.h"
#include "doc/palette_gradient_type.h"

#include <vector>
#include <string>

namespace doc {

  class Remap;

  class Palette : public Object {
  public:
    static void initBestfit();

    Palette();
    Palette(frame_t frame, int ncolors);
    Palette(const Palette& palette);
    Palette(const Palette& palette, const Remap& remap);
    ~Palette();

    Palette& operator=(const Palette& that);

    static Palette* createGrayscale();

    int size() const { return (int)m_colors.size(); }
    void resize(int ncolors, color_t color = doc::rgba(0, 0, 0, 255));

    // Used to share the palette data with a SkSL shader
    const color_t* rawColorsData() const { return m_colors.data(); }

    const std::string& filename() const { return m_filename; }
    const std::string& comment() const { return m_comment; }

    void setFilename(const std::string& filename) {
      m_filename = filename;
    }

    void setComment(const std::string& comment) {
      m_comment = comment;
    }

    int getModifications() const { return m_modifications; }

    // Return true if the palette has alpha != 255 in some entry
    bool hasAlpha() const;

    // Return true if the palette has an alpha value between > 0 and < 255.
    bool hasSemiAlpha() const;

    frame_t frame() const { return m_frame; }
    void setFrame(frame_t frame);

    color_t entry(int i) const {
      // TODO At this moment we cannot enable this assert because
      //      there are situations that are not handled quite well yet.
      //      E.g. A palette with lesser colors is loaded
      //
      //ASSERT(i < size());
      ASSERT(i >= 0);
      if (i >= 0 && i < size())
        return m_colors[i];
      else
        return 0;
    }
    color_t getEntry(int i) const {
      return entry(i);
    }
    void setEntry(int i, color_t color);
    void addEntry(color_t color);

    void copyColorsTo(Palette* dst) const;

    int countDiff(const Palette* other, int* from, int* to) const;

    bool operator==(const Palette& other) const {
      return (countDiff(&other, nullptr, nullptr) == 0);
    }

    bool operator!=(const Palette& other) const {
      return !operator==(other);
    }

    // Returns true if the palette is completely black.
    bool isBlack() const;
    void makeBlack();

    void makeGradient(int from, int to);
    void makeHueGradient(int from, int to);

    int findExactMatch(int r, int g, int b, int a, int mask_index) const;
    bool findExactMatch(color_t color) const;
    int findBestfit(int r, int g, int b, int a, int mask_index) const;
    int findMaskColor() const;

    void applyRemap(const Remap& remap);

    // TODO add undo/redo support of entry names
    void setEntryName(const int i, const std::string& name);
    const std::string& getEntryName(const int i) const;

  private:
    frame_t m_frame;
    std::vector<color_t> m_colors;
    std::vector<std::string> m_names;
    int m_modifications;
    std::string m_filename; // If the palette is associated with a file.
    std::string m_comment; // Some extra comment from the .gpl file (author, website, etc.).
  };

} // namespace doc

#endif
