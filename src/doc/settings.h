// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SETTINGS_H_INCLUDED
#define DOC_SETTINGS_H_INCLUDED
#pragma once

namespace doc {

  class Settings {
  public:
    virtual ~Settings() { }

    // How many megabytes per document we can store for undo information.
    virtual size_t undoSizeLimit() const = 0;
  };

} // namespace doc

#endif
