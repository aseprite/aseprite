// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef UI_UTF8_RANGE_BUILDER_H_INCLUDED
#define UI_UTF8_RANGE_BUILDER_H_INCLUDED

#include "text/text_blob.h"

#include <vector>

namespace ui {
struct Utf8RangeBuilder : text::TextBlob::RunHandler {
  explicit Utf8RangeBuilder(const int minSize) { ranges.reserve(minSize); }

  void commitRunBuffer(text::TextBlob::RunInfo& info) override
  {
    for (int i = 0; i < info.glyphCount; ++i)
      ranges.push_back(info.getGlyphUtf8Range(i));
  }

  std::vector<text::TextBlob::Utf8Range> ranges;
};
} // namespace ui

#endif // UI_UTF8_RANGE_BUILDER_H_INCLUDED
