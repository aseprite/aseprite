// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_OVERLAY_MANAGER_H_INCLUDED
#define UI_OVERLAY_MANAGER_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include "ui/base.h"
#include <vector>

namespace os { class Surface; }

namespace ui {

  class Overlay;

  class OverlayManager {
    friend class UISystem;     // So it can call destroyInstance() from ~UISystem
    static OverlayManager* m_singleton;

    OverlayManager();
    ~OverlayManager();

  public:
    static OverlayManager* instance();

    void addOverlay(Overlay* overlay);
    void removeOverlay(Overlay* overlay);

    void drawOverlays();
    void restoreOverlappedAreas(const gfx::Rect& bounds);

  private:
    static void destroyInstance();

    typedef std::vector<Overlay*> OverlayList;
    typedef OverlayList::iterator iterator;

    iterator begin() { return m_overlays.begin(); }
    iterator end() { return m_overlays.end(); }

    OverlayList m_overlays;
  };

} // namespace ui

#endif
