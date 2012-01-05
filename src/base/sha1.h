// ASE base library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_SHA1_H_INCLUDED
#define BASE_SHA1_H_INCLUDED

#include <vector>
#include <string>

extern "C" struct SHA1Context;

namespace base {

  class Sha1 {
  public:
    enum { HashSize = 20 };

    Sha1();
    explicit Sha1(const std::vector<uint8_t>& digest);

    // Calculates the SHA1 of the given file.
    static Sha1 calculateFromFile(const std::string& fileName);

    bool operator==(const Sha1& other) const;
    bool operator!=(const Sha1& other) const;

    uint8_t operator[](int index) const {
      return m_digest[index];
    }

  private:
    std::vector<uint8_t> m_digest;
  };

} // namespace base

#endif  // BASE_SHA1_H_INCLUDED
