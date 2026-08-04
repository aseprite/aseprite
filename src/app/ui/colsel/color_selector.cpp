// Aseprite
// Copyright (C) 2018-present  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/colsel/color_selector.h"

#include "app/app.h"
#include "app/color_spaces.h"
#include "app/color_utils.h"
#include "app/modules/gfx.h"
#include "app/pref/preferences.h"
#include "app/ui/colsel/spectrum.h"
#include "app/ui/colsel/tint_shade_tone.h"
#include "app/ui/colsel/wheel.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/util/shader_helpers.h"
#include "base/concurrent_queue.h"
#include "base/scoped_value.h"
#include "base/thread.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/register_message.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <thread>

#if SK_ENABLE_SKSL
  #include "os/skia/skia_surface.h"

  #include "include/core/SkCanvas.h"
  #include "include/effects/SkRuntimeEffect.h"
#endif

namespace app::colsel {

using namespace app::skin;
using namespace ui;

os::SurfaceRef ColorSelector::m_tempCanvas;

// static
os::Surface* ColorSelector::getTempCanvas(Display* display, int w, int h, gfx::Color bgColor)
{
  auto activeCS = get_current_color_space(display);
  if (!m_tempCanvas || m_tempCanvas->width() != w || m_tempCanvas->height() != h ||
      m_tempCanvas->colorSpace() != activeCS) {
    os::SurfaceRef oldCanvas = m_tempCanvas;
    m_tempCanvas = os::System::instance()->makeSurface(w, h, activeCS);
    os::Paint paint;
    paint.color(bgColor);
    paint.style(os::Paint::Fill);
    m_tempCanvas->drawRect(gfx::Rect(0, 0, w, h), paint);
    if (oldCanvas) {
      m_tempCanvas->drawSurface(oldCanvas.get(),
                                gfx::Rect(0, 0, oldCanvas->width(), oldCanvas->height()),
                                gfx::Rect(0, 0, w, h),
                                os::Sampling(),
                                nullptr);
    }
  }
  return m_tempCanvas.get();
}

#if SK_ENABLE_SKSL
// static
sk_sp<SkRuntimeEffect> ColorSelector::m_alphaEffect;
#endif

ColorSelector::ColorSelector()
  : Widget(kGenericWidget)
  , m_paintFlags(PaintFlags::AllAreas)
  , m_lockColor(false)
  , m_options({})
{
  m_options.Click.connect([this] {
    if (m_impl)
      m_impl->onOptions(this);
  });
  m_options.setVisible(false);
  addChild(&m_options);

  initTheme();

  m_appConn = App::instance()->ColorSpaceChange.connect(&ColorSelector::updateColorSpace, this);
  m_hueConn =
    Preferences::instance().experimental.hueWithSatValueForColorSelector.AfterChange.connect(
      [this]() {
#if SK_ENABLE_SKSL
        m_bottomShader.clear();
        m_bottomEffect.reset();
#endif
        repaintAllAreas();
      });
}

ColorSelector::~ColorSelector()
{
}

void ColorSelector::setType(const Type type)
{
  m_options.setVisible(false);
  switch (m_type = type) {
    case Type::SPECTRUM: m_impl = std::make_unique<Spectrum>(); break;
    case Type::RGB_WHEEL:
    case Type::RYB_WHEEL:
    case Type::NORMAL_MAP_WHEEL:
      m_impl = std::make_unique<Wheel>(type);
      m_options.setVisible(true);
      break;
    case Type::TINT_SHADE_TONE:
    default:                    m_impl = std::make_unique<TintShadeTone>(); break;
  }

#if SK_ENABLE_SKSL
  m_mainShader.clear();
  m_mainEffect.reset();
  m_bottomShader.clear();
  m_bottomEffect.reset();
#endif

  layout();
}

void ColorSelector::selectColor(const app::Color& color)
{
  if (m_lockColor)
    return;

  if (m_color != color)
    m_paintFlags |= onNeedsSurfaceRepaint(color);

  m_color = color;
  invalidate();
}

app::Color ColorSelector::getColorByPosition(const gfx::Point& pos)
{
  gfx::Rect rc = childrenBounds();
  if (rc.isEmpty())
    return app::Color::fromMask();

  const int u = pos.x - rc.x;
  const int umax = std::max(1, rc.w - 1);

  const gfx::Rect bottomBarBounds = this->bottomBarBounds();
  if ((hasCapture() && m_capturedInBottom) || (!hasCapture() && bottomBarBounds.contains(pos)))
    return getBottomBarColor(u, umax);

  const gfx::Rect alphaBarBounds = this->alphaBarBounds();
  if ((hasCapture() && m_capturedInAlpha) || (!hasCapture() && alphaBarBounds.contains(pos)))
    return getAlphaBarColor(u, umax);

  // border color extends bottom/alpha bar if visible
  if (!hasCapture() && pos.y >= rc.y2()) {
    if (!bottomBarBounds.isEmpty())
      return alphaBarBounds.isEmpty() ? getBottomBarColor(u, umax) : getAlphaBarColor(u, umax);
  }

  const int v = pos.y - rc.y;
  const int vmax = std::max(1, rc.h - bottomBarBounds.h - alphaBarBounds.h - 1);
  return getMainAreaColor(u, umax, v, vmax);
}

app::Color ColorSelector::getAlphaBarColor(const int u, const int umax)
{
  int alpha = (255 * u / umax);
  app::Color color = m_color;
  color.setAlpha(std::clamp(alpha, 0, 255));
  return color;
}

void ColorSelector::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(gfx::Size(32 * ui::guiscale(), 32 * ui::guiscale()));
}

