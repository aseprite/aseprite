// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RENDER_H_INCLUDED
#define APP_RENDER_H_INCLUDED
#pragma once

#include "doc/pixel_format.h"
#include "render/render.h"

namespace app {
  class Document;

  class AppRender : public render::Render {
  public:
    AppRender();
    AppRender(app::Document* doc, doc::PixelFormat pixelFormat);

    void setupBackground(app::Document* doc, doc::PixelFormat pixelFormat);
  };

} // namespace app

#endif // APP_RENDER_H_INCLUDED
