// Aseprite
// Copyright (C) 2018-2022  Igara Studio S.A.
// Copyright (C) 2016-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#define COLSEL_TRACE(...)

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_selector.h"

#include "app/app.h"
#include "app/color_spaces.h"
#include "app/color_utils.h"
#include "app/modules/gfx.h"
#include "app/pref/preferences.h"
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

namespace app {

using namespace app::skin;
using namespace ui;

// TODO This logic could be used to redraw any widget:
// 1. We send a onPaintSurfaceInBgThread() to paint the widget on a
//    offscreen buffer
// 2. When the painting is done, we flip the buffer onto the screen
// 3. If we receive another onPaint() we can cancel the background
//    painting and start another onPaintSurfaceInBgThread()
//
// An alternative (better) way:
// 1. Create an alternative ui::Graphics implementation that generates
//    a list commands for the render thread
// 2. Widgets can still use the same onPaint()
// 3. The background threads consume the list of commands and render
//    the screen.
//
// The bad side is that is harder to invalidate the commands that will
// render an old state of the widget. So the render thread should
// start caring about invalidating old commands (outdated regions) or
// cleaning the queue if it gets too big.
//
class ColorSelector::Painter {
public:
  Painter() : m_canvas(nullptr) {
  }

  ~Painter() {
    ASSERT(!m_canvas);
  }

  void addRef() {
    assert_ui_thread();

    if (m_ref == 0)
      m_paintingThread = std::thread([this]{ paintingProc(); });

    ++m_ref;
  }

  void releaseRef() {
    assert_ui_thread();

    --m_ref;
    if (m_ref == 0) {
      {
        std::unique_lock<std::mutex> lock(m_mutex);
        stopCurrentPainting(lock);

        m_killing = true;
        m_paintingCV.notify_one();
      }

      m_paintingThread.join();
      if (m_canvas)
        m_canvas.reset();
    }
  }

  os::Surface* getCanvas(int w, int h, gfx::Color bgColor) {
    assert_ui_thread();

    auto activeCS = get_current_color_space();

    if (!m_canvas ||
        m_canvas->width() != w ||
        m_canvas->height() != h ||
        m_canvas->colorSpace() != activeCS) {
      std::unique_lock<std::mutex> lock(m_mutex);
      stopCurrentPainting(lock);

      os::SurfaceRef oldCanvas = m_canvas;
      m_canvas = os::instance()->makeSurface(w, h, activeCS);
      os::Paint paint;
      paint.color(bgColor);
      paint.style(os::Paint::Fill);
      m_canvas->drawRect(gfx::Rect(0, 0, w, h), paint);
      if (oldCanvas) {
        m_canvas->drawSurface(
          oldCanvas.get(),
          gfx::Rect(0, 0, oldCanvas->width(), oldCanvas->height()),
          gfx::Rect(0, 0, w, h),
          os::Sampling(),
          nullptr);
      }
    }
    return m_canvas.get();
  }

  void startBgPainting(ColorSelector* colorSelector,
                       const gfx::Rect& mainBounds,
                       const gfx::Rect& bottomBarBounds,
                       const gfx::Rect& alphaBarBounds) {
    assert_ui_thread();
    COLSEL_TRACE("COLSEL: startBgPainting for %p\n", colorSelector);

    std::unique_lock<std::mutex> lock(m_mutex);
    stopCurrentPainting(lock);

    m_colorSelector = colorSelector;
    m_manager = colorSelector->manager();
    m_mainBounds = mainBounds;
    m_bottomBarBounds = bottomBarBounds;
    m_alphaBarBounds = alphaBarBounds;

    m_stopPainting = false;
    m_paintingCV.notify_one();
  }

private:

  void stopCurrentPainting(std::unique_lock<std::mutex>& lock) {
    // Stop current
    if (m_colorSelector) {
      COLSEL_TRACE("COLSEL: stoppping painting of %p\n", m_colorSelector);

      // TODO use another condition variable here
      m_stopPainting = true;
      m_waitStopCV.wait(lock);
    }

    ASSERT(m_colorSelector == nullptr);
  }

