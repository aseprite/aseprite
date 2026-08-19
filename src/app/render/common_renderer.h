// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RENDER_COMMON_RENDERER_H_INCLUDED
#define APP_RENDER_COMMON_RENDERER_H_INCLUDED
#pragma once

#include "app/render/renderer.h"
#include "os/surface.h"

namespace app {

// Common code to render the Editor canvas using a SimpleRenderer/ShaderRenderer
class CommonRenderer : public Renderer {
public:
  void renderCanvas(Editor* editor,
                    ui::Graphics* g,
                    const doc::Sprite* sprite,
                    doc::frame_t frame,
                    const gfx::Rect& dest,
                    const gfx::Rect& expose,
                    bool exposeWithProj) override;

private:
  // Auxiliary surface to re-use on each renderCanvas() call.
  static os::SurfaceRef m_rendered;
};

} // namespace app

#endif