bool ColorSelector::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kMouseDownMessage:
      if (manager()->getCapture())
        break;

      captureMouse();
      [[fallthrough]];

    case kMouseMoveMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      const gfx::Point pos = mouseMsg->position();

      if (msg->type() == kMouseDownMessage) {
        m_capturedInBottom = bottomBarBounds().contains(pos);
        m_capturedInAlpha = alphaBarBounds().contains(pos);
        m_capturedInMain = (hasCapture() && !m_capturedInMain && !m_capturedInBottom);

        const gfx::Rect rc = childrenBounds();
        if (!m_capturedInAlpha && pos.y >= rc.y2()) {
          releaseMouse();
          m_capturedInMain = false;
        }
      }

      app::Color color = getColorByPosition(pos);
      if (color != app::Color::fromMask()) {
        base::ScopedValue switcher(m_lockColor, subColorPicked());

        StatusBar::instance()->showColor(0, color);
        if (hasCapture())
          ColorChange(color, mouseMsg->button());
      }
      break;
    }

    case kMouseUpMessage:
      if (hasCapture()) {
        m_capturedInBottom = false;
        m_capturedInAlpha = false;
        m_capturedInMain = false;
        releaseMouse();
      }
      return true;

    case kSetCursorMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      app::Color color = getColorByPosition(mouseMsg->position());
      if (color.getType() != app::Color::MaskType) {
        auto theme = skin::SkinTheme::get(this);
        ui::set_mouse_cursor(kCustomCursor, theme->cursors.eyedropper());
        return true;
      }
      break;
    }

    case kMouseWheelMessage:
      if (!hasCapture()) {
        double scale = 1.0;
        if (msg->shiftPressed() || msg->ctrlPressed() || msg->altPressed()) {
          scale = 15.0;
        }

        const double dz = scale * (static_cast<MouseMessage*>(msg)->wheelDelta().x -
                                   static_cast<MouseMessage*>(msg)->wheelDelta().y);

        const int step =
          ui::adjustWheelStep(dz, static_cast<MouseMessage*>(msg)->preciseWheel(), 16);
        if (step != 0) {
          double newHue = m_color.getHsvHue() + step;
          while (newHue < 0.0)
            newHue += 360.0;
          newHue = std::fmod(newHue, 360.0);

          if (newHue != m_color.getHsvHue()) {
            const app::Color newColor = app::Color::fromHsv(newHue,
                                                            m_color.getHsvSaturation(),
                                                            m_color.getHsvValue(),
                                                            currentAlphaForNewColor());

            ColorChange(newColor, kButtonNone);
          }
        }
      }
      break;
  }

  return Widget::onProcessMessage(msg);
}

void ColorSelector::onInitTheme(ui::InitThemeEvent& ev)
{
  auto theme = SkinTheme::get(this);

  Widget::onInitTheme(ev);
  setBorder(theme->calcBorder(this, theme->styles.editorView()));

  m_options.setStyle(theme->styles.colorWheelOptions());
  setBgColor(theme->colors.editorFace());
}

void ColorSelector::onResize(ui::ResizeEvent& ev)
{
  Widget::onResize(ev);

  // We'll need to redraw the whole surface again with the new widget
  // size.
  m_paintFlags = PaintFlags::AllAreas;

  const gfx::Size prefSize = m_options.sizeHint();
  gfx::Rect rc = childrenBounds();
  rc.x += rc.w - prefSize.w;
  rc.w = prefSize.w;
  rc.h = prefSize.h;
  m_options.setBounds(rc);
}

