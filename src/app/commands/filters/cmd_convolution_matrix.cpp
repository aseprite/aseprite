// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/commands/command.h"
#include "app/commands/filters/convolution_matrix_stock.h"
#include "app/commands/filters/filter_manager_impl.h"
#include "app/commands/filters/filter_window.h"
#include "app/commands/filters/filter_worker.h"
#include "app/commands/new_params.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/find_widget.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/pref/preferences.h"
#include "base/bind.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "filters/convolution_matrix.h"
#include "filters/convolution_matrix_filter.h"
#include "ui/button.h"
#include "ui/label.h"
#include "ui/listbox.h"
#include "ui/listitem.h"
#include "ui/slider.h"
#include "ui/view.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <cstring>
#include <memory>

namespace app {

using namespace filters;
using namespace ui;

struct ConvolutionMatrixParams : public NewParams {
  Param<bool> ui { this, true, "ui" };
  Param<filters::Target> channels { this, 0, "channels" };
  Param<filters::TiledMode> tiledMode { this, filters::TiledMode::NONE, "tiledMode" };
  Param<std::string> fromResource { this, std::string(), "fromResource" };
};

#ifdef ENABLE_UI

static const char* ConfigSection = "ConvolutionMatrix";

class ConvolutionMatrixWindow : public FilterWindow {
public:
  ConvolutionMatrixWindow(ConvolutionMatrixFilter& filter, FilterManagerImpl& filterMgr, ConvolutionMatrixStock& stock)
    : FilterWindow("Convolution Matrix", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithTiledCheckBox,
                   filter.getTiledMode())
    , m_filter(filter)
    , m_controlsWidget(app::load_widget<Widget>("convolution_matrix.xml", "controls"))
    , m_stock(stock)
    , m_view(app::find_widget<View>(m_controlsWidget.get(), "view"))
    , m_stockListBox(app::find_widget<ListBox>(m_controlsWidget.get(), "stock"))
    , m_reloadButton(app::find_widget<Button>(m_controlsWidget.get(), "reload"))
  {
    getContainer()->addChild(m_controlsWidget.get());

    m_reloadButton->Click.connect(&ConvolutionMatrixWindow::onReloadStock, this);
    m_stockListBox->Change.connect(base::Bind<void>(&ConvolutionMatrixWindow::onMatrixChange, this));

    fillStockListBox();
  }

private:
  void onReloadStock(Event& ev) {
    m_stock.reloadStock();
    fillStockListBox();
  }

  void setupTiledMode(TiledMode tiledMode) override {
    m_filter.setTiledMode(tiledMode);
  }

  void fillStockListBox() {
    const char* oldSelected = (m_filter.getMatrix() ? m_filter.getMatrix()->getName(): NULL);

    // Clean the list
    while (!m_stockListBox->children().empty()) {
      Widget* listitem = m_stockListBox->children().front();
      m_stockListBox->removeChild(listitem);
      delete listitem;
    }

    for (ConvolutionMatrixStock::iterator it = m_stock.begin(), end = m_stock.end();
         it != end; ++it) {
      std::shared_ptr<ConvolutionMatrix> matrix = *it;
      ListItem* listitem = new ListItem(matrix->getName());
      m_stockListBox->addChild(listitem);
    }

    selectMatrixByName(oldSelected);
  }

  void selectMatrixByName(const char* oldSelected)
  {
    Widget* select_this = UI_FIRST_WIDGET(m_stockListBox->children());

    if (oldSelected) {
      for (auto child : m_stockListBox->children()) {
        if (child->text() == oldSelected) {
          select_this = child;
          break;
        }
      }
    }

    if (select_this) {
      select_this->setSelected(true);
      onMatrixChange();
    }

    m_view->updateView();
  }

  void onMatrixChange()
  {
    Widget* selected = m_stockListBox->getSelectedChild();
    std::shared_ptr<ConvolutionMatrix> matrix = m_stock.getByName(selected->text().c_str());
    Target newTarget = matrix->getDefaultTarget();

    m_filter.setMatrix(matrix);

    setNewTarget(newTarget);

    restartPreview();
  }

  ConvolutionMatrixFilter& m_filter;
  std::unique_ptr<Widget> m_controlsWidget;
  ConvolutionMatrixStock& m_stock;
  View* m_view;
  ListBox* m_stockListBox;
  Button* m_reloadButton;
};

#endif  // ENABLE_UI

class ConvolutionMatrixCommand : public CommandWithNewParams<ConvolutionMatrixParams> {
public:
  ConvolutionMatrixCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

ConvolutionMatrixCommand::ConvolutionMatrixCommand()
  : CommandWithNewParams<ConvolutionMatrixParams>(CommandId::ConvolutionMatrix(), CmdRecordableFlag)
{
}

bool ConvolutionMatrixCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void ConvolutionMatrixCommand::onExecute(Context* context)
{
#ifdef ENABLE_UI
  const bool ui = (params().ui() && context->isUIAvailable());
#endif

  static ConvolutionMatrixStock stock; // Load stock
  ConvolutionMatrixFilter filter; // Create the filter and setup initial settings

  std::shared_ptr<ConvolutionMatrix> matrix;
#ifdef ENABLE_UI
  if (ui) {
    // Get last used (selected) matrix
    matrix = stock.getByName(get_config_string(ConfigSection, "Selected", ""));

    DocumentPreferences& docPref = Preferences::instance()
      .document(context->activeDocument());
    filter.setTiledMode(docPref.tiled.mode());
  }
#endif // ENABLE_UI

  if (params().tiledMode.isSet()) filter.setTiledMode(params().tiledMode());
  if (params().fromResource.isSet()) matrix = stock.getByName(params().fromResource().c_str());
  if (matrix) filter.setMatrix(matrix);

  FilterManagerImpl filterMgr(context, &filter);

#ifdef ENABLE_UI
  if (ui) {
    ConvolutionMatrixWindow window(filter, filterMgr, stock);
    if (window.doModal()) {
      if (filter.getMatrix())
        set_config_string(ConfigSection, "Selected", filter.getMatrix()->getName());
    }
  }
  else
#endif // ENABLE_UI
  {
    start_filter_worker(&filterMgr);
  }
}

Command* CommandFactory::createConvolutionMatrixCommand()
{
  return new ConvolutionMatrixCommand;
}

} // namespace app
