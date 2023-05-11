// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/filters/filter_window.h"

#include "app/commands/filters/filter_manager_impl.h"
#include "app/commands/filters/filter_worker.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/tools/tool_box.h"
#include "app/ui/editor/editor.h"
#include "app/ui/context_bar.h"
#include "app/ui/toolbar.h"

namespace app {

using namespace filters;
using namespace ui;

FilterWindow::FilterWindow(const char* title, const char* cfgSection,
                           FilterManagerImpl* filterMgr,
                           WithChannels withChannels,
                           WithTiled withTiled,
                           TiledMode tiledMode)
  : Window(WithTitleBar, title)
  , m_cfgSection(cfgSection)
  , m_filterMgr(filterMgr)
  , m_hbox(HORIZONTAL)
  , m_vbox(VERTICAL)
  , m_container(VERTICAL)
  , m_okButton(Strings::filters_ok())
  , m_cancelButton(Strings::filters_cancel())
  , m_preview(filterMgr)
  , m_targetButton(filterMgr->pixelFormat(), (withChannels == WithChannelsSelector))
  , m_showPreview(Strings::filters_preview())
  , m_tiledCheck(withTiled == WithTiledCheckBox ?
                   new CheckBox(Strings::filters_tiled()) :
                   nullptr)
  , m_editor(nullptr)
  , m_oldTool(nullptr)
{
  m_okButton.processMnemonicFromText();
  m_cancelButton.processMnemonicFromText();
  m_showPreview.processMnemonicFromText();
  if (m_tiledCheck)
    m_tiledCheck->processMnemonicFromText();

  CelsTarget celsTarget = Preferences::instance().filters.celsTarget();
  filterMgr->setCelsTarget(celsTarget);

  m_targetButton.setTarget(filterMgr->getTarget());
  m_targetButton.setCelsTarget(celsTarget);
  m_targetButton.TargetChange.connect(&FilterWindow::onTargetButtonChange, this);
  m_okButton.Click.connect(&FilterWindow::onOk, this);
  m_cancelButton.Click.connect(&FilterWindow::onCancel, this);
  m_showPreview.Click.connect(&FilterWindow::onShowPreview, this);

  m_container.setExpansive(true);

  m_hbox.addChild(&m_container);
  m_hbox.addChild(&m_vbox);

  m_vbox.addChild(&m_okButton);
  m_vbox.addChild(&m_cancelButton);
  m_vbox.addChild(&m_targetButton);
  m_vbox.addChild(&m_showPreview);

  addChild(&m_preview);
  addChild(&m_hbox);

  if (m_tiledCheck) {
    m_tiledCheck->setSelected(tiledMode != TiledMode::NONE);
    m_tiledCheck->Click.connect([this]{ onTiledChange(); });

    m_vbox.addChild(m_tiledCheck);
  }

  // Load "Preview" check status.
  m_showPreview.setSelected(get_config_bool(m_cfgSection, "Preview", true));

  // OK is magnetic (the default button)
  m_okButton.setFocusMagnet(true);

  if (Editor::activeEditor()) {
    m_editor = Editor::activeEditor();
    m_editor->add_observer(this);
    m_oldTool = m_editor->getCurrentEditorTool();
    tools::Tool* hand = App::instance()->toolBox()->getToolById(tools::WellKnownTools::Hand);
    ToolBar::instance()->selectTool(hand);
  }
}

FilterWindow::~FilterWindow()
{
  if (m_oldTool)
    ToolBar::instance()->selectTool(m_oldTool);

  // Save window configuration
  save_window_pos(this, m_cfgSection);

  // Save "Preview" check status.
  set_config_bool(m_cfgSection, "Preview", m_showPreview.isSelected());

  // Save cels target button
  Preferences::instance().filters.celsTarget(m_targetButton.celsTarget());

  if (m_editor)
    m_editor->remove_observer(this);
}

bool FilterWindow::doModal()
{
  bool result = false;

  // Default position
  remapWindow();
  centerWindow();

  // Load window configuration
  load_window_pos(this, m_cfgSection);

  // Start first preview
  restartPreview();

  // Open in foreground
  openWindowInForeground();

  // Did the user press OK?
  if (closer() == &m_okButton) {
    stopPreview();

    // Apply the filter in background
    start_filter_worker(m_filterMgr);
    result = true;
  }

  // Always update editors
  update_screen_for_document(m_filterMgr->document());

  return result;
}

void FilterWindow::onBroadcastMouseMessage(const gfx::Point& screenPos,
                                            ui::WidgetsList& targets) {
  // Add the Filter Window as receptor of mouse events.
  targets.push_back(this);
  // Also add the editor
  if (m_editor)
    targets.push_back(ui::View::getView(m_editor));
  // and add the context bar.
  if (App::instance()->contextBar())
    targets.push_back(App::instance()->contextBar());
}

void FilterWindow::restartPreview()
{
  bool state = m_showPreview.isSelected();
  m_preview.setEnablePreview(state);

  if (state)
    m_preview.restartPreview();
  else
    stopPreview();
}

void FilterWindow::setNewTarget(Target target)
{
  stopPreview();

  m_filterMgr->setTarget(target);
  m_targetButton.setTarget(target);
}

void FilterWindow::onOk()
{
  m_okButton.closeWindow();
}

void FilterWindow::onCancel()
{
  m_cancelButton.closeWindow();
}

void FilterWindow::onShowPreview()
{
  restartPreview();

  // If the preview was disabled just redraw the current editor in its
  // original state.
  if (!m_showPreview.isSelected())
    m_filterMgr->disablePreview();
}

// Called when the user changes the target-buttons.
void FilterWindow::onTargetButtonChange()
{
  stopPreview();

  // Change the targets in the filter manager and restart the filter preview.
  m_filterMgr->setTarget(m_targetButton.target());
  m_filterMgr->setCelsTarget(m_targetButton.celsTarget());
  restartPreview();
}

void FilterWindow::onTiledChange()
{
  ASSERT(m_tiledCheck);
  stopPreview();

  // Call derived class implementation of setupTiledMode() so the
  // filter is modified.
  setupTiledMode(m_tiledCheck->isSelected() ?
    TiledMode::BOTH:
    TiledMode::NONE);

  // Restart the preview.
  restartPreview();
}

void FilterWindow::stopPreview()
{
  m_preview.stop();
}

void FilterWindow::onScrollChanged(Editor* editor)
{
  restartPreview();
}

void FilterWindow::onZoomChanged(Editor* editor)
{
  restartPreview();
}

} // namespace app
