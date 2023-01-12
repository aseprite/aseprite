// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_FILTERS_FILTER_WINDOW_H_INCLUDED
#define APP_COMMANDS_FILTERS_FILTER_WINDOW_H_INCLUDED
#pragma once

#include "app/commands/filters/filter_preview.h"
#include "app/commands/filters/filter_target_buttons.h"
#include "filters/tiled_mode.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/window.h"

namespace app {
  class FilterManagerImpl;

  using namespace filters;

  // A generic window to show parameters for a Filter with integrated
  // preview in the current editor.
  class FilterWindow : public ui::Window {
  public:
    enum WithChannels { WithChannelsSelector, WithoutChannelsSelector };
    enum WithTiled { WithTiledCheckBox, WithoutTiledCheckBox };

    FilterWindow(const char* title, const char* cfgSection,
      FilterManagerImpl* filterMgr,
      WithChannels withChannels,
      WithTiled withTiled,
      TiledMode tiledMode = TiledMode::NONE);
    ~FilterWindow();

    // Shows the window as modal (blocking interface), and returns true
    // if the user pressed "OK" button (i.e. wants to apply the filter
    // with the current settings).
    bool doModal();

    // Starts (or restart) the preview procedure. You should call this
    // method after the user modifies parameters of the Filter.
    void restartPreview();

  protected:
    // Changes the target buttons. Used by convolution matrix filter
    // which specified different targets for each matrix.
    void setNewTarget(Target target);

    // Returns the container where derived classes should put controls.
    ui::Widget* getContainer() { return &m_container; }

    void onOk();
    void onCancel();
    void onShowPreview();
    void onTargetButtonChange();
    void onTiledChange();

    // Derived classes WithTiledCheckBox should set its filter's tiled
    // mode overriding this method.
    virtual void setupTiledMode(TiledMode tiledMode) { }

    // Stops the filter preview background thread, you should call
    // this before you modify the parameters of the Filter.
    void stopPreview();

  private:
    const char* m_cfgSection;
    FilterManagerImpl* m_filterMgr;
    ui::Box m_hbox;
    ui::Box m_vbox;
    ui::Box m_container;
    ui::Button m_okButton;
    ui::Button m_cancelButton;
    FilterPreview m_preview;
    FilterTargetButtons m_targetButton;
    ui::CheckBox m_showPreview;
    ui::CheckBox* m_tiledCheck;
  };

} // namespace app

#endif
