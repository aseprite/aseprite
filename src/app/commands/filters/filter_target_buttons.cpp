// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/filters/filter_target_buttons.h"

#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/ui/skin/button_icon_impl.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "doc/image.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/theme.h"
#include "ui/widget.h"

#include <cstring>

namespace app {

using namespace app::skin;
using namespace filters;
using namespace ui;

FilterTargetButtons::FilterTargetButtons(int imgtype, bool withChannels)
  : ButtonSet(4)
  , m_target(0)
  , m_red(nullptr)
  , m_green(nullptr)
  , m_blue(nullptr)
  , m_alpha(nullptr)
  , m_gray(nullptr)
  , m_index(nullptr)
  , m_cels(nullptr)
{
  setMultipleSelection(true);

  if (withChannels) {
    switch (imgtype) {

      case IMAGE_RGB:
      case IMAGE_INDEXED:
        m_red   = addItem("R");
        m_green = addItem("G");
        m_blue  = addItem("B");
        m_alpha = addItem("A");

        if (imgtype == IMAGE_INDEXED) {
          m_index = addItem("Index", 4, 1);
        }
        break;

      case IMAGE_GRAYSCALE:
        m_gray = addItem("K", 2, 1);
        m_alpha = addItem("A", 2, 1);
        break;
    }
  }

  // Create the button to select which cels will be modified by the
  // filter.
  m_cels = addItem(getCelsIcon(), 4, 1);
}

void FilterTargetButtons::setTarget(int target)
{
  m_target = target;

  selectTargetButton(m_red,   TARGET_RED_CHANNEL);
  selectTargetButton(m_green, TARGET_GREEN_CHANNEL);
  selectTargetButton(m_blue,  TARGET_BLUE_CHANNEL);
  selectTargetButton(m_alpha, TARGET_ALPHA_CHANNEL);
  selectTargetButton(m_gray,  TARGET_GRAY_CHANNEL);
  selectTargetButton(m_index, TARGET_INDEX_CHANNEL);

  m_cels->setIcon(getCelsIcon());
}

void FilterTargetButtons::selectTargetButton(Item* item, Target specificTarget)
{
  if (item)
    item->setSelected((m_target & specificTarget) == specificTarget);
}

void FilterTargetButtons::onItemChange()
{
  ButtonSet::onItemChange();
  Target flags = 0;

  if (m_red && m_red->isSelected()) flags |= TARGET_RED_CHANNEL;
  if (m_green && m_green->isSelected()) flags |= TARGET_GREEN_CHANNEL;
  if (m_blue && m_blue->isSelected()) flags |= TARGET_BLUE_CHANNEL;
  if (m_gray && m_gray->isSelected()) flags |= TARGET_GRAY_CHANNEL;
  if (m_index && m_index->isSelected()) flags |= TARGET_INDEX_CHANNEL;
  if (m_alpha && m_alpha->isSelected()) flags |= TARGET_ALPHA_CHANNEL;

  if (m_cels->isSelected()) {
    m_cels->setSelected(false);

    // Rotate cels target
    flags |= (m_target & (TARGET_ALL_FRAMES | TARGET_ALL_LAYERS));
    if (flags & TARGET_ALL_FRAMES) {
      flags &= ~TARGET_ALL_FRAMES;

      if (flags & TARGET_ALL_LAYERS)
        flags &= ~TARGET_ALL_LAYERS;
      else
        flags |= TARGET_ALL_LAYERS;
    }
    else {
      flags |= TARGET_ALL_FRAMES;
    }
  }

  if (m_target != flags) {
    m_target = flags;
    m_cels->setIcon(getCelsIcon());

    TargetChange();
  }
}

SkinPartPtr FilterTargetButtons::getCelsIcon() const
{
  SkinTheme* theme = SkinTheme::instance();

  if (m_target & TARGET_ALL_FRAMES) {
    return (m_target & TARGET_ALL_LAYERS) ?
      theme->parts.targetFramesLayers():
      theme->parts.targetFrames();
  }
  else {
    return (m_target & TARGET_ALL_LAYERS) ?
      theme->parts.targetLayers():
      theme->parts.targetOne();
  }
}

} // namespace app
