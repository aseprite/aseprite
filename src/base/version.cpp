// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/version.h"

#include "base/convert_to.h"

#include <cctype>

namespace base {

Version::Version(const std::string& from)
  : m_prereleaseDigit(0)
{
  std::string::size_type i = 0;
  std::string::size_type j = 0;
  std::string::size_type k = from.find('-', 0);

  while ((j != std::string::npos) &&
         (k == std::string::npos || j < k)) {
    j = from.find('.', i);

    std::string digit;
    if (j != std::string::npos) {
      digit = from.substr(i, j - i);
      i = j+1;
    }
    else
      digit = from.substr(i);

    m_digits.push_back(convert_to<int>(digit));
  }

  if (k != std::string::npos) {
    auto k0 = ++k;
    for (; k < from.size() && !std::isdigit(from[k]); ++k)
      ;
    if (k < from.size()) {
      m_prereleaseDigit = convert_to<int>(from.substr(k));
      m_prerelease = from.substr(k0, k - k0);
    }
    else
      m_prerelease = from.substr(k0);
  }

  while (!m_prerelease.empty() &&
         m_prerelease[m_prerelease.size()-1] == '.')
    m_prerelease.erase(m_prerelease.size()-1);
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
  }

  if (m_prerelease.empty()) {
    return false;
  }
  else if (other.m_prerelease.empty()) {
    return true;
  }
  else {
    int res = m_prerelease.compare(other.m_prerelease);
    if (res < 0)
      return true;
    else if (res > 0)
      return false;
    else
      return (m_prereleaseDigit < other.m_prereleaseDigit);
  }

  return false;
}

std::string Version::str() const
{
  std::string res;
  for (auto digit : m_digits) {
    if (!res.empty())
      res.push_back('.');
    res += base::convert_to<std::string>(digit);
  }
  if (!m_prerelease.empty()) {
    res.push_back('-');
    res += m_prerelease;
    if (m_prereleaseDigit)
      res += base::convert_to<std::string>(m_prereleaseDigit);
  }
  return res;
}

} // namespace base
