// ASE gfx library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "gfx/border.h"

using namespace gfx;

Border::Border()
{
  m_left = 0;
  m_top = 0;
  m_right = 0;
  m_bottom = 0;
}

Border::Border(int left, int top, int right, int bottom)
{
  m_left = left;
  m_top = top;
  m_right = right;
  m_bottom = bottom;
}

const Border& Border::operator+=(int value)
{
  m_left += value;
  m_top += value;
  m_right += value;
  m_bottom += value;
  return *this;
}

const Border& Border::operator-=(int value)
{
  m_left -= value;
  m_top -= value;
  m_right -= value;
  m_bottom -= value;
  return *this;
}

const Border& Border::operator*=(int value)
{
  m_left *= value;
  m_top *= value;
  m_right *= value;
  m_bottom *= value;
  return *this;
}

const Border& Border::operator/=(int value)
{
  m_left /= value;
  m_top /= value;
  m_right /= value;
  m_bottom /= value;
  return *this;
}

Border Border::operator+(int value) const
{
  return Border(m_left + value,
		m_top + value,
		m_right + value,
		m_bottom + value);
}

Border Border::operator-(int value) const
{
  return Border(m_left - value,
		m_top - value,
		m_right - value,
		m_bottom - value);
}

Border Border::operator*(int value) const
{
  return Border(m_left * value,
		m_top * value,
		m_right * value,
		m_bottom * value);
}

Border Border::operator/(int value) const
{
  return Border(m_left / value,
		m_top / value,
		m_right / value,
		m_bottom / value);
}

Border Border::operator-() const
{
  return Border(-m_left, -m_top, -m_right, -m_bottom);
}

bool Border::operator==(const Border& br) const
{
  return
    m_left == br.m_left && m_top == br.m_top &&
    m_right == br.m_right && m_bottom == br.m_bottom;
}

bool Border::operator!=(const Border& br) const
{
  return
    m_left != br.m_left || m_top != br.m_top ||
    m_right != br.m_right || m_bottom != br.m_bottom;
}
