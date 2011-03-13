/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#ifndef COMMANDS_FILTERS_FILTER_WINDOW_H_INCLUDED
#define COMMANDS_FILTERS_FILTER_WINDOW_H_INCLUDED

#include "gui/box.h"
#include "gui/button.h"
#include "gui/frame.h"
#include "commands/filters/filter_preview.h"
#include "commands/filters/filter_target_buttons.h"
#include "filters/tiled_mode.h"

class FilterManagerImpl;

// A generic window to show parameters for a Filter with integrated
// preview in the current editor.
class FilterWindow : public Frame
{
public:
  enum WithChannels { WithChannelsSelector, WithoutChannelsSelector };
  enum WithTiled { WithTiledCheckBox, WithoutTiledCheckBox };

  FilterWindow(const char* title, const char* cfgSection,
	       FilterManagerImpl* filterMgr,
	       WithChannels withChannels,
	       WithTiled withTiled,
	       TiledMode tiledMode = TILED_NONE);
  ~FilterWindow();

  // Shows the window as modal (blocking interface), and returns true
  // if the user pressed "OK" button (i.e. wants to apply the filter
  // with the current settings).
  bool doModal();

  // Starts (or restart) the preview procedure. You should call this
  // method each time the user modifies parameters of the Filter.
  void restartPreview();

protected:
  // Changes the target buttons. Used by convolution matrix filter
  // which specified different targets for each matrix.
  void setNewTarget(Target target);

  // Returns the container where derived classes should put controls.
  Widget* getContainer() { return &m_container; }

  void onOk(Event& ev);
  void onCancel(Event& ev);
  void onShowPreview(Event& ev);
  void onTargetButtonChange();
  void onTiledChange();

  // Derived classes WithTiledCheckBox should set its filter's tiled
  // mode overriding this method.
  virtual void setupTiledMode(TiledMode tiledMode) { }

private:
  const char* m_cfgSection;
  FilterManagerImpl* m_filterMgr;
  Box m_hbox;
  Box m_vbox;
  Box m_container;
  Button m_okButton;
  Button m_cancelButton;
  FilterPreview m_preview;
  FilterTargetButtons m_targetButton;
  CheckBox m_showPreview;
  CheckBox* m_tiledCheck;
};

#endif
