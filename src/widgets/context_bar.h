/* ASEPRITE
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

#ifndef WIDGETS_CONTEXT_BAR_H_INCLUDED
#define WIDGETS_CONTEXT_BAR_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/box.h"

namespace ui {
  class Box;
  class Label;
}

class ContextBar : public ui::Box
{
public:
  ContextBar();
  ~ContextBar();

protected:
  bool onProcessMessage(ui::Message* msg) OVERRIDE;

private:
  void onPenSizeAfterChange();
  void onCurrentToolChange();

  class BrushTypeField;
  class BrushAngleField;
  class BrushSizeField;
  class ToleranceField;
  class InkTypeField;
  class InkOpacityField;
  class SprayWidthField;
  class SpraySpeedField;

  ui::Label* m_brushLabel;
  BrushTypeField* m_brushType;
  BrushAngleField* m_brushAngle;
  BrushSizeField* m_brushSize;
  ui::Label* m_toleranceLabel;
  ToleranceField* m_tolerance;
  InkTypeField* m_inkType;
  ui::Label* m_opacityLabel;
  InkOpacityField* m_inkOpacity;
  ui::Box* m_sprayBox;
  SprayWidthField* m_sprayWidth;
  SpraySpeedField* m_spraySpeed;
};

#endif
