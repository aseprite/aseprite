// SHE library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SHORTCUT_H_INCLUDED
#define SHE_SHORTCUT_H_INCLUDED
#pragma once

#include "she/keys.h"

namespace she {

  class Shortcut {
  public:
    Shortcut(int unicode = 0,
             KeyModifiers modifiers = kKeyNoneModifier)
      : m_unicode(unicode)
      , m_modifiers(modifiers) {
    }

    int unicode() const { return m_unicode; }
    KeyModifiers modifiers() const { return m_modifiers; }

    bool isEmpty() const { return m_unicode == 0; }

  private:
    int m_unicode;
    KeyModifiers m_modifiers;
  };

} // namespace she

#endif
