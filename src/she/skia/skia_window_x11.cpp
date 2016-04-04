// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/skia/skia_window_x11.h"

#include "gfx/size.h"
#include "she/x11/x11.h"

#include "SkBitmap.h"


namespace she {

SkiaWindow::SkiaWindow(EventQueue* queue, SkiaDisplay* display,
                       int width, int height, int scale)
  : X11Window(X11::instance()->display(), width, height)
  , m_clientSize(width, height)
  , m_scale(scale)
{
}

SkiaWindow::~SkiaWindow()
{
}

int SkiaWindow::scale() const
{
  return m_scale;
}

void SkiaWindow::setScale(int scale)
{
  m_scale = scale;
}

void SkiaWindow::setVisible(bool visible)
{
}

void SkiaWindow::maximize()
{
}

bool SkiaWindow::isMaximized() const
{
  return false;
}

bool SkiaWindow::isMinimized() const
{
  return false;
}

gfx::Size SkiaWindow::clientSize() const
{
  return m_clientSize;
}

gfx::Size SkiaWindow::restoredSize() const
{
  return m_clientSize;
}

void SkiaWindow::captureMouse()
{
}

void SkiaWindow::releaseMouse()
{
}

void SkiaWindow::setMousePosition(const gfx::Point& position)
{
}

bool SkiaWindow::setNativeMouseCursor(NativeCursor cursor)
{
  return false;
}

void SkiaWindow::updateWindow(const gfx::Rect& bounds)
{
}

void SkiaWindow::onExpose()
{
}

} // namespace she
