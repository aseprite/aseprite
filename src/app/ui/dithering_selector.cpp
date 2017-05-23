// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/dithering_selector.h"

#include "app/modules/palettes.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "doc/conversion_she.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/primitives.h"
#include "render/quantization.h"
#include "she/surface.h"
#include "she/system.h"
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
    , m_algo(algo)
    , m_matrix(matrix) {
    generatePreview();
  }

  render::DitheringAlgorithm algo() const { return m_algo; }
  render::DitheringMatrix matrix() const { return m_matrix; }

private:
  void generatePreview() {
    const doc::Palette* palette = get_current_palette();

    const int w = 128, h = 16;
    doc::ImageRef image1(doc::Image::create(doc::IMAGE_RGB, w, h));
    doc::clear_image(image1.get(), 0);
    for (int y=0; y<h; ++y)
      for (int x=0; x<w; ++x) {
        int v = 255 * x / (w-1);
        image1->putPixel(x, y, doc::rgba(v, v, v, 255));
      }

    doc::ImageRef image2(doc::Image::create(doc::IMAGE_INDEXED, w, h));
    doc::clear_image(image2.get(), 0);
    render::convert_pixel_format(
      image1.get(), image2.get(), IMAGE_INDEXED,
      m_algo, m_matrix, nullptr, palette, true, -1, nullptr);

    m_preview = she::instance()->createRgbaSurface(w, h);
    doc::convert_image_to_surface(image2.get(), palette, m_preview,
                                  0, 0, 0, 0, w, h);
  }

  void onSizeHint(SizeHintEvent& ev) override {
    gfx::Size sz = textSize();

    sz.w = MAX(sz.w, m_preview->width()) + 4*guiscale();
    sz.h += 6*guiscale() + m_preview->height();

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
      m_preview,
      rc.x+2*guiscale(),
      rc.y+4*guiscale()+textsz.h);
  }

  render::DitheringAlgorithm m_algo;
  render::DitheringMatrix m_matrix;
  she::Surface* m_preview;
};

} // anonymous namespace

DitheringSelector::DitheringSelector()
{
  setUseCustomWidget(true);

  addItem(new DitherItem(render::DitheringAlgorithm::None,
                         render::DitheringMatrix(), "No Dithering"));
  addItem(new DitherItem(render::DitheringAlgorithm::Ordered,
                         render::BayerMatrix(8), "Ordered Dithering - Bayer Matrix 8x8"));
  addItem(new DitherItem(render::DitheringAlgorithm::Ordered,
                         render::BayerMatrix(4), "Ordered Dithering - Bayer Matrix 4x4"));
  addItem(new DitherItem(render::DitheringAlgorithm::Ordered,
                         render::BayerMatrix(2), "Ordered Dithering - Bayer Matrix 2x2"));
  addItem(new DitherItem(render::DitheringAlgorithm::Old,
                         render::BayerMatrix(8), "Old Dithering - Bayer Matrix 8x8"));
  addItem(new DitherItem(render::DitheringAlgorithm::Old,
                         render::BayerMatrix(4), "Old Dithering - Bayer Matrix 4x4"));
  addItem(new DitherItem(render::DitheringAlgorithm::Old,
                         render::BayerMatrix(2), "Old Dithering - Bayer Matrix 2x2"));

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
