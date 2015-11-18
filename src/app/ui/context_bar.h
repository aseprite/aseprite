// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_CONTEXT_BAR_H_INCLUDED
#define APP_UI_CONTEXT_BAR_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "app/tools/selection_mode.h"
#include "app/ui/context_bar_observer.h"
#include "base/connection.h"
#include "base/observable.h"
#include "doc/brush.h"
#include "doc/brushes.h"
#include "ui/box.h"

#include <vector>

namespace doc {
  class Remap;
}

namespace ui {
  class Box;
  class Button;
  class Label;
}

namespace tools {
  class Tool;
}

namespace app {

  class IBrushSettings;
  class IToolSettings;

  class ContextBar : public ui::Box,
                     public base::Observable<ContextBarObserver> {
  public:
    ContextBar();

    void updateForCurrentTool();
    void updateForTool(tools::Tool* tool);
    void updateForMovingPixels();
    void updateForSelectingBox(const std::string& text);
    void updateSelectionMode(app::tools::SelectionMode mode);
    void updateAutoSelectLayer(bool state);

    void setActiveBrush(const doc::BrushRef& brush);
    doc::BrushRef activeBrush(tools::Tool* tool = nullptr) const;
    void discardActiveBrush();

    // Adds a new brush and returns the slot number where the brush
    // is now available.
    int addBrush(const doc::BrushRef& brush);
    void removeBrush(int slot);
    void removeAllBrushes();
    void setActiveBrushBySlot(int slot);
    doc::Brushes getBrushes();

    void lockBrushSlot(int slot);
    void unlockBrushSlot(int slot);
    bool isBrushSlotLocked(int slot) const;

    static doc::BrushRef createBrushFromPreferences(
      ToolPreferences::Brush* brushPref = nullptr);

    doc::Remap* createShadesRemap(bool left);

    // Signals
    Signal0<void> BrushChange;

  protected:
    void onPreferredSize(ui::PreferredSizeEvent& ev) override;
    void onToolSetOpacity(const int& newOpacity);
    void onToolSetFreehandAlgorithm();

  private:
    void onBrushSizeChange();
    void onBrushAngleChange();
    void onCurrentToolChange();
    void onSymmetryModeChange();
    void onDropPixels(ContextBarObserver::DropAction action);

    struct BrushSlot {
      // True if the user locked the brush using the shortcut key to
      // access it.
      bool locked;

      // Can be null if the user deletes the brush.
      doc::BrushRef brush;

      BrushSlot(const doc::BrushRef& brush)
        : locked(false), brush(brush) {
      }
    };

    typedef std::vector<BrushSlot> BrushSlots;

    class BrushTypeField;
    class BrushAngleField;
    class BrushSizeField;
    class ToleranceField;
    class ContiguousField;
    class StopAtGridField;
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

    BrushTypeField* m_brushType;
    BrushAngleField* m_brushAngle;
    BrushSizeField* m_brushSize;
    ui::Label* m_toleranceLabel;
    ToleranceField* m_tolerance;
    ContiguousField* m_contiguous;
    StopAtGridField* m_stopAtGrid;
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
    SelectionModeField* m_selectionMode;
    TransparentColorField* m_transparentColor;
    PivotField* m_pivot;
    RotAlgorithmField* m_rotAlgo;
    DropPixelsField* m_dropPixels;
    doc::BrushRef m_activeBrush;
    BrushSlots m_brushes;
    ui::Label* m_selectBoxHelp;
    SymmetryField* m_symmetry;
    ScopedConnection m_sizeConn;
    ScopedConnection m_angleConn;
    ScopedConnection m_opacityConn;
    ScopedConnection m_freehandAlgoConn;
  };

} // namespace app

#endif
