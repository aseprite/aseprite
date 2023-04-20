// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/input_options.h"

#include "app/pref/preferences.h"

namespace app {

static InputOptions* g_instance = nullptr;
// static
InputOptions* InputOptions::instance() { return g_instance; };

InputOptions::InputOptions() :
  m_scrollWheelSensitivity(1.0)
{
  ASSERT(!g_instance);
  g_instance = this;
};

InputOptions::InputOptions(Preferences &pref)
  : m_scrollWheelSensitivity(pref.mouse.scrollWheelSens())
{
  ASSERT(!g_instance);
  g_instance = this;
};

InputOptions::~InputOptions()
{
  ASSERT(g_instance == this);
  g_instance = nullptr;
}

// ui::InputOptionsDelegate impl
void InputOptions::scrollWheelSensitivity(double sens)
{
  m_scrollWheelSensitivity = sens;
};

double InputOptions::scrollWheelSensitivity()
{
  return m_scrollWheelSensitivity;
};

} // namespace app
