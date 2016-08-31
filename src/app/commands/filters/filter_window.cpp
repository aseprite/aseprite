// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/filters/filter_window.h"

#include "base/bind.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/commands/filters/filter_worker.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"

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
  , m_okButton("&OK")
  , m_cancelButton("&Cancel")
  , m_preview(filterMgr)
  , m_targetButton(filterMgr->pixelFormat(), (withChannels == WithChannelsSelector))
  , m_showPreview("&Preview")
  , m_tiledCheck(withTiled == WithTiledCheckBox ? new CheckBox("&Tiled") : NULL)
{
  m_targetButton.setTarget(filterMgr->getTarget());
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
    m_tiledCheck->Click.connect(base::Bind<void>(&FilterWindow::onTiledChange, this));

    m_vbox.addChild(m_tiledCheck);
  }

  // Load "Preview" check status.
  m_showPreview.setSelected(get_config_bool(m_cfgSection, "Preview", true));

  // OK is magnetic (the default button)
  m_okButton.setFocusMagnet(true);
}

FilterWindow::~FilterWindow()
{
  // Save window configuration
  save_window_pos(this, m_cfgSection);

  // Save "Preview" check status.
  set_config_bool(m_cfgSection, "Preview", m_showPreview.isSelected());
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
    m_preview.stop();

    // Apply the filter in background
    start_filter_worker(m_filterMgr);
    result = true;
  }

  // Always update editors
  update_screen_for_document(m_filterMgr->document());

  return result;
}

void FilterWindow::restartPreview()
{
  if (m_showPreview.isSelected())
    m_preview.restartPreview();
}

void FilterWindow::setNewTarget(Target target)
{
  m_filterMgr->setTarget(target);
  m_targetButton.setTarget(target);
}

void FilterWindow::onOk(Event& ev)
{
  m_okButton.closeWindow();
}

void FilterWindow::onCancel(Event& ev)
{
  m_cancelButton.closeWindow();
}

void FilterWindow::onShowPreview(Event& ev)
{
  restartPreview();
}

// Called when the user changes the target-buttons.
void FilterWindow::onTargetButtonChange()
{
  // Change the targets in the filter manager and restart the filter preview.
  m_filterMgr->setTarget(m_targetButton.getTarget());
  restartPreview();
}

void FilterWindow::onTiledChange()
{
  ASSERT(m_tiledCheck != NULL);

  // Call derived class implementation of setupTiledMode() so the
  // filter is modified.
  setupTiledMode(m_tiledCheck->isSelected() ?
    TiledMode::BOTH:
    TiledMode::NONE);

  // Restart the preview.
  restartPreview();
}

} // namespace app
