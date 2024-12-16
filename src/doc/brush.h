// Aseprite Document Library
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_BRUSH_H_INCLUDED
#define DOC_BRUSH_H_INCLUDED
#pragma once

#include "doc/brush_pattern.h"
#include "doc/brush_type.h"
#include "doc/color.h"
#include "doc/image_ref.h"
#include "gfx/point.h"
#include "gfx/rect.h"

#include <memory>
#include <optional>
#include <vector>

namespace doc {

class Brush;
using BrushRef = std::shared_ptr<Brush>;

class Brush {
public:
  static const int kMinBrushSize = 1;
  static const int kMaxBrushSize = 64;

  enum class ImageColor { MainColor, BackgroundColor, BothColors };

  Brush();
  Brush(BrushType type, int size, int angle);
  ~Brush();

  // Don't offer copy constructor/operator, use clone*() functions
  // instead.
  Brush(const Brush&) = delete;
  Brush& operator=(const Brush&) = delete;

  // Cloned brushes can share the same image until
  // setSize()/Angle()/etc. (regenerate()) is called for the new
  // brush. In that case the original brush and the cloned one will
  // have a different image after all.
  BrushRef cloneWithSharedImages() const;
  BrushRef cloneWithNewImages() const;
  BrushRef cloneWithExistingImages(const ImageRef& image, const ImageRef& maskBitmap) const;

  BrushType type() const { return m_type; }
  int size() const { return m_size; }
  int angle() const { return m_angle; }
  Image* image() const { return m_image.get(); }
  Image* maskBitmap() const { return m_maskBitmap.get(); }
  int gen() const { return m_gen; }

  BrushPattern pattern() const { return m_pattern; }
  gfx::Point patternOrigin() const { return m_patternOrigin; }
  Image* patternImage() const { return m_patternImage.get(); }

  const gfx::Rect& bounds() const { return m_bounds; }
  const gfx::Point& center() const { return m_center; }

  void setSize(int size);
  void setAngle(int angle);
  void setImage(const Image* image, const Image* maskBitmap);

  // Special functions to change the colors of the image or restore
  // the colors to the original image used to create the brush.
  void setImageColor(ImageColor imageColor, color_t color);
  void resetImageColors();

  void setPattern(BrushPattern pattern) { m_pattern = pattern; }
  void setPatternOrigin(const gfx::Point& patternOrigin) { m_patternOrigin = patternOrigin; }
  void setPatternImage(ImageRef& patternImage) { m_patternImage = patternImage; }
  void setCenter(const gfx::Point& center);

  // Returns the original image used to create the brush before
  // calling any setImageColor()
  Image* originalImage() const
  {
    if (m_backupImage)
      return m_backupImage.get();
    return m_image.get();
  }

private:
  void clean();
  void regenerate();
  void regenerateMaskBitmap();
  void resetBounds();
  void copyFieldsFromBrush(const Brush& brush);

  BrushType m_type; // Type of brush
  int m_size;       // Size (diameter)
  int m_angle;      // Angle in degrees 0-360
  ImageRef m_image; // Image of the brush
  ImageRef m_maskBitmap;
  gfx::Rect m_bounds;
  gfx::Point m_center;
  BrushPattern m_pattern;     // How the image should be replicated
  gfx::Point m_patternOrigin; // From what position the brush was taken
  ImageRef m_patternImage;
  int m_gen;

  // Extra data used for setImageColor()
  ImageRef m_backupImage;             // Backup image to avoid losing original brush colors/pattern
  std::optional<color_t> m_mainColor; // Main image brush color
  std::optional<color_t> m_bgColor;   // Background color
};

} // namespace doc

#endif
