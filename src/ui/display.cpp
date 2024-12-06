// Aseprite UI Library
// Copyright (C) 2019-2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

//#define DEBUG_DIRTY_RECTS 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/display.h"

#include "base/debug.h"
#include "base/remove_from_container.h"
#include "ui/system.h"
#include "ui/widget.h"
#include "ui/window.h"
#include "os/system.h"

#include <algorithm>

namespace ui {

Display::Display(Display* parentDisplay,
                 const os::WindowRef& nativeWindow,
                 Widget* containedWidget)
  : m_parentDisplay(parentDisplay)
  , m_nativeWindow(nativeWindow)
  , m_containedWidget(containedWidget)
  , m_layers(1)           // One UI layer by default (the backLayer())
{
  m_layers[0] = UILayer::Make();

  os::Paint p;
  p.blendMode(os::BlendMode::Src);
  m_layers[0]->setPaint(p);

#if 0 // When compiling tests all these values can be nullptr
  ASSERT(m_nativeWindow);
  ASSERT(m_containedWidget);
  ASSERT(m_containedWidget->type() == kManagerWidget ||
         m_containedWidget->type() == kWindowWidget);
#endif
  m_dirtyRegion = bounds();

  // Configure the back layer surface.
  configureBackLayer();
}

os::SurfaceRef Display::nativeSurface() const
{
  if (m_nativeWindow)
    return base::AddRef(m_nativeWindow->surface());
  else
    return nullptr;
}

void Display::addLayer(const UILayerRef& layer)
{
  ASSERT(layer);

  m_layers.push_back(layer);

  // Mark the new UILayer are as dirty so it's visible in the next
  // flipDisplay() call.
  dirtyRect(layer->bounds());
}

void Display::removeLayer(const UILayerRef& layer)
{
  ASSERT(layer);

  dirtyRect(layer->bounds());
  base::remove_from_container(m_layers, layer);
}

void Display::configureBackLayer()
{
  const os::SurfaceRef displaySurface = nativeSurface();
  if (!displaySurface)
    return;

  UILayerRef layer = backLayer();

  os::SurfaceRef layerSurface = layer->surface();
  if (!layerSurface ||
      layerSurface == displaySurface ||
      layerSurface->width() != displaySurface->width() ||
      layerSurface->height() != displaySurface->height()) {
    layerSurface = os::System::instance()
      ->makeSurface(displaySurface->width(),
                    displaySurface->height());
    layer->setSurface(layerSurface);
  }
}

gfx::Size Display::size() const
{
  // When running tests this can be nullptr
  if (!m_nativeWindow)
    return gfx::Size(1, 1);

  const int scale = m_nativeWindow->scale();
  ASSERT(scale > 0);
  return gfx::Size(m_nativeWindow->width() / scale,
                   m_nativeWindow->height() / scale);
}

void Display::dirtyRect(const gfx::Rect& bounds)
{
  m_dirtyRegion |= gfx::Region(bounds);
}

void Display::flipDisplay()
{
  if (m_dirtyRegion.isEmpty())
    return;

  // If the GPU acceleration is on, it's recommended to draw the whole
  // surface on each frame.
  if (m_nativeWindow->gpuAcceleration()) {
    m_dirtyRegion = gfx::Region(bounds());
  }
  else {
    // Limit the region to the bounds of the window
    m_dirtyRegion &= gfx::Region(bounds());
    if (m_dirtyRegion.isEmpty())
      return;
  }

  const os::SurfaceRef windowSurface = nativeSurface();

  // Compose all UI layers in the dirty regions
  for (const UILayerRef& layer : m_layers) {
    const os::SurfaceRef layerSurface = layer->surface();
    if (!layerSurface || layerSurface == windowSurface)
      continue;

    for (const gfx::Rect& rc : m_dirtyRegion) {
      gfx::Rect srcRc = rc;
      srcRc.offset(-layer->position());

      windowSurface->saveClip();
      if (!layer->clipRegion().isEmpty())
        windowSurface->clipRegion(layer->clipRegion());

      windowSurface->drawSurface(
        layerSurface.get(), srcRc, rc,
        os::Sampling(), &layer->paint());

      windowSurface->restoreClip();
    }
  }

#if DEBUG_DIRTY_RECTS
  for (const gfx::Rect& rc : m_dirtyRegion) {
    os::Paint paint;
    paint.color(gfx::rgba(255, 0, 0));
    paint.style(os::Paint::Stroke);
    windowSurface->drawRect(rc, paint);
  }
#endif

  // Invalidate the dirty region in the os::Window only if the window
  // is visible (e.g. if the window is hidden or minimized, we don't
  // need to do this).
  if (m_nativeWindow->isVisible()) {
    m_nativeWindow->invalidateRegion(m_dirtyRegion);
    m_nativeWindow->swapBuffers();
  }
  // If the native window is not minimized/hidden by the user, make it
  // visible (e.g. when we flipDisplay() for the first time).
  else if (!m_nativeWindow->isMinimized()) {
    m_nativeWindow->setVisible(true);
  }

  m_dirtyRegion.clear();
}

void Display::invalidateRect(const gfx::Rect& rect)
{
  m_containedWidget->invalidateRect(rect);
}

void Display::invalidateRegion(const gfx::Region& region)
{
  m_containedWidget->invalidateRegion(region);
}

void Display::addWindow(Window* window)
{
  m_windows.insert(m_windows.begin(), window);
}

void Display::removeWindow(Window* window)
{
  auto it = std::find(m_windows.begin(), m_windows.end(), window);
  ASSERT(it != m_windows.end())
  if (it == m_windows.end())
    return;

  m_windows.erase(it);
}

// TODO code similar to Manager::handleWindowZOrder()
void Display::handleWindowZOrder(Window* window)
{
  removeWindow(window);

  if (window->isOnTop())
    m_windows.insert(m_windows.begin(), window);
  else {
    int pos = (int)m_windows.size();

    for (auto it=m_windows.rbegin(),
           end=m_windows.rend();
         it != end; ++it) {
      if (static_cast<Window*>(*it)->isOnTop())
        break;

      --pos;
    }

    m_windows.insert(m_windows.begin()+pos, window);
  }
}

gfx::Size Display::workareaSizeUIScale()
{
  if (get_multiple_displays()) {
    return
      nativeWindow()->screen()->workarea().size() /
      nativeWindow()->scale();
  }
  else {
    return size();
  }
}

} // namespace ui
