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
    struct Chunk {
      std::string name;
      base::buffer data;
      // Flags PNG_HAVE_IHDR/PNG_HAVE_PLTE/PNG_AFTER_IDAT
      int location;
    };

    using Chunks = std::vector<Chunk>;

    void addChunk(Chunk&& chunk) {
      m_userChunks.emplace_back(std::move(chunk));
    }

    bool isEmpty() const {
      return m_userChunks.empty();
    }

    int size() const {
      return int(m_userChunks.size());
    }

    const Chunks& chunks() const { return m_userChunks; }

  private:
    Chunks m_userChunks;
  };

} // namespace app

#endif
