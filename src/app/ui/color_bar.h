// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_COLOR_BAR_H_INCLUDED
#define APP_UI_COLOR_BAR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/context_observer.h"
#include "app/doc_observer.h"
#include "app/docs_observer.h"
#include "app/ui/button_set.h"
#include "app/ui/color_button.h"
#include "app/ui/input_chain_element.h"
#include "app/ui/palette_view.h"
#include "doc/pixel_format.h"
#include "doc/sort_palette.h"
#include "obs/connection.h"
#include "obs/signal.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/splitter.h"
#include "ui/view.h"

namespace ui {
  class TooltipManager;
}

namespace app {
  class ColorButton;
  class ColorSpectrum;
  class ColorTintShadeTone;
  class ColorWheel;
  class CommandExecutionEvent;
  class PaletteIndexChangeEvent;
  class PalettePopup;
  class PalettesLoader;

  class ColorBar : public ui::Box
                 , public PaletteViewDelegate
                 , public ContextObserver
                 , public DocObserver
                 , public InputChainElement {
    static ColorBar* m_instance;
  public:
    enum class ColorSelector {
      NONE,
      SPECTRUM,
      RGB_WHEEL,
      RYB_WHEEL,
      TINT_SHADE_TONE,
      NORMAL_MAP_WHEEL,
    };

    static ColorBar* instance() { return m_instance; }

    ColorBar(int align, ui::TooltipManager* tooltipManager);
    ~ColorBar();

    void setPixelFormat(PixelFormat pixelFormat);

    app::Color getFgColor() const;
    app::Color getBgColor() const;
    void setFgColor(const app::Color& color);
    void setBgColor(const app::Color& color);

    PaletteView* getPaletteView();

    ColorSelector getColorSelector() const;
    void setColorSelector(ColorSelector selector);

    // Used by the Palette Editor command to change the status of button
    // when the visibility of the dialog changes.
    bool inEditMode() const;
    void setEditMode(bool state);

    ColorButton* fgColorButton() { return &m_fgColor; }
    ColorButton* bgColorButton() { return &m_bgColor; }

    // ContextObserver impl
    void onActiveSiteChange(const Site& site) override;

    // DocObserver impl
    void onGeneralUpdate(DocEvent& ev) override;

    // InputChainElement impl
    void onNewInputPriority(InputChainElement* element,
                            const ui::Message* msg) override;
    bool onCanCut(Context* ctx) override;
    bool onCanCopy(Context* ctx) override;
    bool onCanPaste(Context* ctx) override;
    bool onCanClear(Context* ctx) override;
    bool onCut(Context* ctx) override;
    bool onCopy(Context* ctx) override;
    bool onPaste(Context* ctx) override;
    bool onClear(Context* ctx) override;
    void onCancel(Context* ctx) override;

    obs::signal<void()> ChangeSelection;

  protected:
    void onAppPaletteChange();
    void onFocusPaletteView(ui::Message* msg);
    void onBeforeExecuteCommand(CommandExecutionEvent& ev);
    void onAfterExecuteCommand(CommandExecutionEvent& ev);
    void onPaletteButtonClick();
    void onRemapButtonClick();
    void onPaletteIndexChange(PaletteIndexChangeEvent& ev);
    void onFgColorChangeFromPreferences();
    void onBgColorChangeFromPreferences();
    void onFgColorButtonBeforeChange(app::Color& color);
    void onFgColorButtonChange(const app::Color& color);
    void onBgColorButtonChange(const app::Color& color);
    void onColorButtonChange(const app::Color& color);
    void onPickSpectrum(const app::Color& color, ui::MouseButtons buttons);
    void onReverseColors();
    void onSortBy(doc::SortPaletteBy channel);
    void onGradient();
    void onFixWarningClick(ColorButton* colorButton, ui::Button* warningIcon);
    void onTimerTick();
    void setAscending(bool ascending);

    // PaletteViewDelegate impl
    void onPaletteViewIndexChange(int index, ui::MouseButtons buttons) override;
    void onPaletteViewModification(const doc::Palette* newPalette, PaletteViewModification mod) override;
    void onPaletteViewChangeSize(int boxsize) override;
    void onPaletteViewPasteColors(const Palette* fromPal, const doc::PalettePicks& from, const doc::PalettePicks& to) override;
    app::Color onPaletteViewGetForegroundIndex() override;
    app::Color onPaletteViewGetBackgroundIndex() override;

  private:
    void showRemap();
    void hideRemap();
    void setPalette(const doc::Palette* newPalette, const std::string& actionText);
    void setTransparentIndex(int index);
    void updateWarningIcon(const app::Color& color, ui::Button* warningIcon);
    int setPaletteEntry(const app::Color& color);
    void updateCurrentSpritePalette(const char* operationName);
    void setupTooltips(ui::TooltipManager* tooltipManager);
    void registerCommands();
    void showPaletteSortOptions();
    void showPalettePresets();
    void showPaletteOptions();
    static void fixColorIndex(ColorButton& color);

    class ScrollableView : public ui::View {
    public:
      ScrollableView();
    protected:
      void onInitTheme(ui::InitThemeEvent& ev) override;
    };

    class WarningIcon;

    ButtonSet m_buttons;
    std::unique_ptr<PalettePopup> m_palettePopup;
    ui::Splitter m_splitter;
    ui::VBox m_palettePlaceholder;
    ui::VBox m_selectorPlaceholder;
    ScrollableView m_scrollableView;
    PaletteView m_paletteView;
    ui::Button m_remapButton;
    ColorSelector m_selector;
    ColorTintShadeTone* m_tintShadeTone;
    ColorSpectrum* m_spectrum;
    ColorWheel* m_wheel;
    ColorButton m_fgColor;
    ColorButton m_bgColor;
    WarningIcon* m_fgWarningIcon;
    WarningIcon* m_bgWarningIcon;

    // True when the user clicks the PaletteView so we're changing the
    // color from the palette view.
    bool m_fromPalView;

    // If m_syncingWithPref is true it means that the eyedropper was
    // used to change the color.
    bool m_fromPref;

    bool m_fromFgButton;
    bool m_fromBgButton;

    std::unique_ptr<doc::Palette> m_oldPalette;
    Doc* m_lastDocument;
    bool m_ascending;
    obs::scoped_connection m_beforeCmdConn;
    obs::scoped_connection m_afterCmdConn;
    obs::scoped_connection m_fgConn;
    obs::scoped_connection m_bgConn;
    obs::scoped_connection m_appPalChangeConn;
    ui::MouseButtons m_lastButtons;

    // True if we the editing mode is on.
    bool m_editMode;

    // Timer to redraw editors after a palette change.
    ui::Timer m_redrawTimer;
    bool m_redrawAll;

    // True if a palette change must be implant in the UndoHistory
    // (e.g. when two or more changes in the palette are made in a
    // very short time).
    bool m_implantChange;

    // True if the App::PaletteChange signal is generated by this same
    // ColorBar.
    bool m_selfPalChange;
  };

  class DisableColorBarEditMode {
  public:
    DisableColorBarEditMode()
      : m_colorBar(ColorBar::instance())
      , m_oldMode(m_colorBar->inEditMode()) {
      if (m_oldMode)
        m_colorBar->setEditMode(false);
    }
    ~DisableColorBarEditMode() {
      if (m_oldMode)
        m_colorBar->setEditMode(true);
    }
  private:
    ColorBar* m_colorBar;
    bool m_oldMode;
  };

} // namespace app

#endif