  void paintingProc() {
    COLSEL_TRACE("COLSEL: paintingProc starts\n");

    std::unique_lock<std::mutex> lock(m_mutex);
    while (true) {
      m_paintingCV.wait(lock);

      if (m_killing)
        break;

      auto colorSel = m_colorSelector;
      if (!colorSel)
        break;

      COLSEL_TRACE("COLSEL: starting painting in bg for %p\n", colorSel);

      // Do the intesive painting in the background thread
      {
        lock.unlock();
        colorSel->onPaintSurfaceInBgThread(
          m_canvas.get(),
          m_mainBounds,
          m_bottomBarBounds,
          m_alphaBarBounds,
          m_stopPainting);
        lock.lock();
      }

      m_colorSelector = nullptr;

      if (m_stopPainting) {
        COLSEL_TRACE("COLSEL: painting for %p stopped\n");
        m_waitStopCV.notify_one();
      }
      else {
        COLSEL_TRACE("COLSEL: painting for %p done and sending message\n");
        colorSel->m_paintFlags |= DoneFlag;
      }
    }

    COLSEL_TRACE("COLSEL: paintingProc ends\n");
  }

  int m_ref = 0;
  bool m_killing = false;
  bool m_stopPainting = false;
  std::mutex m_mutex;
  std::condition_variable m_paintingCV;
  std::condition_variable m_waitStopCV;
  os::SurfaceRef m_canvas;
  ColorSelector* m_colorSelector;
  ui::Manager* m_manager;
  gfx::Rect m_mainBounds;
  gfx::Rect m_bottomBarBounds;
  gfx::Rect m_alphaBarBounds;
  std::thread m_paintingThread;
};

static ColorSelector::Painter painter;

#if SK_ENABLE_SKSL
// static
sk_sp<SkRuntimeEffect> ColorSelector::m_alphaEffect;
#endif

ColorSelector::ColorSelector()
  : Widget(kGenericWidget)
  , m_paintFlags(AllAreasFlag)
  , m_lockColor(false)
  , m_timer(100, this)
{
  initTheme();
  painter.addRef();

  m_appConn = App::instance()
    ->ColorSpaceChange.connect(
      &ColorSelector::updateColorSpace, this);
}

ColorSelector::~ColorSelector()
{
  painter.releaseRef();
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
  const int umax = std::max(1, rc.w-1);

  const gfx::Rect bottomBarBounds = this->bottomBarBounds();
  if (( hasCapture() && m_capturedInBottom) ||
      (!hasCapture() && bottomBarBounds.contains(pos)))
    return getBottomBarColor(u, umax);

  const gfx::Rect alphaBarBounds = this->alphaBarBounds();
  if (( hasCapture() && m_capturedInAlpha) ||
      (!hasCapture() && alphaBarBounds.contains(pos)))
    return getAlphaBarColor(u, umax);

  const int v = pos.y - rc.y;
  const int vmax = std::max(1, rc.h-bottomBarBounds.h-alphaBarBounds.h-1);
  return getMainAreaColor(u, umax,
                          v, vmax);
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
  ev.setSizeHint(gfx::Size(32*ui::guiscale(), 32*ui::guiscale()));
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
        m_capturedInMain = (hasCapture() &&
                            !m_capturedInMain &&
                            !m_capturedInBottom);
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
        if (msg->shiftPressed() ||
            msg->ctrlPressed() ||
            msg->altPressed()) {
          scale = 15.0;
        }

        double newHue = m_color.getHsvHue()
          + scale*(+ static_cast<MouseMessage*>(msg)->wheelDelta().x
                   - static_cast<MouseMessage*>(msg)->wheelDelta().y);

        while (newHue < 0.0)
          newHue += 360.0;
        newHue = std::fmod(newHue, 360.0);

        if (newHue != m_color.getHsvHue()) {
          app::Color newColor =
            app::Color::fromHsv(
              newHue,
              m_color.getHsvSaturation(),
              m_color.getHsvValue(),
              getCurrentAlphaForNewColor());

          ColorChange(newColor, kButtonNone);
        }
      }
      break;

