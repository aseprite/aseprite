/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "commands/filters/filter_window.h"

#include "base/bind.h"
#include "commands/filters/filter_manager_impl.h"
#include "commands/filters/filter_worker.h"
#include "ini_file.h"
#include "modules/gui.h"

FilterWindow::FilterWindow(const char* title, const char* cfgSection,
                           FilterManagerImpl* filterMgr,
                           WithChannels withChannels,
                           WithTiled withTiled,
                           TiledMode tiledMode)
  : Frame(false, title)
  , m_cfgSection(cfgSection)
  , m_filterMgr(filterMgr)
  , m_hbox(JI_HORIZONTAL)
  , m_container(JI_VERTICAL)
  , m_vbox(JI_VERTICAL)
  , m_okButton("&OK")
  , m_cancelButton("&Cancel")
  , m_preview(filterMgr)
  , m_targetButton(filterMgr->getImgType(), (withChannels == WithChannelsSelector))
  , m_showPreview("&Preview")
  , m_tiledCheck(withTiled == WithTiledCheckBox ? new CheckBox("&Tiled") : NULL)
{
  m_targetButton.setTarget(filterMgr->getTarget());
  m_targetButton.TargetChange.connect(&FilterWindow::onTargetButtonChange, this);
  m_okButton.Click.connect(&FilterWindow::onOk, this);
  m_cancelButton.Click.connect(&FilterWindow::onCancel, this);
  m_showPreview.Click.connect(&FilterWindow::onShowPreview, this);

  jwidget_expansive(&m_container, true);

  m_hbox.addChild(&m_container);
  m_hbox.addChild(&m_vbox);

  m_vbox.addChild(&m_okButton);
  m_vbox.addChild(&m_cancelButton);
  m_vbox.addChild(&m_targetButton);
  m_vbox.addChild(&m_showPreview);

  addChild(&m_preview);
  addChild(&m_hbox);

  if (m_tiledCheck) {
    m_tiledCheck->setSelected(tiledMode != TILED_NONE);
    m_tiledCheck->Click.connect(Bind<void>(&FilterWindow::onTiledChange, this));

    m_vbox.addChild(m_tiledCheck);
  }

  // Load "Preview" check status.
  m_showPreview.setSelected(get_config_bool(m_cfgSection, "Preview", true));

  // OK is magnetic (the default button)
  jwidget_magnetic(&m_okButton, true);
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
  remap_window();
  center_window();

  // Load window configuration
  load_window_pos(this, m_cfgSection);

  // Start first preview
  restartPreview();

  // Open in foreground
  open_window_fg();

  // Did the user press OK?
  if (get_killer() == &m_okButton) {
    m_preview.stop();

    // Apply the filter in background
    start_filter_worker(m_filterMgr);
    result = true;
  }

  // Always update editors
  update_screen_for_document(m_filterMgr->getDocument());

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
  setupTiledMode(m_tiledCheck->isSelected() ? TILED_BOTH: TILED_NONE);

  // Restart the preview.
  restartPreview();
}
