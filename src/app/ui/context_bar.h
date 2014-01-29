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

#ifndef APP_UI_CONTEXT_BAR_H_INCLUDED
#define APP_UI_CONTEXT_BAR_H_INCLUDED

#include "app/settings/settings_observers.h"
#include "base/compiler_specific.h"
#include "ui/box.h"

namespace ui {
  class Box;
  class Label;
}

namespace tools {
  class Tool;
}

namespace app {

  class IToolSettings;

  class ContextBar : public ui::Box,
                     public ToolSettingsObserver {
  public:
    ContextBar();
    ~ContextBar();

    void updateFromTool(tools::Tool* tool);

  protected:
    bool onProcessMessage(ui::Message* msg) OVERRIDE;

    // ToolSettingsObserver impl
    void onSetOpacity(int newOpacity) OVERRIDE;

  private:
    void onPenSizeChange();
    void onPenAngleChange();
    void onCurrentToolChange();

    class BrushTypeField;
    class BrushAngleField;
    class BrushSizeField;
    class ToleranceField;
    class InkTypeField;
    class InkOpacityField;
    class SprayWidthField;
    class SpraySpeedField;
    class SelectionModeField;
    class TransparentColorField;
    class RotAlgorithmField;
    class FreehandAlgorithmField;
    class GrabAlphaField;

    IToolSettings* m_toolSettings;
    BrushTypeField* m_brushType;
    BrushAngleField* m_brushAngle;
    BrushSizeField* m_brushSize;
    ui::Label* m_toleranceLabel;
    ToleranceField* m_tolerance;
    InkTypeField* m_inkType;
    ui::Label* m_opacityLabel;
    InkOpacityField* m_inkOpacity;
    GrabAlphaField* m_grabAlpha;
    ui::Box* m_freehandBox;
    FreehandAlgorithmField* m_freehandAlgo;
    ui::Box* m_sprayBox;
    SprayWidthField* m_sprayWidth;
    SpraySpeedField* m_spraySpeed;
    ui::Box* m_selectionOptionsBox;
    SelectionModeField* m_selectionMode;
    TransparentColorField* m_transparentColor;
    RotAlgorithmField* m_rotAlgo;
  };

} // namespace app

#endif
