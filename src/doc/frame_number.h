// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_FRAME_NUMBER_H_INCLUDED
#define DOC_FRAME_NUMBER_H_INCLUDED
#pragma once

namespace doc {

  class FrameNumber {
  public:
    FrameNumber() : m_value(0) { }
    explicit FrameNumber(int value) : m_value(value) { }

    FrameNumber next(int i = 1) const { return FrameNumber(m_value+i); };
    FrameNumber previous(int i = 1) const { return FrameNumber(m_value-i); };

    operator int() { return m_value; }
    operator const int() const { return m_value; }

    FrameNumber& operator=(const FrameNumber& o) { m_value = o.m_value; return *this; }
    FrameNumber& operator++() { ++m_value; return *this; }
    FrameNumber& operator--() { --m_value; return *this; }
    FrameNumber operator++(int) { FrameNumber old(*this); ++m_value; return old; }
    FrameNumber operator--(int) { FrameNumber old(*this); --m_value; return old; }
    FrameNumber& operator+=(const FrameNumber& o) { m_value += o.m_value; return *this; }
    FrameNumber& operator-=(const FrameNumber& o) { m_value -= o.m_value; return *this; }
    bool operator<(const FrameNumber& o) const { return m_value < o.m_value; }
    bool operator>(const FrameNumber& o) const { return m_value > o.m_value; }
    bool operator<=(const FrameNumber& o) const { return m_value <= o.m_value; }
    bool operator>=(const FrameNumber& o) const { return m_value >= o.m_value; }
    bool operator==(const FrameNumber& o) const { return m_value == o.m_value; }
    bool operator!=(const FrameNumber& o) const { return m_value != o.m_value; }

  private:
    int m_value;
  };

  inline FrameNumber operator+(const FrameNumber& x, const FrameNumber& y) {
    return FrameNumber((int)x + (int)y);
  }

  inline FrameNumber operator-(const FrameNumber& x, const FrameNumber& y) {
    return FrameNumber((int)x - (int)y);
  }

} // namespace doc

#endif
