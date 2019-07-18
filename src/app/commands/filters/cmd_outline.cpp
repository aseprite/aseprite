// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color.h"
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/commands/filters/filter_window.h"
#include "app/commands/filters/filter_worker.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/ini_file.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/ui/color_bar.h"
#include "app/ui/color_button.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "filters/outline_filter.h"
#include "ui/button.h"
#include "ui/label.h"
#include "ui/slider.h"
#include "ui/widget.h"
#include "ui/window.h"

#include "outline.xml.h"

namespace app {

using namespace app::skin;

enum { CIRCLE, SQUARE, HORZ, VERT };

struct OutlineParams : public NewParams {
  Param<bool> ui { this, true, "ui" };
  Param<filters::Target> channels { this, 0, "channels" };
  Param<filters::OutlineFilter::Place> place { this, OutlineFilter::Place::Outside, "place" };
  Param<filters::OutlineFilter::Matrix> matrix { this, OutlineFilter::Matrix::Circle, "matrix" };
  Param<app::Color> color { this, app::Color(), "color" };
  Param<app::Color> bgColor { this, app::Color(), "bgColor" };
  Param<filters::TiledMode> tiledMode { this, filters::TiledMode::NONE, "tiledMode" };
};

// Wrapper for ReplaceColorFilter to handle colors in an easy way
class OutlineFilterWrapper : public OutlineFilter {
public:
  OutlineFilterWrapper(Layer* layer) : m_layer(layer) { }

  void color(const app::Color& color) {
    m_color = color;
    if (m_layer)
      OutlineFilter::color(color_utils::color_for_layer(color, m_layer));
  }

  void bgColor(const app::Color& color) {
    m_bgColor = color;
    if (m_layer)
      OutlineFilter::bgColor(color_utils::color_for_layer(color, m_layer));
  }

  app::Color color() const { return m_color; }
  app::Color bgColor() const { return m_bgColor; }

private:
  Layer* m_layer;
  app::Color m_color;
  app::Color m_bgColor;
};

#ifdef ENABLE_UI

static const char* ConfigSection = "Outline";

class OutlineWindow : public FilterWindow {
public:
  OutlineWindow(OutlineFilterWrapper& filter,
                FilterManagerImpl& filterMgr)
    : FilterWindow("Outline", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithTiledCheckBox,
                   filter.tiledMode())
    , m_filter(filter) {
    getContainer()->addChild(&m_panel);

    m_panel.color()->setColor(m_filter.color());
    m_panel.bgColor()->setColor(m_filter.bgColor());
    m_panel.place()->setSelectedItem((int)m_filter.place());
    updateButtonsFromMatrix();

    m_panel.color()->Change.connect(&OutlineWindow::onColorChange, this);
    m_panel.bgColor()->Change.connect(&OutlineWindow::onBgColorChange, this);
    m_panel.outlineType()->ItemChange.connect(
      [this](ButtonSet::Item*){
        onMatrixTypeChange();
      });
    m_panel.outlineMatrix()->ItemChange.connect(
      [this](ButtonSet::Item* item){
        onMatrixPixelChange(m_panel.outlineMatrix()->getItemIndex(item));
      });
    m_panel.place()->ItemChange.connect(
      [this](ButtonSet::Item*){
        onPlaceChange((OutlineFilter::Place)m_panel.place()->selectedItem());
      });
  }

private:
  void updateButtonsFromMatrix() {
    const OutlineFilter::Matrix matrix = m_filter.matrix();

    int commonMatrix = -1;
    switch (matrix) {
      case OutlineFilter::Matrix::Circle:     commonMatrix = CIRCLE; break;
      case OutlineFilter::Matrix::Square:     commonMatrix = SQUARE; break;
      case OutlineFilter::Matrix::Horizontal: commonMatrix = HORZ; break;
      case OutlineFilter::Matrix::Vertical:   commonMatrix = VERT; break;
    }
    m_panel.outlineType()->setSelectedItem(commonMatrix, false);

    auto theme = static_cast<SkinTheme*>(this->theme());
    auto emptyIcon = theme->parts.outlineEmptyPixel();
    auto pixelIcon = theme->parts.outlineFullPixel();

    for (int i=0; i<9; ++i) {
      m_panel.outlineMatrix()
        ->getItem(i)->setIcon(
          (((int)matrix) & (1 << (8-i))) ? pixelIcon: emptyIcon);
    }
  }

