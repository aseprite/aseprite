// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef BASE_STRING_H_INCLUDED
#define BASE_STRING_H_INCLUDED

#include <string>
#include <iterator>

namespace base {

  typedef std::string string;

  string string_to_lower(const string& original);
  string string_to_upper(const string& original);

  string to_utf8(const std::wstring& widestring);
  std::wstring from_utf8(const string& utf8string);

  int utf8_length(const string& utf8string);

  template<typename SubIterator>
  class utf8_iteratorT : public std::iterator<std::forward_iterator_tag,
                                              string::value_type,
                                              string::difference_type,
                                              typename SubIterator::pointer,
                                              typename SubIterator::reference> {
  public:
    typedef typename SubIterator::pointer pointer; // Needed for GCC

    explicit utf8_iteratorT(const SubIterator& it)
      : m_internal(it) {
    }

    utf8_iteratorT& operator++() {
      int c = *m_internal;
      ++m_internal;

      if (c & 0x80) {
        int n = 1;
        while (c & (0x80>>n))
          n++;

        c &= (1<<(8-n))-1;

        while (--n > 0) {
          int t = *m_internal;
          ++m_internal;

          if ((!(t & 0x80)) || (t & 0x40)) {
            --m_internal;
            return *this;
          }

          c = (c<<6) | (t & 0x3F);
        }
      }

      return *this;
    }

    utf8_iteratorT& operator+=(int i) {
      while (i--)
        operator++();
      return *this;
    }

    utf8_iteratorT operator+(int i) {
      utf8_iteratorT it(*this);
      it += i;
      return it;
    }

    const int operator*() const {
      SubIterator it = m_internal;
      int c = *it;
      ++it;

      if (c & 0x80) {
        int n = 1;
        while (c & (0x80>>n))
          n++;

        c &= (1<<(8-n))-1;

        while (--n > 0) {
          int t = *it;
          ++it;

          if ((!(t & 0x80)) || (t & 0x40))
            return '^';

          c = (c<<6) | (t & 0x3F);
        }
      }

      return c;
    }

    bool operator==(const utf8_iteratorT& it) const {
      return m_internal == it.m_internal;
    }

    bool operator!=(const utf8_iteratorT& it) const {
      return m_internal != it.m_internal;
    }

    pointer operator->() {
      return m_internal.operator->();
    }

  private:
    SubIterator m_internal;
  };

  class utf8_iterator : public utf8_iteratorT<string::iterator> {
  public:
    utf8_iterator(const utf8_iteratorT<string::iterator>& it)
      : utf8_iteratorT<string::iterator>(it) {
    }
    explicit utf8_iterator(const string::iterator& it)
      : utf8_iteratorT<string::iterator>(it) {
    }
  };

  class utf8_const_iterator : public utf8_iteratorT<string::const_iterator> {
  public:
    utf8_const_iterator(const utf8_iteratorT<string::const_iterator>& it)
      : utf8_iteratorT<string::const_iterator>(it) {
    }
    explicit utf8_const_iterator(const string::const_iterator& it)
      : utf8_iteratorT<string::const_iterator>(it) {
    }
  };

}

#endif
