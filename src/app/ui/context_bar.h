// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_CONTEXT_BAR_H_INCLUDED
#define APP_UI_CONTEXT_BAR_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "app/shade.h"
#include "app/tools/active_tool_observer.h"
#include "app/tools/ink_type.h"
#include "app/tools/tool_loop_modifiers.h"
#include "app/ui/context_bar_observer.h"
#include "doc/brush.h"
#include "obs/connection.h"
#include "obs/observable.h"
#include "obs/signal.h"
#include "ui/box.h"

#include <vector>

namespace doc {
  class Remap;
}

namespace render {
  class DitheringAlgorithmBase;
  class DitheringMatrix;
}

namespace ui {
  class Box;
  class Button;
  class Label;
  class TooltipManager;
}

namespace app {

  namespace tools {
    class Ink;
    class Tool;
  }

  class BrushSlot;
  class DitheringSelector;

  class ContextBar : public ui::Box
                   , public obs::observable<ContextBarObserver>
                   , public tools::ActiveToolObserver {
  public:
    ContextBar(ui::TooltipManager* tooltipManager);
    ~ContextBar();

    void updateForActiveTool();
    void updateForTool(tools::Tool* tool);
    void updateForMovingPixels();
    void updateForSelectingBox(const std::string& text);
    void updateToolLoopModifiersIndicators(tools::ToolLoopModifiers modifiers);
    void updateAutoSelectLayer(bool state);
    bool isAutoSelectLayer() const;

    void setActiveBrush(const doc::BrushRef& brush);
    void setActiveBrushBySlot(tools::Tool* tool, int slot);
    doc::BrushRef activeBrush(tools::Tool* tool = nullptr,
                              tools::Ink* ink = nullptr) const;
    void discardActiveBrush();

    BrushSlot createBrushSlotFromPreferences();
    static doc::BrushRef createBrushFromPreferences(
      ToolPreferences::Brush* brushPref = nullptr);

    doc::Remap* createShadeRemap(bool left);
    void reverseShadeColors();
    Shade getShade() const;

    void setInkType(tools::InkType type);

    // For gradients
    render::DitheringMatrix ditheringMatrix();
    render::DitheringAlgorithmBase* ditheringAlgorithm();

    // Signals
    obs::signal<void()> BrushChange;

  protected:
    void onInitTheme(ui::InitThemeEvent& ev) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onToolSetOpacity(const int& newOpacity);
    void onToolSetFreehandAlgorithm();
    void onToolSetContiguous();

  private:
    void onBrushSizeChange();
    void onBrushAngleChange();
    void onSymmetryModeChange();
    void onFgOrBgColorChange(doc::Brush::ImageColor imageColor);
    void onDropPixels(ContextBarObserver::DropAction action);

    // ActiveToolObserver impl
    void onActiveToolChange(tools::Tool* tool) override;

    void setupTooltips(ui::TooltipManager* tooltipManager);
    void registerCommands();
    void showBrushes();

    class ZoomButtons;
    class BrushBackField;
    class BrushTypeField;
    class BrushAngleField;
    class BrushSizeField;
    class ToleranceField;
    class ContiguousField;
    class PaintBucketSettingsField;
    class InkTypeField;
    class InkOpacityField;
    class InkShadesField;
    class SprayWidthField;
    class SpraySpeedField;
    class SelectionModeField;
    class TransparentColorField;
    class PivotField;
    class RotAlgorithmField;
    class FreehandAlgorithmField;
    class BrushPatternField;
    class EyedropperField;
    class DropPixelsField;
    class AutoSelectLayerField;
    class SymmetryField;

    ZoomButtons* m_zoomButtons;
    BrushBackField* m_brushBack;
    BrushTypeField* m_brushType;
    BrushAngleField* m_brushAngle;
    BrushSizeField* m_brushSize;
    ui::Label* m_toleranceLabel;
    ToleranceField* m_tolerance;
    ContiguousField* m_contiguous;
    PaintBucketSettingsField* m_paintBucketSettings;
    InkTypeField* m_inkType;
    ui::Label* m_inkOpacityLabel;
    InkOpacityField* m_inkOpacity;
    InkShadesField* m_inkShades;
    EyedropperField* m_eyedropperField;
    AutoSelectLayerField* m_autoSelectLayer;
    ui::Box* m_freehandBox;
    FreehandAlgorithmField* m_freehandAlgo;
    BrushPatternField* m_brushPatternField;
    ui::Box* m_sprayBox;
    ui::Label* m_sprayLabel;
    SprayWidthField* m_sprayWidth;
    SpraySpeedField* m_spraySpeed;
    ui::Box* m_selectionOptionsBox;
    DitheringSelector* m_ditheringSelector;
    SelectionModeField* m_selectionMode;
    TransparentColorField* m_transparentColor;
    PivotField* m_pivot;
    RotAlgorithmField* m_rotAlgo;
    DropPixelsField* m_dropPixels;
    doc::BrushRef m_activeBrush;
    ui::Label* m_selectBoxHelp;
    SymmetryField* m_symmetry;
    obs::scoped_connection m_sizeConn;
    obs::scoped_connection m_angleConn;
    obs::scoped_connection m_opacityConn;
    obs::scoped_connection m_freehandAlgoConn;
    obs::scoped_connection m_contiguousConn;
  };

} // namespace app

#endif
