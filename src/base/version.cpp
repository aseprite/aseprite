// ASE base library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "base/version.h"

using namespace base;

Version::Version()
{
}

Version::Version(int major, int minor, int rev, int compilation)
{
  if (major >= 0 || minor >= 0 || rev >= 0 || compilation >= 0)
    addDigit(major);

  if (minor >= 0 || rev >= 0 || compilation >= 0)
    addDigit(minor);

  if (rev >= 0 || compilation >= 0)
    addDigit(rev);

  if (compilation >= 0)
    addDigit(compilation);
}

bool Version::operator==(const Version& other) const
{
  Digits::const_iterator
    it1 = m_digits.begin(), end1 = m_digits.end(),
    it2 = other.m_digits.begin(), end2 = other.m_digits.end();

  while (it1 != end1 || it2 != end2) {
    int digit1 = (it1 != end1 ? *it1++: 0);
    int digit2 = (it2 != end2 ? *it2++: 0);

    if (digit1 != digit2)
      return false;
  }

  return true;
}

bool Version::operator<(const Version& other) const
{
  Digits::const_iterator
    it1 = m_digits.begin(), end1 = m_digits.end(),
    it2 = other.m_digits.begin(), end2 = other.m_digits.end();

  while (it1 != end1 || it2 != end2) {
    int digit1 = (it1 != end1 ? *it1++: 0);
    int digit2 = (it2 != end2 ? *it2++: 0);

    if (digit1 < digit2)
      return true;
    else if (digit1 > digit2)
      return false;
    // else continue...
  }

  return false;
}
