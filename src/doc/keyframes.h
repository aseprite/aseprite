// Aseprite Document Library
// Copyright (c) 2022-2024 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_KEYFRAMES_H_INCLUDED
#define DOC_KEYFRAMES_H_INCLUDED
#pragma once

#include "doc/frame.h"

#include <memory>
#include <vector>

namespace doc {

template<typename T>
class Keyframes {
public:
  class Key {
  public:
    Key(const frame_t frame, std::unique_ptr<T>&& value) : m_frame(frame), m_value(std::move(value))
    {
    }
    Key(const Key& o) : m_frame(o.m_frame), m_value(o.m_value ? new T(*o.m_value) : nullptr) {}
    Key(Key&& o) = default;
    Key& operator=(Key&& o) = default;
    frame_t frame() const { return m_frame; }
    T* value() const { return m_value.get(); }
    void setFrame(const frame_t frame) { m_frame = frame; }
    void setValue(std::unique_ptr<T>&& value) { m_value = std::move(value); }

  private:
    frame_t m_frame = 0;
    std::unique_ptr<T> m_value = nullptr;
  };

  typedef std::vector<Key> List;
  typedef typename List::iterator iterator;
  typedef typename List::const_iterator const_iterator;

  class Range {
  public:
    class RangeIterator {
    public:
      RangeIterator(const iterator& it, const iterator& end, const frame_t frame)
        : m_it(it)
        , m_end(end)
        , m_frame(frame)
      {
        if (it != end) {
          m_next = it;
          ++m_next;
        }
        else
          m_next = end;
      }
      RangeIterator& operator++()
      {
        ++m_frame;
        if (m_next != m_end && m_next->frame() == m_frame) {
          m_it = m_next;
          ++m_next;
        }
        return *this;
      }
      bool operator!=(const RangeIterator& other) { return (m_frame != other.m_frame + 1); }
      T* operator*()
      {
        if (m_it != m_end)
          return m_it->value();
        return nullptr;
      }
      T* operator->()
      {
        if (m_it != m_end)
          return m_it->value();
        return nullptr;
      }

    private:
      iterator m_it, m_next;
      const iterator m_end;
      frame_t m_frame;
    };
    Range(const iterator& fromIt, const iterator& endIt, const frame_t from, const frame_t to)
      : m_fromIt(fromIt)
      , m_endIt(endIt)
      , m_from(from)
      , m_to(to)
    {
    }
    RangeIterator begin() const { return RangeIterator(m_fromIt, m_endIt, m_from); }
    RangeIterator end() const { return RangeIterator(m_fromIt, m_endIt, m_to); }
    bool empty() const { return (m_fromIt == m_endIt || m_to < m_fromIt->frame()); }
    size_t countKeys() const
    {
      size_t count = 0;
      auto it = m_fromIt;
      for (; it != m_endIt; ++it) {
        if (it->frame() > m_to)
          break;
        ++count;
      }
      return count;
    }

  private:
    const iterator m_fromIt, m_endIt;
    const frame_t m_from, m_to;
  };

  Keyframes() {}

  Keyframes(const Keyframes& other)
  {
    for (const auto& key : other.m_keys)
      m_keys.push_back(Key(key.frame(), std::make_unique<T>(*key.value())));
  }

  void insert(const frame_t frame, std::unique_ptr<T>&& value)
  {
    auto it = getIterator(frame);
    if (it == end())
      m_keys.push_back(Key(frame, std::move(value)));
    else if (it->frame() == frame)
      it->setValue(std::move(value));
    else {
      // We must insert keys in order. So if "frame" is less than
      // the "it" frame, insert() will insert the new key before the
      // iterator just as we want. In other case we have to use the
      // next iterator (++it).
      if (frame > it->frame())
        ++it;
      m_keys.insert(it, Key(frame, std::move(value)));
    }
  }

  void remove(const frame_t frame)
  {
    auto it = getIterator(frame);
    if (it != end())
      m_keys.erase(it);
  }

  T* operator[](const frame_t frame)
  {
    auto it = getIterator(frame);
    if (it != end() && it->value() && frame >= it->frame())
      return it->value();
    return nullptr;
  }

  iterator begin() { return m_keys.begin(); }
  iterator end() { return m_keys.end(); }
  const_iterator begin() const { return m_keys.begin(); }
  const_iterator end() const { return m_keys.end(); }

  std::size_t size() const { return m_keys.size(); }
  bool empty() const { return m_keys.empty(); }

  iterator getIterator(const frame_t frame)
  {
    auto it = m_keys.begin(), end = m_keys.end();
    auto next = it;
    for (; it != end; it = next) {
      ++next;
      if (((frame >= it->frame()) && (next == end || frame < next->frame())) ||
          (frame < it->frame())) {
        return it;
      }
    }
    return end;
  }

  frame_t fromFrame() const
  {
    if (!m_keys.empty())
      return m_keys.front().frame();
    return -1;
  }

  frame_t toFrame() const
  {
    if (!m_keys.empty())
      return m_keys.back().frame();
    return -1;
  }

  Range range(const frame_t from, const frame_t to)
  {
    return Range(getIterator(from), end(), from, to);
  }

private:
  List m_keys;
};

} // namespace doc

#endif
