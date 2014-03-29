// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef BASE_VERSION_H_INCLUDED
#define BASE_VERSION_H_INCLUDED
#pragma once

#include <string>
#include <vector>

namespace base {

  class Version {
  public:
    Version();
    explicit Version(int major, int minor = -1, int revision = -1, int build = -1);

    int major() const    { return (*this)[0]; }
    int minor() const    { return (*this)[1]; }
    int revision() const { return (*this)[2]; }
    int build() const    { return (*this)[3]; }

    Version& major(int digit)    { (*this)[0] = digit; return *this; }
    Version& minor(int digit)    { (*this)[1] = digit; return *this; }
    Version& revision(int digit) { (*this)[2] = digit; return *this; }
    Version& build(int digit)    { (*this)[3] = digit; return *this; }

    size_t size() const {
      return m_digits.size();
    }

    // operator[] that can be used to set version digits.
    int& operator[](size_t index) {
      if (index >= m_digits.size())
        m_digits.resize(index+1);
      return m_digits[index];
    }

    // operator[] that can be used to get version digits.
    int operator[](size_t index) const {
      if (index < m_digits.size())
        return m_digits[index];
      else
        return 0;
    }

    // Adds a new digit.
    void addDigit(int digit) {
      m_digits.push_back(digit);
    }

    // Comparison
    bool operator==(const Version& other) const;
    bool operator<(const Version& other) const;

    bool operator!=(const Version& other) const {
      return !operator==(other);
    }

    bool operator>(const Version& other) const {
      return !operator<(other) && !operator==(other);
    }

    bool operator>=(const Version& other) const {
      return !operator<(other);
    }

    bool operator<=(const Version& other) const {
      return operator<(other) || operator==(other);
    }

  private:
    typedef std::vector<int> Digits;
    Digits m_digits;
  };

}

#endif
