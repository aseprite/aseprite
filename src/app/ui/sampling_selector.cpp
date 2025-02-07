// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/sampling_selector.h"

#include "app/i18n/strings.h"
#include "ui/listitem.h"

namespace app {

using namespace ui;

SamplingSelector::SamplingSelector(Behavior behavior)
  : m_behavior(behavior)
  , m_downsamplingLabel(Strings::downsampling_label())
{
  addChild(&m_downsamplingLabel);
  addChild(&m_downsampling);

  m_downsampling.addItem(new ListItem(Strings::downsampling_nearest()));
  m_downsampling.addItem(new ListItem(Strings::downsampling_bilinear()));
  m_downsampling.addItem(new ListItem(Strings::downsampling_bilinear_mipmap()));
  m_downsampling.addItem(new ListItem(Strings::downsampling_trilinear_mipmap()));
  m_downsampling.setSelectedItemIndex((int)Preferences::instance().editor.downsampling());

  if (m_behavior == Behavior::ChangeOnRealTime)
    m_downsampling.Change.connect([this] { save(); });

  m_samplingChangeConn = Preferences::instance().editor.downsampling.AfterChange.connect(
    [this] { onPreferenceChange(); });
}

void SamplingSelector::save()
{
  const int i = m_downsampling.getSelectedItemIndex();
  Preferences::instance().editor.downsampling((gen::Downsampling)i);
}

void SamplingSelector::onPreferenceChange()
{
  const int i = (int)Preferences::instance().editor.downsampling();
  if (m_downsampling.getSelectedItemIndex() != i)
    m_downsampling.setSelectedItemIndex(i);
}

} // namespace app
