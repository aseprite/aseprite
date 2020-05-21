// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_STATUS_BAR_H_INCLUDED
#define APP_UI_STATUS_BAR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/context_observer.h"
#include "app/tools/active_tool_observer.h"
#include "app/ui/doc_observer_widget.h"
#include "base/time.h"
#include "ui/base.h"
#include "ui/box.h"

#include <string>
#include <vector>

namespace ui {
  class Box;
  class Button;
  class Entry;
  class Label;
  class TooltipManager;
  class Window;
}

namespace render {
  class Zoom;
}

namespace app {
  class ButtonSet;
  class Editor;
  class ZoomEntry;

  namespace tools {
    class Tool;
  }

  class StatusBar : public DocObserverWidget<ui::HBox>
                  , public tools::ActiveToolObserver {
    static StatusBar* m_instance;
  public:
    static StatusBar* instance() { return m_instance; }

    enum BackupIcon { None, Normal, Small };

    StatusBar(ui::TooltipManager* tooltipManager);
    ~StatusBar();

    void clearText();
    void showDefaultText();
    void showDefaultText(Doc* document);

    bool setStatusText(int msecs, const std::string& text);
    void showTip(int msecs, const std::string& msg);
    void showColor(int msecs, const char* text, const Color& color);
    void showTool(int msecs, tools::Tool* tool);
    void showSnapToGridWarning(bool state);

    // Used by AppEditor to update the zoom level in the status bar.
    void updateFromEditor(Editor* editor);

    void showBackupIcon(BackupIcon icon);

  protected:
    void onInitTheme(ui::InitThemeEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;

    // ContextObserver impl
    void onActiveSiteChange(const Site& site) override;

    // DocObserver impl
    void onPixelFormatChanged(DocEvent& ev) override;

    // ActiveToolObserver impl
    void onSelectedToolChange(tools::Tool* tool) override;

  private:
    void onCelOpacitySliderChange();
    void newFrame();
    void onChangeZoom(const render::Zoom& zoom);
    void updateSnapToGridWindowPosition();

    base::tick_t m_timeout;

    // Indicators
    class Indicators;
    class IndicatorsGeneration;
    Indicators* m_indicators;

    // Box of main commands
    ui::Widget* m_docControls;
    ui::Box* m_commandsBox;
    ui::Label* m_frameLabel;
    ui::Entry* m_currentFrame;        // Current frame and go to frame entry
    ui::Button* m_newFrame;           // Button to create a new frame
    ZoomEntry* m_zoomEntry;

    // Tip window
    class CustomizedTipWindow;
    CustomizedTipWindow* m_tipwindow;

    // Snap to grid window
    class SnapToGridWindow;
    SnapToGridWindow* m_snapToGridWindow;
  };

} // namespace app

#endif