void ColorSelector::onPaint(ui::PaintEvent& ev)
{
  ui::Graphics* g = ev.graphics();
  auto theme = SkinTheme::get(this);

  theme->drawRect(g,
                  clientBounds(),
                  theme->parts.editorNormal().get(),
                  false); // Do not fill the center

  gfx::Rect rc = clientChildrenBounds();
  if (rc.isEmpty())
    return;

  m_hueWithSatValue = Preferences::instance().experimental.hueWithSatValueForColorSelector();

  gfx::Rect bottomBarBounds = this->bottomBarBounds();
  gfx::Rect alphaBarBounds = this->alphaBarBounds();

  os::Surface* painterSurface = nullptr;

#if SK_ENABLE_SKSL // Paint with shaders
  if (buildEffects()) {
    auto displayCs = get_current_color_space(display());
    auto gCs = g->getInternalSurface()->colorSpace();

    // Get a temporal canvas to paint each region, this surface will
    // not be updated until PaintFlags are updated. So in successive
    // onPaint() calls we reutilize m_tempCanvas to paint the
    // background + the indicators.
    painterSurface = getTempCanvas(display(), rc.w, rc.h, theme->colors.workspace());
    SkCanvas* canvas = &static_cast<os::SkiaSurface*>(painterSurface)->canvas();

    canvas->save();
    {
      SkPaint p;
      p.setStyle(SkPaint::kFill_Style);

      // Main area
      gfx::Rect rc2(0, 0, rc.w, std::max(1, rc.h - bottomBarBounds.h - alphaBarBounds.h));

      if ((m_paintFlags & PaintFlags::MainArea) == PaintFlags::MainArea) {
        SkRuntimeShaderBuilder builder1(m_mainEffect);
        builder1.uniform("iRes") = SkV3{ float(rc2.w), float(rc2.h), 0.0f };
        setShaderParams(builder1, this->color(), true);
        p.setShader(builder1.makeShader());
        canvas->drawRect(SkRect::MakeXYWH(0, 0, rc2.w, rc2.h), p);
        m_paintFlags ^= PaintFlags::MainArea;
      }

      // Bottom bar
      canvas->translate(0.0, rc2.h);
      rc2.h = bottomBarBounds.h;
      if ((m_paintFlags & PaintFlags::BottomBar) == PaintFlags::BottomBar) {
        SkRuntimeShaderBuilder builder2(m_bottomEffect);
        builder2.uniform("iRes") = SkV3{ float(rc2.w), float(rc2.h), 0.0f };
        setShaderParams(builder2, this->color(), false);
        p.setShader(builder2.makeShader());

        canvas->drawRect(SkRect::MakeXYWH(0, 0, rc2.w, rc2.h), p);
        m_paintFlags ^= PaintFlags::BottomBar;
      }

      // Alpha bar
      canvas->translate(0.0, rc2.h);
      rc2.h = alphaBarBounds.h;
      if ((m_paintFlags & PaintFlags::AlphaBar) == PaintFlags::AlphaBar) {
        SkRuntimeShaderBuilder builder3(m_alphaEffect);
        builder3.uniform("iRes") = SkV3{ float(rc2.w), float(rc2.h), 0.0f };
        builder3.uniform("iColor") = appColor_to_SkV4(m_color);
        builder3.uniform("iBg1") = gfxColor_to_SkV4(grid_color1());
        builder3.uniform("iBg2") = gfxColor_to_SkV4(grid_color2());
        p.setShader(builder3.makeShader());

        canvas->drawRect(SkRect::MakeXYWH(0, 0, rc2.w, rc2.h), p);
        m_paintFlags ^= PaintFlags::AlphaBar;
      }
    }
    canvas->restore();
  }
  else
#endif // SK_ENABLE_SKSL
  {
    painterSurface = getTempCanvas(display(), rc.w, rc.h, theme->colors.workspace());
  }

  // Paint background: main area + bottom bar + alpha bar
  if (painterSurface)
    g->drawSurface(painterSurface, rc.x, rc.y);

  // Paint indicators and harmonies.
  rc.h -= bottomBarBounds.h + alphaBarBounds.h;
  if (m_impl)
    m_impl->onPaintMainArea(this, g, rc);

  if (!bottomBarBounds.isEmpty()) {
    bottomBarBounds.offset(-bounds().origin());
    if (m_impl)
      m_impl->onPaintBottomBar(this, g, bottomBarBounds);
  }

  if (!alphaBarBounds.isEmpty()) {
    alphaBarBounds.offset(-bounds().origin());
    onPaintAlphaBar(g, alphaBarBounds);
  }

  if ((m_paintFlags & PaintFlags::AllAreas) != PaintFlags::None) {
    // In the past we offered an alternative way to paint color
    // selectors without shaders, but the implementation was removed.
  }
}

