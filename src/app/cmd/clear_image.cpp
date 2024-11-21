// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/clear_image.h"

#include "app/doc.h"
#include "doc/image.h"
#include "doc/primitives.h"

namespace app {
namespace cmd {

using namespace doc;

ClearImage::ClearImage(Cel* cel, color_t color)
  : WithCel(cel)
  , m_color(color), m_frame_copy(cel->frame())
{
}

void ClearImage::onExecute()
{
  Cel* cel = this->cel();
  Image* image = cel->image();

  ASSERT(!m_copy);
  m_copy.reset(Image::createCopy(image));
  clear_image(image, m_color);

  image->incrementVersion();
}

void ClearImage::onUndo()
{
  Cel* cel = this->cel();
  Image* image = cel->image();

  // Restore frame number of the cel, since it could get displaced
  // during the removal of the frame
  if (cel->frame() != m_frame_copy)
    cel->setFrame(m_frame_copy); 

  copy_image(image, m_copy.get());
  m_copy.reset();

  image->incrementVersion();
}

} // namespace cmd
} // namespace app
