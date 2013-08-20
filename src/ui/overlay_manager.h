// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UI_OVERLAY_MANAGER_H_INCLUDED
#define UI_OVERLAY_MANAGER_H_INCLUDED

#include "ui/base.h"
#include <vector>

namespace she { class Surface; }

namespace ui {

  class Overlay;

  class OverlayManager {
    friend class GuiSystem;     // So it can call destroyInstance() from ~GuiSystem
    static OverlayManager* m_singleton;

    OverlayManager();
    ~OverlayManager();

  public:
    static OverlayManager* instance();

    void addOverlay(Overlay* overlay);
    void removeOverlay(Overlay* overlay);

    void captureOverlappedAreas();
    void restoreOverlappedAreas();
    void drawOverlays();

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
