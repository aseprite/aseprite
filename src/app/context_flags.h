// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CONTEXT_FLAGS_H_INCLUDED
#define APP_CONTEXT_FLAGS_H_INCLUDED
#pragma once

#include "base/ints.h"

namespace app {

  class Context;
  class Site;

  class ContextFlags {
  public:
    enum {
      HasActiveDocument           = 1 << 0,
      HasActiveSprite             = 1 << 1,
      HasVisibleMask              = 1 << 2,
      HasActiveLayer              = 1 << 3,
      HasActiveCel                = 1 << 4,
      HasActiveImage              = 1 << 5,
      HasBackgroundLayer          = 1 << 6,
      ActiveDocumentIsReadable    = 1 << 7,
      ActiveDocumentIsWritable    = 1 << 8,
      ActiveLayerIsImage          = 1 << 9,
      ActiveLayerIsBackground     = 1 << 10,
      ActiveLayerIsVisible        = 1 << 11,
      ActiveLayerIsEditable       = 1 << 12,
      ActiveLayerIsReference      = 1 << 13,
    };

    ContextFlags();

    bool check(uint32_t flags) const { return (m_flags & flags) == flags; }
    void update(Context* context);

  private:
    void updateFlagsFromSite(const Site& site);

    uint32_t m_flags;
  };

} // namespace app

#endif