  void onColorChange(const app::Color& color) {
    m_filter.color(color);
    restartPreview();
  }

  void onBgColorChange(const app::Color& color) {
    m_filter.bgColor(color);
    restartPreview();
  }

  void onPlaceChange(OutlineFilter::Place place) {
    m_filter.place(place);
    restartPreview();
  }

  void onMatrixTypeChange() {
    OutlineFilter::Matrix matrix = OutlineFilter::Matrix::None;
    switch (m_panel.outlineType()->selectedItem()) {
      case CIRCLE: matrix = OutlineFilter::Matrix::Circle; break;
      case SQUARE: matrix = OutlineFilter::Matrix::Square; break;
      case HORZ: matrix = OutlineFilter::Matrix::Horizontal; break;
      case VERT: matrix = OutlineFilter::Matrix::Vertical; break;
    }
    m_filter.matrix(matrix);
    updateButtonsFromMatrix();
    restartPreview();
  }

  void onMatrixPixelChange(const int index) {
    int matrix = (int)m_filter.matrix();
    matrix ^= (1 << (8-index));
    m_filter.matrix((OutlineFilter::Matrix)matrix);
    updateButtonsFromMatrix();
    restartPreview();
  }

  void setupTiledMode(TiledMode tiledMode) override {
    m_filter.tiledMode(tiledMode);
  }

  OutlineFilterWrapper& m_filter;
  gen::Outline m_panel;
};

#endif  // ENABLE_UI

class OutlineCommand : public CommandWithNewParams<OutlineParams> {
public:
  OutlineCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

OutlineCommand::OutlineCommand()
  : CommandWithNewParams<OutlineParams>(CommandId::Outline(), CmdRecordableFlag)
{
}

bool OutlineCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void OutlineCommand::onExecute(Context* context)
{
#ifdef ENABLE_UI
  const bool ui = (params().ui() && context->isUIAvailable());
#endif

  Site site = context->activeSite();

  OutlineFilterWrapper filter(site.layer());
  if (site.layer() && site.layer()->isBackground() && site.image()) {
    // TODO configure default pixel (same as Autocrop/Trim refpixel)
    filter.bgColor(app::Color::fromImage(site.image()->pixelFormat(),
                                         site.image()->getPixel(0, 0)));
  }
  else {
    filter.bgColor(app::Color::fromMask());
  }

#ifdef ENABLE_UI
  if (ui) {
    filter.place((OutlineFilter::Place)get_config_int(ConfigSection, "Place", int(OutlineFilter::Place::Outside)));
    filter.matrix((OutlineFilter::Matrix)get_config_int(ConfigSection, "Matrix", int(OutlineFilter::Matrix::Circle)));
    filter.color(ColorBar::instance()->getFgColor());

    DocumentPreferences& docPref = Preferences::instance()
      .document(site.document());
    filter.tiledMode(docPref.tiled.mode());
  }
#endif // ENABLE_UI

  if (params().place.isSet()) filter.place(params().place());
  if (params().matrix.isSet()) filter.matrix(params().matrix());
  if (params().color.isSet()) filter.color(params().color());
  if (params().bgColor.isSet()) filter.bgColor(params().bgColor());
  if (params().tiledMode.isSet()) filter.tiledMode(params().tiledMode());

  FilterManagerImpl filterMgr(context, &filter);
  filterMgr.setTarget(
    site.sprite()->pixelFormat() == IMAGE_INDEXED ?
    TARGET_INDEX_CHANNEL:
    TARGET_RED_CHANNEL |
    TARGET_GREEN_CHANNEL |
    TARGET_BLUE_CHANNEL |
    TARGET_GRAY_CHANNEL |
    TARGET_ALPHA_CHANNEL);

  if (params().channels.isSet()) filterMgr.setTarget(params().channels());

#ifdef ENABLE_UI
  if (ui) {
    OutlineWindow window(filter, filterMgr);
    if (window.doModal()) {
      set_config_int(ConfigSection, "Place", int(filter.place()));
      set_config_int(ConfigSection, "Matrix", int(filter.matrix()));
    }
  }
  else
#endif // ENABLE_UI
  {
    start_filter_worker(&filterMgr);
  }
}

Command* CommandFactory::createOutlineCommand()
{
  return new OutlineCommand;
}

} // namespace app
