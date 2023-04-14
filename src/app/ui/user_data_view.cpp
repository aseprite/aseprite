// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/user_data_view.h"

#include "app/color.h"
#include "app/pref/preferences.h"
#include "app/ui/color_button.h"
#include "base/scoped_value.h"
#include "doc/user_data.h"
#include "ui/base.h"
#include "ui/entry.h"
#include "ui/grid.h"
#include "ui/label.h"
#include "ui/widget.h"

namespace app {

UserDataView::UserDataView(Option<bool>& visibility)
  : m_visibility(visibility)
{
}

void UserDataView::configureAndSet(const doc::UserData& userData, ui::Grid* parent)
{
  base::ScopedValue switchSelf(m_selfUpdate, true);

  if (!m_isConfigured) {
    // Find the correct hspan to add to an arbitrary grid column count:
    // Example with grid columns count = 4:
    //
    //    <------------------ columns = 4 ----------------->
    //
    //   |            |            |           |            |
    //   | Color:      =========== color picker ============
    //   | User Data:  =========== text  entry =============
    //
    //   | hspan1 = 1 |          hspan2 = 3                 |
    std::vector<ui::Grid::Info> childrenInfo(parent->children().size());
    int i = 0;
    int columnCount = 0;
    for(auto child : parent->children()) {
      childrenInfo[i] = parent->getChildInfo(child);
      if (columnCount < childrenInfo[i].col)
        columnCount = childrenInfo[i].col;
      i++;
    }
    int hspan1 = 1;
    int hspan2 = columnCount;
    int vspan = 1;
    parent->addChildInCell(colorLabel(), hspan1, vspan, ui::LEFT);
    parent->addChildInCell(color(), hspan2, vspan, ui::HORIZONTAL);
    parent->addChildInCell(entryLabel(), hspan1, vspan, ui::LEFT);
    parent->addChildInCell(entry(), hspan2, vspan, ui::HORIZONTAL);
    color()->Change.connect([this]{ onColorChange(); });
    entry()->Change.connect([this]{ onEntryChange(); });
    m_isConfigured = true;
  }
  m_userData = userData;
  color_t c = userData.color();
  color()->setColor(Color::fromRgb(rgba_getr(c),rgba_getg(c), rgba_getb(c), rgba_geta(c)));
  entry()->setText(m_userData.text());
  setVisible(isVisible());
}

void UserDataView::toggleVisibility()
{
  setVisible(!isVisible());
}

void UserDataView::setVisible(bool state, bool saveAsDefault)
{
  colorLabel()->setVisible(state);
  color()->setVisible(state);
  entryLabel()->setVisible(state);
  entry()->setVisible(state);
  if (saveAsDefault)
    m_visibility.setValue(state);
}

void UserDataView::onEntryChange()
{
  if (entry()->text() != m_userData.text()) {
    m_userData.setText(entry()->text());
    if (!m_selfUpdate)
      UserDataChange();
  }
}

void UserDataView::onColorChange()
{
  color_t c = m_userData.color();
  app::Color oldColor = app::Color::fromRgb(rgba_getr(c), rgba_getg(c), rgba_getb(c), rgba_geta(c));
  app::Color newColor = color()->getColor();
  if (newColor != oldColor) {
    m_userData.setColor(rgba(newColor.getRed(),
                             newColor.getGreen(),
                             newColor.getBlue(),
                             newColor.getAlpha()));
    if (!m_selfUpdate)
      UserDataChange();
  }
}

} // namespace app
