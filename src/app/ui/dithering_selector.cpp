// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/dithering_selector.h"

#include "app/app.h"
#include "app/console.h"
#include "app/extensions.h"
#include "app/i18n/strings.h"
#include "app/modules/palettes.h"
#include "app/ui/skin/skin_theme.h"
#include "app/util/conversion_to_surface.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/primitives.h"
#include "os/surface.h"
#include "os/system.h"
#include "render/dithering.h"
#include "render/gradient.h"
#include "render/quantization.h"
#include "ui/graphics.h"
#include "ui/listitem.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"

#include <algorithm>

namespace app {

using namespace ui;

namespace {

class DitherItem : public ListItem {
public:

  DitherItem(render::DitheringAlgorithm algo,
             const render::DitheringMatrix& matrix,
             const std::string& text)
    : ListItem(text)
    , m_matrixOnly(false)
    , m_dithering(algo, matrix)
    , m_preview(nullptr)
    , m_palId(0)
    , m_palMods(0)
  {
  }

  DitherItem(const render::DitheringMatrix& matrix,
             const std::string& text)
    : ListItem(text)
    , m_matrixOnly(true)
    , m_dithering(render::DitheringAlgorithm::None, matrix)
    , m_preview(nullptr)
    , m_palId(0)
    , m_palMods(0)
  {
  }

  render::DitheringAlgorithm algo() const {
    return m_dithering.algorithm();
  }

  render::DitheringMatrix matrix() const {
    return m_dithering.matrix();
  }

private:
  os::Surface* preview() {
    const doc::Palette* palette = get_current_palette();
    ASSERT(palette);

    if (m_preview) {
      // Reuse the preview in case that the palette is exactly the same
      if (palette->id() == m_palId &&
          palette->getModifications() == m_palMods)
        return m_preview.get();

      // In other case regenerate the preview for the current palette
      m_preview.reset();
    }

    const int w = 128, h = 16;
    doc::ImageRef image1(doc::Image::create(doc::IMAGE_RGB, w, h));
    render_rgba_linear_gradient(
      image1.get(),
      gfx::Point(0, 0),
      gfx::Point(0, 0),
      gfx::Point(w-1, 0),
      doc::rgba(0, 0, 0, 255),
      doc::rgba(255, 255, 255, 255),
      (m_matrixOnly ? m_dithering.matrix():
                      render::DitheringMatrix()));

    doc::ImageRef image2;
    if (m_matrixOnly) {
      image2 = image1;
    }
    else {
      image2.reset(doc::Image::create(doc::IMAGE_INDEXED, w, h));
      doc::clear_image(image2.get(), 0);
      render::convert_pixel_format(
        image1.get(), image2.get(), IMAGE_INDEXED,
        m_dithering, nullptr, palette, true, -1, nullptr);
    }

    m_preview = os::instance()->makeRgbaSurface(w, h);
    convert_image_to_surface(image2.get(), palette, m_preview.get(),
                             0, 0, 0, 0, w, h);

    m_palId = palette->id();
    m_palMods = palette->getModifications();
    return m_preview.get();
  }

  void onSizeHint(SizeHintEvent& ev) override {
    gfx::Size sz = textSize();

    sz.w = std::max(sz.w, preview()->width()*guiscale()) + 4*guiscale();
    sz.h += 6*guiscale() + preview()->height()*guiscale();

    ev.setSizeHint(sz);
  }

  void onPaint(PaintEvent& ev) override {
    Graphics* g = ev.graphics();
    auto theme = skin::SkinTheme::get(this);

    gfx::Color fg, bg;
    if (isSelected()) {
      fg = theme->colors.listitemSelectedText();
      bg = theme->colors.listitemSelectedFace();
    }
    else {
      fg = theme->colors.listitemNormalText();
      bg = theme->colors.listitemNormalFace();
    }

    gfx::Rect rc = clientBounds();
    g->fillRect(bg, rc);

    gfx::Size textsz = textSize();
    g->drawText(text(), fg, bg,
                gfx::Point(rc.x+2*guiscale(),
                           rc.y+2*guiscale()));

    ui::Paint paint;
    paint.blendMode(os::BlendMode::SrcOver);

    g->drawSurface(
      preview(),
      preview()->bounds(),
      gfx::Rect(
        rc.x+2*guiscale(),
        rc.y+4*guiscale()+textsz.h,
        preview()->width()*guiscale(),
        preview()->height()*guiscale()),
      os::Sampling(),
      &paint);
  }

  bool m_matrixOnly;
  render::Dithering m_dithering;
  os::SurfaceRef m_preview;
  doc::ObjectId m_palId;
  int m_palMods;
};

} // anonymous namespace

DitheringSelector::DitheringSelector(Type type)
  : m_type(type)
{
  Extensions& extensions = App::instance()->extensions();

  // If an extension with "ditheringMatrices" is disable/enable, we
  // regenerate this DitheringSelector
  m_extChanges =
    extensions.DitheringMatricesChange.connect(
      [this]{ regenerate(); });

  setUseCustomWidget(true);
  regenerate();
}

void DitheringSelector::onInitTheme(ui::InitThemeEvent& ev)
{
  ComboBox::onInitTheme(ev);
  if (getItem(0))
    setSizeHint(getItem(0)->sizeHint());
}

void DitheringSelector::regenerate()
{
  deleteAllItems();

  Extensions& extensions = App::instance()->extensions();
  auto ditheringMatrices = extensions.ditheringMatrices();

  switch (m_type) {
    case SelectBoth:
      addItem(new DitherItem(render::DitheringAlgorithm::None,
                             render::DitheringMatrix(),
                             Strings::dithering_selector_no_dithering()));
      for (const auto& it : ditheringMatrices) {
        try {
          addItem(new DitherItem(
            render::DitheringAlgorithm::Ordered,
            it.matrix(),
            Strings::dithering_selector_ordered_dithering() + it.name()));
        }
        catch (const std::exception& e) {
          LOG(ERROR, "%s\n", e.what());
          Console::showException(e);
        }
      }
      for (const auto& it : ditheringMatrices) {
        try {
          addItem(
            new DitherItem(
              render::DitheringAlgorithm::Old,
              it.matrix(),
              Strings::dithering_selector_old_dithering() + it.name()));
        }
        catch (const std::exception& e) {
          LOG(ERROR, "%s\n", e.what());
          Console::showException(e);
        }
      }
      addItem(
        new DitherItem(
          render::DitheringAlgorithm::ErrorDiffusion,
          render::DitheringMatrix(),
          Strings::dithering_selector_floyd_steinberg()));
      break;
    case SelectMatrix:
      addItem(new DitherItem(render::DitheringMatrix(),
                             Strings::dithering_selector_no_dithering()));
      for (auto& it : ditheringMatrices) {
        try {
          addItem(new DitherItem(it.matrix(), it.name()));
        }
        catch (const std::exception& e) {
          LOG(ERROR, "%s\n", e.what());
          Console::showException(e);
        }
      }
      break;
  }

  setSelectedItemIndex(0);
  setSizeHint(getItem(0)->sizeHint());
}

render::DitheringAlgorithm DitheringSelector::ditheringAlgorithm()
{
  auto item = static_cast<DitherItem*>(getSelectedItem());
  if (item)
    return item->algo();
  else
    return render::DitheringAlgorithm::None;
}

render::DitheringMatrix DitheringSelector::ditheringMatrix()
{
  auto item = static_cast<DitherItem*>(getSelectedItem());
  if (item)
    return item->matrix();
  else
    return render::DitheringMatrix();
}

} // namespace app
