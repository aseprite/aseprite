// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/slice_window.h"

#include "app/doc.h"
#include "app/ui/user_data_popup.h"
#include "base/bind.h"
#include "doc/slice.h"
#include "doc/sprite.h"

#include <algorithm>

namespace app {

SliceWindow::SliceWindow(const doc::Sprite* sprite,
                         const doc::SelectedObjects& slices,
                         const doc::frame_t frame)
  : m_mods(kNone)
{
  ASSERT(!slices.empty());

  Slice* slice = slices.frontAs<Slice>();
  m_userData = slice->userData();
  userData()->Click.connect(base::Bind<void>(&SliceWindow::onPopupUserData, this));

  if (slices.size() == 1) {
    // If we are going to edit just one slice, we indicate like all
    // fields were modified, so then the slice properties transaction
    // is created comparing the window fields with the slice fields
    // (and not with which field was modified).
    m_mods = kAll;

    name()->setText(slice->name());

    const doc::SliceKey* key = slice->getByFrame(frame);
    ASSERT(key);
    if (!key)
      return;

    boundsX()->setTextf("%d", key->bounds().x);
    boundsY()->setTextf("%d", key->bounds().y);
    boundsW()->setTextf("%d", key->bounds().w);
    boundsH()->setTextf("%d", key->bounds().h);

    center()->Click.connect(base::Bind<void>(&SliceWindow::onCenterChange, this));
    pivot()->Click.connect(base::Bind<void>(&SliceWindow::onPivotChange, this));

    if (key->hasCenter()) {
      center()->setSelected(true);
      centerX()->setTextf("%d", key->center().x);
      centerY()->setTextf("%d", key->center().y);
      centerW()->setTextf("%d", key->center().w);
      centerH()->setTextf("%d", key->center().h);
    }
    else {
      onCenterChange();
    }

    if (key->hasPivot()) {
      pivot()->setSelected(true);
      pivotX()->setTextf("%d", key->pivot().x);
      pivotY()->setTextf("%d", key->pivot().y);
    }
    else {
      onPivotChange();
    }
  }
  // Edit multiple slices
  else {
    ui::Entry* entries[] = {
      name(),
      boundsX(), boundsY(), boundsW(), boundsH(),
      centerX(), centerY(), centerW(), centerH(),
      pivotX(), pivotY() };
    const Mods entryMods[] = {
      kName,
      kBoundsX, kBoundsY, kBoundsW, kBoundsH,
      kCenterX, kCenterY, kCenterW, kCenterH,
      kPivotX, kPivotY };

    for (int i=0; i<sizeof(entries)/sizeof(entries[0]); ++i) {
      auto entry = entries[i];
      Mods mod = entryMods[i];
      entry->setSuffix("*");
      entry->Change.connect(
        [this, entry, mod]{
          onModifyField(entry, mod);
        });
    }
  }
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

gfx::Point SliceWindow::pivotValue() const
{
  if (!pivot()->isSelected())
    return doc::SliceKey::NoPivot;

  return gfx::Point(pivotX()->textInt(),
                    pivotY()->textInt());
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

void SliceWindow::onPivotChange()
{
  bool state = pivot()->isSelected();

  pivotX()->setEnabled(state);
  pivotY()->setEnabled(state);

  if (state) {
    pivotX()->setText("0");
    pivotY()->setText("0");
  }
}

void SliceWindow::onPopupUserData()
{
  if (show_user_data_popup(userData()->bounds(), m_userData))
    m_mods = Mods(int(m_mods) | int(kUserData));
}

void SliceWindow::onModifyField(ui::Entry* entry,
                                const Mods mods)
{
  if (entry)
    entry->setSuffix(std::string());
  m_mods = Mods(int(m_mods) | int(mods));
}

} // namespace app
