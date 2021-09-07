// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_PNG_OPTIONS_H_INCLUDED
#define APP_FILE_PNG_OPTIONS_H_INCLUDED
#pragma once

#include "app/file/format_options.h"
#include "base/buffer.h"

namespace app {

  // Data for PNG files
  class PngOptions : public FormatOptions {
  public:
    struct TextEntry {
      // Flags PNG_TEXT_COMPRESSION_NONE/PNG_TEXT_COMPRESSION_zTXt/PNG_ITXT_COMPRESSION_NONE/PNG_ITXT_COMPRESSION_zTXt
      int compression;
      size_t text_length;
      size_t itxt_length;
      base::buffer key;
      std::unique_ptr<base::buffer> text;
      std::unique_ptr<base::buffer> lang;
      std::unique_ptr<base::buffer> lang_key;
    };

    struct Chunk {
      std::string name;
      base::buffer data;
      // Flags PNG_HAVE_IHDR/PNG_HAVE_PLTE/PNG_AFTER_IDAT
      int location;
    };

    using Text = std::vector<TextEntry>;
    using Chunks = std::vector<Chunk>;

    void addTextEntry(TextEntry&& text) {
      m_Text.emplace_back(std::move(text));
    }

    void addChunk(Chunk&& chunk) {
      m_userChunks.emplace_back(std::move(chunk));
    }

    bool isEmpty() {
      return m_Text.empty() && m_userChunks.empty();
    }

    const Text& text() const { return m_Text; }
    const Chunks& chunks() const { return m_userChunks; }

  private:
    Text m_Text;
    Chunks m_userChunks;
  };

} // namespace app

#endif
