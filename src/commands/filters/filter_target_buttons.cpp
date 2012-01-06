/* ASE - Allegro Sprite Editor
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

#include "commands/filters/filter_target_buttons.h"

#include <allegro.h>
#include <string.h>

#include "base/bind.h"
#include "gui/box.h"
#include "gui/button.h"
#include "gui/theme.h"
#include "gui/widget.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "skin/skin_parts.h"

FilterTargetButtons::FilterTargetButtons(int imgtype, bool withChannels)
  : Box(JI_VERTICAL)
  , m_target(0)
{
#define ADD(box, widget, hook)                                          \
  if (widget) {                                                         \
    jwidget_set_border(widget, 2 * jguiscale());                        \
    box->addChild(widget);                                              \
    widget->Click.connect(Bind<void>(&FilterTargetButtons::hook, this, widget)); \
  }

  Box* hbox;
  CheckBox* r = NULL;
  CheckBox* g = NULL;
  CheckBox* b = NULL;
  CheckBox* k = NULL;
  CheckBox* a = NULL;
  CheckBox* index = NULL;
  Button* images = NULL;

  hbox = new Box(JI_HORIZONTAL | JI_HOMOGENEOUS);

  jwidget_noborders(this);
  jwidget_noborders(hbox);

  if (withChannels) {
    switch (imgtype) {

      case IMAGE_RGB:
      case IMAGE_INDEXED:
        r = check_button_new("R", 2, 0, 0, 0);
        g = check_button_new("G", 0, 0, 0, 0);
        b = check_button_new("B", 0, (imgtype == IMAGE_RGB) ? 0: 2, 0, 0);

        r->setName("r");
        g->setName("g");
        b->setName("b");

        if (imgtype == IMAGE_RGB) {
          a = check_button_new("A", 0, 2, 0, 0);
          a->setName("a");
        }
        else {
          index = check_button_new("Index", 0, 0, 0, 0);
          index->setName("i");
        }
        break;

      case IMAGE_GRAYSCALE:
        k = check_button_new("K", 2, 0, 0, 0);
        a = check_button_new("A", 0, 2, 0, 0);

        k->setName("k");
        a->setName("a");
        break;
    }
  }

  // Create the button to select "image" target
  images = new Button(NULL);
  setup_bevels(images,
               withChannels ? 0: 2,
               withChannels ? 0: 2, 2, 2);
  setup_mini_look(images);
  set_gfxicon_to_button(images,
                        getTargetNormalIcon(),
                        getTargetSelectedIcon(), -1,
                        JI_CENTER | JI_MIDDLE);

  // Make hierarchy
  ADD(hbox, r, onChannelChange);
  ADD(hbox, g, onChannelChange);
  ADD(hbox, b, onChannelChange);
  ADD(hbox, k, onChannelChange);
  ADD(hbox, a, onChannelChange);

  if (withChannels)
    addChild(hbox);
  else
    jwidget_free(hbox);

  ADD(this, index, onChannelChange);
  ADD(this, images, onImagesChange);
}

void FilterTargetButtons::setTarget(int target)
{
  m_target = target;

  selectTargetButton("r", TARGET_RED_CHANNEL);
  selectTargetButton("g", TARGET_GREEN_CHANNEL);
  selectTargetButton("b", TARGET_BLUE_CHANNEL);
  selectTargetButton("a", TARGET_ALPHA_CHANNEL);
  selectTargetButton("k", TARGET_GRAY_CHANNEL);
  selectTargetButton("i", TARGET_INDEX_CHANNEL);
}

void FilterTargetButtons::selectTargetButton(const char* name, int specificTarget)
{
  Widget* wgt = findChild(name);
  if (wgt != NULL)
    wgt->setSelected((m_target & specificTarget) == specificTarget);
}

void FilterTargetButtons::onChannelChange(ButtonBase* button)
{
  int flag = 0;

  switch (button->name[0]) {
    case 'r': flag = TARGET_RED_CHANNEL; break;
    case 'g': flag = TARGET_GREEN_CHANNEL; break;
    case 'b': flag = TARGET_BLUE_CHANNEL; break;
    case 'k': flag = TARGET_GRAY_CHANNEL; break;
    case 'a': flag = TARGET_ALPHA_CHANNEL; break;
    case 'i': flag = TARGET_INDEX_CHANNEL; break;
    default:
      return;
  }

  if (button->isSelected())
    m_target |= flag;
  else
    m_target &= ~flag;

  TargetChange();
}

void FilterTargetButtons::onImagesChange(ButtonBase* button)
{
  // Rotate target
  if (m_target & TARGET_ALL_FRAMES) {
    m_target &= ~TARGET_ALL_FRAMES;

    if (m_target & TARGET_ALL_LAYERS)
      m_target &= ~TARGET_ALL_LAYERS;
    else
      m_target |= TARGET_ALL_LAYERS;
  }
  else {
    m_target |= TARGET_ALL_FRAMES;
  }

  set_gfxicon_to_button(button,
                        getTargetNormalIcon(),
                        getTargetSelectedIcon(), -1,
                        JI_CENTER | JI_MIDDLE);

  TargetChange();
}

int FilterTargetButtons::getTargetNormalIcon() const
{
  if (m_target & TARGET_ALL_FRAMES) {
    return (m_target & TARGET_ALL_LAYERS) ?
      PART_TARGET_FRAMES_LAYERS:
      PART_TARGET_FRAMES;
  }
  else {
    return (m_target & TARGET_ALL_LAYERS) ?
      PART_TARGET_LAYERS:
      PART_TARGET_ONE;
  }
}

int FilterTargetButtons::getTargetSelectedIcon() const
{
  if (m_target & TARGET_ALL_FRAMES) {
    return (m_target & TARGET_ALL_LAYERS) ?
      PART_TARGET_FRAMES_LAYERS_SELECTED:
      PART_TARGET_FRAMES_SELECTED;
  }
  else {
    return (m_target & TARGET_ALL_LAYERS) ?
      PART_TARGET_LAYERS_SELECTED:
      PART_TARGET_ONE_SELECTED;
  }
}