    case kTimerMessage:
      if (m_paintFlags & DoneFlag) {
        m_timer.stop();
        invalidate();
        return true;
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
}

void ColorSelector::onResize(ui::ResizeEvent& ev)
{
  Widget::onResize(ev);

  // We'll need to redraw the whole surface again with the new widget
  // size.
  m_paintFlags = AllAreasFlag;
}

void ColorSelector::onPaint(ui::PaintEvent& ev)
{
  ui::Graphics* g = ev.graphics();
  auto theme = SkinTheme::get(this);

  theme->drawRect(g, clientBounds(),
                  theme->parts.editorNormal().get(),
                  false);       // Do not fill the center

  gfx::Rect rc = clientChildrenBounds();
  if (rc.isEmpty())
    return;

  gfx::Rect bottomBarBounds = this->bottomBarBounds();
  gfx::Rect alphaBarBounds = this->alphaBarBounds();

  os::Surface* painterSurface = nullptr;

#if SK_ENABLE_SKSL              // Paint with shaders
  if (buildEffects()) {
    SkCanvas* canvas;
    bool isSRGB;
    // TODO compare both color spaces
    if ((!get_current_color_space() ||
         get_current_color_space()->isSRGB()) &&
        (!g->getInternalSurface()->colorSpace() ||
         g->getInternalSurface()->colorSpace()->isSRGB())) {
      // We can render directly in the ui::Graphics surface
      canvas = &static_cast<os::SkiaSurface*>(g->getInternalSurface())->canvas();
      isSRGB = true;
    }
    else {
      // We'll paint in the ColorSelector::Painter canvas, and so we
      // can convert color spaces.
      painterSurface = painter.getCanvas(rc.w, rc.h, theme->colors.workspace());
      canvas = &static_cast<os::SkiaSurface*>(painterSurface)->canvas();
      isSRGB = false;
    }

    canvas->save();
    {
      SkPaint p;
      p.setStyle(SkPaint::kFill_Style);

      // Main area
      gfx::Rect rc2(0, 0, rc.w, std::max(1, rc.h-bottomBarBounds.h-alphaBarBounds.h));

      SkRuntimeShaderBuilder builder1(m_mainEffect);
      builder1.uniform("iRes") = SkV3{float(rc2.w), float(rc2.h), 0.0f};
      setShaderParams(builder1, true);
      p.setShader(builder1.makeShader());

      if (isSRGB)
        canvas->translate(rc.x+g->getInternalDeltaX(),
                          rc.y+g->getInternalDeltaY());

      canvas->drawRect(SkRect::MakeXYWH(0, 0, rc2.w, rc2.h), p);

      // Bottom bar
      canvas->translate(0.0, rc2.h);
      rc2.h = bottomBarBounds.h;

      SkRuntimeShaderBuilder builder2(m_bottomEffect);
      builder2.uniform("iRes") = SkV3{float(rc2.w), float(rc2.h), 0.0f};
      setShaderParams(builder2, false);
      p.setShader(builder2.makeShader());

      canvas->drawRect(SkRect::MakeXYWH(0, 0, rc2.w, rc2.h), p);

      // Alpha bar
      canvas->translate(0.0, rc2.h);
      rc2.h = alphaBarBounds.h;

      SkRuntimeShaderBuilder builder3(m_alphaEffect);
      builder3.uniform("iRes") = SkV3{float(rc2.w), float(rc2.h), 0.0f};
      builder3.uniform("iColor") = appColor_to_SkV4(m_color);
      builder3.uniform("iBg1") = gfxColor_to_SkV4(grid_color1());
      builder3.uniform("iBg2") = gfxColor_to_SkV4(grid_color2());
      p.setShader(builder3.makeShader());

      canvas->drawRect(SkRect::MakeXYWH(0, 0, rc2.w, rc2.h), p);
    }
    canvas->restore();

    // We already painted all areas
    m_paintFlags = 0;
  }
  else
#endif // SK_ENABLE_SKSL
  {
    painterSurface = painter.getCanvas(rc.w, rc.h, theme->colors.workspace());
  }

  if (painterSurface)
    g->drawSurface(painterSurface, rc.x, rc.y);

  rc.h -= bottomBarBounds.h + alphaBarBounds.h;
  onPaintMainArea(g, rc);

  if (!bottomBarBounds.isEmpty()) {
    bottomBarBounds.offset(-bounds().origin());
    onPaintBottomBar(g, bottomBarBounds);
  }

  if (!alphaBarBounds.isEmpty()) {
    alphaBarBounds.offset(-bounds().origin());
    onPaintAlphaBar(g, alphaBarBounds);
  }

  if ((m_paintFlags & AllAreasFlag) != 0) {
    m_paintFlags &= ~DoneFlag;
    m_timer.start();

    gfx::Point d = -rc.origin();
    rc.offset(d);
    if (!bottomBarBounds.isEmpty()) bottomBarBounds.offset(d);
    if (!alphaBarBounds.isEmpty()) alphaBarBounds.offset(d);
    painter.startBgPainting(this, rc, bottomBarBounds, alphaBarBounds);
  }
}

void ColorSelector::onPaintAlphaBar(ui::Graphics* g, const gfx::Rect& rc)
{
  const double lit = m_color.getHslLightness();
  const int alpha = m_color.getAlpha();
  const gfx::Point pos(rc.x + int(rc.w * alpha / 255),
                       rc.y + rc.h/2);
  paintColorIndicator(g, pos, lit < 0.5);
}

void ColorSelector::onPaintSurfaceInBgThread(
  os::Surface* s,
  const gfx::Rect& main,
  const gfx::Rect& bottom,
  const gfx::Rect& alpha,
  bool& stop)
{
  if ((m_paintFlags & AlphaBarFlag) && !alpha.isEmpty()) {
    draw_alpha_slider(s, alpha, m_color);
    if (stop)
      return;
    m_paintFlags ^= AlphaBarFlag;
  }
}

int ColorSelector::onNeedsSurfaceRepaint(const app::Color& newColor)
{
  return (m_color.getRed()   != newColor.getRed()   ||
          m_color.getGreen() != newColor.getGreen() ||
          m_color.getBlue()  != newColor.getBlue()  ? AlphaBarFlag: 0);
}

void ColorSelector::paintColorIndicator(ui::Graphics* g,
                                        const gfx::Point& pos,
                                        const bool white)
{
  auto theme = SkinTheme::get(this);
  os::Surface* icon = theme->parts.colorWheelIndicator()->bitmap(0);

  g->drawColoredRgbaSurface(
    icon,
    white ? gfx::rgba(255, 255, 255): gfx::rgba(0, 0, 0),
    pos.x-icon->width()/2,
    pos.y-icon->height()/2);
}

int ColorSelector::getCurrentAlphaForNewColor() const
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
  if (rc.h > 2*size) {
    if (rc.h > 3*size)          // Alpha bar is visible too
      return gfx::Rect(rc.x, rc.y2()-size*2, rc.w, size);
    else
      return gfx::Rect(rc.x, rc.y2()-size, rc.w, size);
  }
  else
    return gfx::Rect();
}

