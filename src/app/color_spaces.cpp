// Aseprite
// Copyright (C) 2018-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/color_spaces.h"

#include "app/doc.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "os/system.h"
#include "os/window.h"

namespace app {

// We use this variable to avoid accessing Preferences::instance()
// from background threads when a color conversion between color
// spaces must be done.
static bool g_manage = false;

void initialize_color_spaces(Preferences& pref)
{
  g_manage = pref.color.manage();
  pref.color.manage.AfterChange.connect([](bool manage) { g_manage = manage; });
}

os::ColorSpaceRef get_screen_color_space()
{
  return os::instance()->defaultWindow()->colorSpace();
}

os::ColorSpaceRef get_current_color_space()
{
  if (auto* editor = Editor::activeEditor())
    return editor->document()->osColorSpace();
  else
    return get_screen_color_space();
}

gfx::ColorSpaceRef get_working_rgb_space_from_preferences()
{
  if (Preferences::instance().color.manage()) {
    const std::string name = Preferences::instance().color.workingRgbSpace();
    if (name == "sRGB")
      return gfx::ColorSpace::MakeSRGB();

    std::vector<os::ColorSpaceRef> colorSpaces;
    os::instance()->listColorSpaces(colorSpaces);
    for (auto& cs : colorSpaces) {
      if (cs->gfxColorSpace()->name() == name)
        return cs->gfxColorSpace();
    }
  }
  return gfx::ColorSpace::MakeNone();
}

//////////////////////////////////////////////////////////////////////
// Color conversion

ConvertCS::ConvertCS()
{
  if (g_manage) {
    auto srcCS = get_current_color_space();
    auto dstCS = get_screen_color_space();
    if (srcCS && dstCS)
      m_conversion = os::instance()->convertBetweenColorSpace(srcCS, dstCS);
  }
}

ConvertCS::ConvertCS(const os::ColorSpaceRef& srcCS, const os::ColorSpaceRef& dstCS)
{
  if (g_manage) {
    m_conversion = os::instance()->convertBetweenColorSpace(srcCS, dstCS);
  }
}

ConvertCS::ConvertCS(ConvertCS&& that) : m_conversion(std::move(that.m_conversion))
{
}

gfx::Color ConvertCS::operator()(const gfx::Color c)
{
  if (m_conversion) {
    gfx::Color out;
    m_conversion->convertRgba((uint32_t*)&out, (const uint32_t*)&c, 1);
    return out;
  }
  else {
    return c;
  }
}

ConvertCS convert_from_current_to_screen_color_space()
{
  return ConvertCS();
}

ConvertCS convert_from_custom_to_srgb(const os::ColorSpaceRef& from)
{
  return ConvertCS(from, os::instance()->makeColorSpace(gfx::ColorSpace::MakeSRGB()));
}

} // namespace app