void ColorSelector::onPaintAlphaBar(ui::Graphics* g, const gfx::Rect& rc)
{
  const double lit = m_color.getHslLightness();
  const int alpha = m_color.getAlpha();
  const gfx::Point pos(rc.x + int(rc.w * alpha / 255), rc.y + rc.h / 2);
  paintColorIndicator(g, pos, lit < 0.5);
}

PaintFlags ColorSelector::onNeedsSurfaceRepaint(const app::Color& newColor)
{
  PaintFlags flags = PaintFlags::None;
  if (m_impl)
    flags |= m_impl->onNeedsSurfaceRepaint(this, newColor);
  flags |= (m_color.getRed() != newColor.getRed() || m_color.getGreen() != newColor.getGreen() ||
                m_color.getBlue() != newColor.getBlue() ?
              PaintFlags::AlphaBar :
              PaintFlags::None);
  return flags;
}

void ColorSelector::paintColorIndicator(ui::Graphics* g, const gfx::Point& pos, const bool white)
{
  auto theme = SkinTheme::get(this);
  os::Surface* icon = theme->parts.colorWheelIndicator()->bitmap(0);

  g->drawColoredRgbaSurface(icon,
                            white ? gfx::rgba(255, 255, 255) : gfx::rgba(0, 0, 0),
                            pos.x - icon->width() / 2,
                            pos.y - icon->height() / 2);
}

int ColorSelector::currentAlphaForNewColor() const
{
  if (m_color.getType() != Color::MaskType)
    return m_color.getAlpha();
  else
    return 255;
}

gfx::Rect ColorSelector::bottomBarBounds() const
{
  auto theme = SkinTheme::get(this);
  const gfx::Rect rc = childrenBounds();
  const int size = theme->dimensions.colorSelectorBarSize();
  if (rc.h > 2 * size) {
    if (rc.h > 3 * size) // Alpha bar is visible too
      return gfx::Rect(rc.x, rc.y2() - size * 2, rc.w, size);
    else
      return gfx::Rect(rc.x, rc.y2() - size, rc.w, size);
  }
  else
    return gfx::Rect();
}

gfx::Rect ColorSelector::alphaBarBounds() const
{
  auto theme = SkinTheme::get(this);
  const gfx::Rect rc = childrenBounds();
  const int size = theme->dimensions.colorSelectorBarSize();
  if (rc.h > 3 * size)
    return gfx::Rect(rc.x, rc.y2() - size, rc.w, size);
  else
    return gfx::Rect();
}

void ColorSelector::updateColorSpace()
{
  m_paintFlags |= PaintFlags::AllAreas;
  invalidate();
}

#if SK_ENABLE_SKSL

const char* ColorSelector::mainAreaShader() const
{
  if (m_mainShader.empty() && m_impl)
    m_mainShader = m_impl->mainAreaShader();
  return m_mainShader.c_str();
}

const char* ColorSelector::bottomBarShader() const
{
  if (m_bottomShader.empty() && m_impl)
    m_bottomShader = m_impl->bottomBarShader(this);
  return m_bottomShader.c_str();
}

// static
const char* ColorSelector::alphaBarShader()
{
  return R"(
uniform half3 iRes;
uniform half4 iColor, iBg1, iBg2;

half4 main(vec2 fragcoord) {
 vec2 d = (fragcoord.xy / iRes.xy);
 half4 p = (mod((fragcoord.x / iRes.y) + floor(d.y+0.5), 2.0) > 1.0) ? iBg2: iBg1;
 half4 q = iColor.rgb1;
 float a = d.x;
 return (1.0-a)*p + a*q;
}
)";
}

bool ColorSelector::buildEffects()
{
  if (!m_mainEffect) {
    if (const char* code = mainAreaShader())
      m_mainEffect = make_shader(code);
  }

  if (!m_bottomEffect) {
    if (const char* code = bottomBarShader())
      m_bottomEffect = make_shader(code);
  }

  if (!m_alphaEffect) {
    if (const char* code = alphaBarShader())
      m_alphaEffect = make_shader(code);
  }

  return (m_mainEffect && m_bottomEffect && m_alphaEffect);
}

#endif // SK_ENABLE_SKSL

} // namespace app::colsel
