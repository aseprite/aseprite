// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/slice_window.h"

#include "app/document.h"
#include "app/ui/user_data_popup.h"
#include "base/bind.h"
#include "doc/slice.h"
#include "doc/sprite.h"

#include <algorithm>

namespace app {

SliceWindow::SliceWindow(const doc::Sprite* sprite,
                         const doc::Slice* slice,
                         const doc::frame_t frameNum)
  : m_userData(slice->userData())
{
  name()->setText(slice->name());

  const doc::SliceKey* key = slice->getByFrame(frameNum);
  ASSERT(key);
  if (!key)
    return;

  userData()->Click.connect(base::Bind<void>(&SliceWindow::onPopupUserData, this));

  boundsX()->setTextf("%d", key->bounds().x);
  boundsY()->setTextf("%d", key->bounds().y);
  boundsW()->setTextf("%d", key->bounds().w);
  boundsH()->setTextf("%d", key->bounds().h);

  center()->Click.connect(base::Bind<void>(&SliceWindow::onCenterChange, this));

  if (!key->center().isEmpty()) {
    center()->setSelected(true);
    centerX()->setTextf("%d", key->center().x);
    centerY()->setTextf("%d", key->center().y);
    centerW()->setTextf("%d", key->center().w);
    centerH()->setTextf("%d", key->center().h);
  }
  else
    onCenterChange();
}

bool SliceWindow::show()
{
  openWindowInForeground();
  return (closer() == ok());
}

std::string SliceWindow::nameValue() const
{
  return name()->text();
}

gfx::Rect SliceWindow::boundsValue() const
{
  gfx::Rect rc(boundsX()->textInt(),
               boundsY()->textInt(),
               boundsW()->textInt(),
               boundsH()->textInt());
  if (rc.w < 1) rc.w = 1;
  if (rc.h < 1) rc.h = 1;
  return rc;
}

gfx::Rect SliceWindow::centerValue() const
{
  if (!center()->isSelected())
    return gfx::Rect(0, 0, 0, 0);

  gfx::Rect rc(centerX()->textInt(),
               centerY()->textInt(),
               centerW()->textInt(),
               centerH()->textInt());
  if (rc.w < 1) rc.w = 1;
  if (rc.h < 1) rc.h = 1;
  return rc;
}

void SliceWindow::onCenterChange()
{
  bool state = center()->isSelected();

  centerX()->setEnabled(state);
  centerY()->setEnabled(state);
  centerW()->setEnabled(state);
  centerH()->setEnabled(state);

  if (state) {
    centerX()->setText("1");
    centerY()->setText("1");
    centerW()->setTextf("%d", std::max(1, boundsW()->textInt()-2));
    centerH()->setTextf("%d", std::max(1, boundsH()->textInt()-2));
  }
}

void SliceWindow::onPopupUserData()
{
  show_user_data_popup(userData()->bounds(), m_userData);
}

} // namespace app
