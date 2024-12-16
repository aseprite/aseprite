// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_IMAGE_VIEW_H_INCLUDED
#define UI_IMAGE_VIEW_H_INCLUDED
#pragma once

#include "os/ref.h"
#include "ui/widget.h"

namespace os {
class Surface;
}

namespace ui {

class ImageView : public Widget {
public:
  ImageView(const os::Ref<os::Surface>& sur, int align);

protected:
  void onSizeHint(SizeHintEvent& ev) override;
  void onPaint(PaintEvent& ev) override;

private:
  os::Ref<os::Surface> m_sur;
};

} // namespace ui

#endif
