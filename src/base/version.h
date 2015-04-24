// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_VERSION_H_INCLUDED
#define BASE_VERSION_H_INCLUDED
#pragma once

#include <string>
#include <vector>

namespace base {

  class Version {
  public:
    explicit Version(const std::string& from);

    bool operator<(const Version& other) const;

    std::string str() const;

  private:
    typedef std::vector<int> Digits;
    Digits m_digits;
    std::string m_prerelease; // alpha, beta, dev, rc (empty if it's official release)
    int m_prereleaseDigit;
  };

}

#endif
