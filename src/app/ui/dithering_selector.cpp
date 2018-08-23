// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/dithering_selector.h"

#include "app/app.h"
#include "app/extensions.h"
#include "app/modules/palettes.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "doc/conversion_to_surface.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/primitives.h"
#include "render/gradient.h"
#include "render/quantization.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/graphics.h"
#include "ui/listitem.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"

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
    , m_algo(algo)
    , m_matrix(matrix)
    , m_preview(nullptr)
    , m_palId(0)
    , m_palMods(0)
  {
  }

  DitherItem(const render::DitheringMatrix& matrix,
             const std::string& text)
    : ListItem(text)
    , m_matrixOnly(true)
    , m_algo(render::DitheringAlgorithm::None)
    , m_matrix(matrix)
    , m_preview(nullptr)
    , m_palId(0)
    , m_palMods(0)
  {
  }

  render::DitheringAlgorithm algo() const { return m_algo; }
  render::DitheringMatrix matrix() const { return m_matrix; }

private:
  os::Surface* preview() {
    const doc::Palette* palette = get_current_palette();
    ASSERT(palette);

    if (m_preview) {
      // Reuse the preview in case that the palette is exactly the same
      if (palette->id() == m_palId &&
          palette->getModifications() == m_palMods)
        return m_preview;

      // In other case regenerate the preview for the current palette
      m_preview->dispose();
      m_preview = nullptr;
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
      m_matrixOnly ? m_matrix: render::DitheringMatrix());

    doc::ImageRef image2;
    if (m_matrixOnly) {
      image2 = image1;
    }
    else {
      image2.reset(doc::Image::create(doc::IMAGE_INDEXED, w, h));
      doc::clear_image(image2.get(), 0);
      render::convert_pixel_format(
        image1.get(), image2.get(), IMAGE_INDEXED,
        m_algo, m_matrix, nullptr, palette, true, -1, nullptr);
    }

    m_preview = os::instance()->createRgbaSurface(w, h);
    doc::convert_image_to_surface(image2.get(), palette, m_preview,
                                  0, 0, 0, 0, w, h);

    m_palId = palette->id();
    m_palMods = palette->getModifications();
    return m_preview;
  }

  void onSizeHint(SizeHintEvent& ev) override {
    gfx::Size sz = textSize();

    sz.w = MAX(sz.w, preview()->width()) + 4*guiscale();
    sz.h += 6*guiscale() + preview()->height();

    ev.setSizeHint(sz);
  }

  void onPaint(PaintEvent& ev) override {
    Graphics* g = ev.graphics();
    skin::SkinTheme* theme = static_cast<skin::SkinTheme*>(this->theme());

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
    g->drawRgbaSurface(
      preview(),
      rc.x+2*guiscale(),
      rc.y+4*guiscale()+textsz.h);
  }

  bool m_matrixOnly;
  render::DitheringAlgorithm m_algo;
  render::DitheringMatrix m_matrix;
  os::Surface* m_preview;
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
      base::Bind<void>(&DitheringSelector::regenerate, this));

  setUseCustomWidget(true);
  regenerate();
}

void DitheringSelector::regenerate()
{
  removeAllItems();

  Extensions& extensions = App::instance()->extensions();
  auto ditheringMatrices = extensions.ditheringMatrices();

  switch (m_type) {
    case SelectBoth:
      addItem(new DitherItem(render::DitheringAlgorithm::None,
                             render::DitheringMatrix(), "No Dithering"));
      for (const auto& it : ditheringMatrices) {
        addItem(
          new DitherItem(
            render::DitheringAlgorithm::Ordered,
            it.matrix(),
            "Ordered Dithering+" + it.name()));
      }
      for (const auto& it : ditheringMatrices) {
        addItem(
          new DitherItem(
            render::DitheringAlgorithm::Old,
            it.matrix(),
            "Old Dithering+" + it.name()));
      }
      break;
    case SelectMatrix:
      addItem(new DitherItem(render::DitheringMatrix(), "No Dithering"));
      for (auto& it : ditheringMatrices)
        addItem(new DitherItem(it.matrix(), it.name()));
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
