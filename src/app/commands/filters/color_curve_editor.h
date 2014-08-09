/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_COMMANDS_FILTERS_COLOR_CURVE_EDITOR_H_INCLUDED
#define APP_COMMANDS_FILTERS_COLOR_CURVE_EDITOR_H_INCLUDED
#pragma once

#include "base/override.h"
#include "base/signal.h"
#include "gfx/point.h"
#include "ui/widget.h"

namespace filters {
  class ColorCurve;
}

namespace app {
  using namespace filters;

  class ColorCurveEditor : public ui::Widget {
  public:
    ColorCurveEditor(ColorCurve* curve, int x1, int y1, int x2, int y2);

    ColorCurve* getCurve() const { return m_curve; }

    Signal0<void> CurveEditorChange;

  protected:
    bool onProcessMessage(ui::Message* msg) OVERRIDE;
    void onPreferredSize(ui::PreferredSizeEvent& ev) OVERRIDE;

  private:
    gfx::Point* getClosestPoint(int x, int y, int** edit_x, int** edit_y);
    int editNodeManually(gfx::Point& point);

    ColorCurve* m_curve;
    int m_x1, m_y1;
    int m_x2, m_y2;
    int m_status;
    gfx::Point* m_editPoint;
    int* m_editX;
    int* m_editY;
  };

} // namespace app

#endif
