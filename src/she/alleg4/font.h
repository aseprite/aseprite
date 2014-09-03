// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_ALLEG4_FONT_H_INCLUDED
#define SHE_ALLEG4_FONT_H_INCLUDED
#pragma once

#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "she/font.h"

namespace she {

  class Alleg4Font : public Font {
  public:
    enum DestroyFlag {
      None = 0,
      DeleteThis = 1,
      DestroyHandle = 2,
      DeleteAndDestroy = DeleteThis | DestroyHandle,
    };

    Alleg4Font(FONT* font, DestroyFlag destroy)
      : m_font(font)
      , m_destroy(destroy) {
    }

    ~Alleg4Font() {
      if (m_destroy & DestroyHandle)
        destroy_font(m_font);
    }

    virtual void dispose() override {
      if (m_destroy & DeleteThis)
        delete this;
    }

    virtual int height() const override {
      return text_height(m_font);
    }

    virtual int charWidth(int chr) const override {
      return m_font->vtable->char_length(m_font, chr);
    }

    virtual int textLength(const char* str) const override {
      return text_length(m_font, str);
    }

    virtual bool isScalable() const {
      return false;
    }

    virtual void setSize(int size) {
      // Do nothing
    }

    virtual void* nativeHandle() override {
      return reinterpret_cast<void*>(m_font);
    }

  private:
    FONT* m_font;
    DestroyFlag m_destroy;
  };

} // namespace she

#endif