gfx::Rect ColorSelector::alphaBarBounds() const
{
  auto theme = SkinTheme::get(this);
  const gfx::Rect rc = childrenBounds();
  const int size = theme->dimensions.colorSelectorBarSize();
  if (rc.h > 3*size)
    return gfx::Rect(rc.x, rc.y2()-size, rc.w, size);
  else
    return gfx::Rect();
}

void ColorSelector::updateColorSpace()
{
  m_paintFlags |= AllAreasFlag;
  invalidate();
}

#if SK_ENABLE_SKSL

// static
const char* ColorSelector::getAlphaBarShader()
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
  if (!Preferences::instance().experimental.useShadersForColorSelectors())
    return false;

  if (!m_mainEffect) {
    if (const char* code = getMainAreaShader())
      m_mainEffect = buildEffect(code);
  }

  if (!m_bottomEffect) {
    if (const char* code = getBottomBarShader())
      m_bottomEffect = buildEffect(code);
  }

  if (!m_alphaEffect) {
    if (const char* code = getAlphaBarShader())
      m_alphaEffect = buildEffect(code);
  }

  return (m_mainEffect && m_bottomEffect && m_alphaEffect);
}

sk_sp<SkRuntimeEffect> ColorSelector::buildEffect(const char* code)
{
  auto result = SkRuntimeEffect::MakeForShader(SkString(code));
  if (!result.errorText.isEmpty()) {
    LOG(ERROR, "Shader error: %s\n", result.errorText.c_str());
    std::printf("Shader error: %s\n", result.errorText.c_str());
    return nullptr;
  }
  else {
    return result.effect;
  }
}

void ColorSelector::resetBottomEffect()
{
  m_bottomEffect.reset();
}

#endif  // SK_ENABLE_SKSL

} // namespace app
