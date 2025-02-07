// Aseprite Document IO Library
// Copyright (c) 2019 Igara Studio S.A.
// Copyright (c) 2017-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DIO_PIXEL_IO_H_INCLUDED
#define DIO_PIXEL_IO_H_INCLUDED
#pragma once

#include "dio/file_interface.h"
#include "doc/image_traits.h"

#include <cstring>

namespace dio {

template<typename ImageTraits>
class PixelIO {
public:
  typename ImageTraits::pixel_t read_pixel(FileInterface* fi);
  void write_pixel(FileInterface* fi, typename ImageTraits::pixel_t c);
  void read_scanline(typename ImageTraits::address_t address, int w, uint8_t* buffer);
  void write_scanline(typename ImageTraits::address_t address, int w, uint8_t* buffer);
};

template<>
class PixelIO<doc::RgbTraits> {
  int r, g, b, a;

public:
  doc::RgbTraits::pixel_t read_pixel(FileInterface* f)
  {
    r = f->read8();
    g = f->read8();
    b = f->read8();
    a = f->read8();
    return doc::rgba(r, g, b, a);
  }
  void write_pixel(FileInterface* f, doc::RgbTraits::pixel_t c)
  {
    f->write8(doc::rgba_getr(c));
    f->write8(doc::rgba_getg(c));
    f->write8(doc::rgba_getb(c));
    f->write8(doc::rgba_geta(c));
  }
  void read_scanline(doc::RgbTraits::address_t address, int w, uint8_t* buffer)
  {
    for (int x = 0; x < w; ++x, ++address) {
      r = *(buffer++);
      g = *(buffer++);
      b = *(buffer++);
      a = *(buffer++);
      *address = doc::rgba(r, g, b, a);
    }
  }
  void write_scanline(doc::RgbTraits::address_t address, int w, uint8_t* buffer)
  {
    for (int x = 0; x < w; ++x, ++address) {
      *(buffer++) = doc::rgba_getr(*address);
      *(buffer++) = doc::rgba_getg(*address);
      *(buffer++) = doc::rgba_getb(*address);
      *(buffer++) = doc::rgba_geta(*address);
    }
  }
};

template<>
class PixelIO<doc::GrayscaleTraits> {
  int k, a;

public:
  doc::GrayscaleTraits::pixel_t read_pixel(FileInterface* f)
  {
    k = f->read8();
    a = f->read8();
    return doc::graya(k, a);
  }
  void write_pixel(FileInterface* f, doc::GrayscaleTraits::pixel_t c)
  {
    f->write8(doc::graya_getv(c));
    f->write8(doc::graya_geta(c));
  }
  void read_scanline(doc::GrayscaleTraits::address_t address, int w, uint8_t* buffer)
  {
    for (int x = 0; x < w; ++x, ++address) {
      k = *(buffer++);
      a = *(buffer++);
      *address = doc::graya(k, a);
    }
  }
  void write_scanline(doc::GrayscaleTraits::address_t address, int w, uint8_t* buffer)
  {
    for (int x = 0; x < w; ++x, ++address) {
      *(buffer++) = doc::graya_getv(*address);
      *(buffer++) = doc::graya_geta(*address);
    }
  }
};

template<>
class PixelIO<doc::IndexedTraits> {
public:
  doc::IndexedTraits::pixel_t read_pixel(FileInterface* f) { return f->read8(); }
  void write_pixel(FileInterface* f, doc::IndexedTraits::pixel_t c) { f->write8(c); }
  void read_scanline(doc::IndexedTraits::address_t address, int w, uint8_t* buffer)
  {
    std::memcpy(address, buffer, w);
  }
  void write_scanline(doc::IndexedTraits::address_t address, int w, uint8_t* buffer)
  {
    std::memcpy(buffer, address, w);
  }
};

template<>
class PixelIO<doc::TilemapTraits> {
  int b1, b2, b3, b4;

public:
  doc::TilemapTraits::pixel_t read_pixel(FileInterface* f)
  {
    int b1 = f->read8();
    int b2 = f->read8();
    int b3 = f->read8();
    int b4 = f->read8();

    if (f->ok()) {
      // Little endian
      return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
    }
    else
      return 0;
  }
  void read_scanline(doc::TilemapTraits::address_t address, int w, uint8_t* buffer)
  {
    for (int x = 0; x < w; ++x, ++address) {
      b1 = *(buffer++);
      b2 = *(buffer++);
      b3 = *(buffer++);
      b4 = *(buffer++);
      *address = ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
    }
  }
};

} // namespace dio

#endif
