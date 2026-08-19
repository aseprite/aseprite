// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/render/common_renderer.h"

#include "app/doc.h"
#include "app/pref/preferences.h"
#include "os/system.h"

namespace app {

os::SurfaceRef CommonRenderer::m_rendered;

void CommonRenderer::renderCanvas(Editor* editor,
                                  ui::Graphics* g,
                                  const doc::Sprite* sprite,
                                  const doc::frame_t frame,
                                  const gfx::Rect& dest,
                                  const gfx::Rect& expose,
                                  const bool exposeWithProj)
{
  const auto& pref = Preferences::instance(); // TODO move these options to Renderer
  const auto& renderProperties = properties();
  const render::Projection proj = projection();

  // Create a temporary surface to draw the sprite on it
  {
    gfx::Size needed = (exposeWithProj ? dest.size() : expose.size());
    if (!m_rendered || m_rendered->width() < needed.w || m_rendered->height() < needed.h) {
      if (m_rendered)
        needed |= m_rendered->size();
      m_rendered = os::System::instance()->makeRgbaSurface(needed.w, needed.h);
    }
  }
  m_rendered->setColorSpace(static_cast<Doc*>(sprite->document())->osColorSpace());

  if (!exposeWithProj)
    setProjection(render::Projection());
  renderSprite(m_rendered.get(), sprite, frame, gfx::Clip(0, 0, expose));

  if (m_rendered && m_rendered->nativeHandle()) {
    os::Paint p;
    if (!exposeWithProj) {
      os::Sampling sampling;
      p.srcEdges(os::Paint::SrcEdges::Fast); // Enable mipmaps if possible

      if (proj.scaleX() < 1.0) {
        switch (pref.editor.downsampling()) {
          case gen::Downsampling::NEAREST:
            sampling = os::Sampling(os::Sampling::Filter::Nearest);
            break;
          case gen::Downsampling::BILINEAR:
            sampling = os::Sampling(os::Sampling::Filter::Linear);
            break;
          case gen::Downsampling::BILINEAR_MIPMAP:
            sampling = os::Sampling(os::Sampling::Filter::Linear, os::Sampling::Mipmap::Nearest);
            break;
          case gen::Downsampling::TRILINEAR_MIPMAP:
            sampling = os::Sampling(os::Sampling::Filter::Linear, os::Sampling::Mipmap::Linear);
            break;
        }
      }

      if (renderProperties.requiresRgbaBackbuffer)
        p.blendMode(os::BlendMode::SrcOver);
      else
        p.blendMode(os::BlendMode::Src);

      gfx::Rect destClip = dest;
      if (proj.scaleX() < 1.0)
        --destClip.w;
      if (proj.scaleY() < 1.0)
        --destClip.h;

      ui::IntersectClip clip(g, destClip);
      if (clip) {
        g->drawSurface(m_rendered.get(), gfx::Rect(0, 0, expose.w, expose.h), dest, sampling, &p);
      }
    }
    else {
      g->drawSurface(m_rendered.get(),
                     gfx::Rect(0, 0, dest.w, dest.h),
                     dest,
                     os::Sampling(os::Sampling::Filter::Nearest),
                     &p);
    }
  }
}

} // namespace app
