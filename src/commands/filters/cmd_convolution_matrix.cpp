/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <string.h>

#include "app/color.h"
#include "base/bind.h"
#include "commands/command.h"
#include "commands/filters/convolution_matrix_stock.h"
#include "commands/filters/filter_manager_impl.h"
#include "commands/filters/filter_window.h"
#include "document_wrappers.h"
#include "filters/convolution_matrix.h"
#include "filters/convolution_matrix_filter.h"
#include "gui/button.h"
#include "gui/frame.h"
#include "gui/label.h"
#include "gui/list.h"
#include "gui/listbox.h"
#include "gui/slider.h"
#include "gui/view.h"
#include "gui/widget.h"
#include "ini_file.h"
#include "modules/gui.h"
#include "raster/mask.h"
#include "raster/sprite.h"

static const char* ConfigSection = "ConvolutionMatrix";

//////////////////////////////////////////////////////////////////////
// Convolution Matrix window

class ConvolutionMatrixWindow : public FilterWindow
{
public:
  ConvolutionMatrixWindow(ConvolutionMatrixFilter& filter, FilterManagerImpl& filterMgr, ConvolutionMatrixStock& stock)
    : FilterWindow("Convolution Matrix", ConfigSection, &filterMgr,
                   WithChannelsSelector,
                   WithTiledCheckBox,
                   filter.getTiledMode())
    , m_filter(filter)
    , m_controlsWidget(load_widget("convolution_matrix.xml", "controls"))
    , m_stock(stock)
  {
    get_widgets(m_controlsWidget,
                "view", &m_view,
                "stock", &m_stockListBox,
                "reload", &m_reloadButton, NULL);

    getContainer()->addChild(m_controlsWidget);

    m_reloadButton->Click.connect(&ConvolutionMatrixWindow::onReloadStock, this);
    // TODO convert listbox to c++ class ListBox
    //m_stockListBox->Change.connect(Bind<void>(&ConvolutionMatrixWindow::onMatrixChange, this));
    hook_signal(m_stockListBox, JI_SIGNAL_LISTBOX_CHANGE, &ConvolutionMatrixWindow::listboxChangeHandler, this);

    fillStockListBox();
  }

private:
  void onReloadStock(Event& ev)
  {
    m_stock.reloadStock();
    fillStockListBox();
  }

  void setupTiledMode(TiledMode tiledMode)
  {
    m_filter.setTiledMode(tiledMode);
  }

  void fillStockListBox()
  {
    const char* oldSelected = (m_filter.getMatrix() ? m_filter.getMatrix()->getName(): NULL);
    JLink link, next;

    // Clean the list
    JI_LIST_FOR_EACH_SAFE(m_stockListBox->children, link, next) {
      Widget* listitem = reinterpret_cast<Widget*>(link->data);
      m_stockListBox->removeChild(listitem);
      jwidget_free(listitem);
    }

    for (ConvolutionMatrixStock::iterator it = m_stock.begin(), end = m_stock.end();
         it != end; ++it) {
      SharedPtr<ConvolutionMatrix> matrix = *it;
      Widget* listitem = jlistitem_new(matrix->getName()); // TODO convert listitem to a class
      m_stockListBox->addChild(listitem);
    }

    selectMatrixByName(oldSelected);
  }

  void selectMatrixByName(const char* oldSelected)
  {
    Widget* select_this = reinterpret_cast<Widget*>(jlist_first_data(m_stockListBox->children));

    if (oldSelected) {
      JLink link;
      JI_LIST_FOR_EACH(m_stockListBox->children, link) {
        Widget* child = reinterpret_cast<Widget*>(link->data);

        if (strcmp(child->getText(), oldSelected) == 0) {
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
    Widget* selected = jlistbox_get_selected_child(m_stockListBox);
    SharedPtr<ConvolutionMatrix> matrix = m_stock.getByName(selected->getText());
    Target newTarget = matrix->getDefaultTarget();

    m_filter.setMatrix(matrix);

    setNewTarget(newTarget);

    restartPreview();
  }

  // TODO This function must be removed if ListBox C++ class is added.
  static bool listboxChangeHandler(Widget* widget, void *data)
  {
    ConvolutionMatrixWindow* window = (ConvolutionMatrixWindow*)data;
    window->onMatrixChange();
    return true;
  }

  ConvolutionMatrixFilter& m_filter;
  WidgetPtr m_controlsWidget;
  ConvolutionMatrixStock& m_stock;
  View* m_view;
  Widget* m_stockListBox;
  Button* m_reloadButton;
};

//////////////////////////////////////////////////////////////////////
// Convolution Matrix command

class ConvolutionMatrixCommand : public Command
{
public:
  ConvolutionMatrixCommand();
  Command* clone() const { return new ConvolutionMatrixCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

ConvolutionMatrixCommand::ConvolutionMatrixCommand()
  : Command("ConvolutionMatrix",
            "Convolution Matrix",
            CmdRecordableFlag)
{
}

bool ConvolutionMatrixCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void ConvolutionMatrixCommand::onExecute(Context* context)
{
  // Load stock
  ConvolutionMatrixStock m_stock;

  // Get last used (selected) matrix
  SharedPtr<ConvolutionMatrix> matrix =
    m_stock.getByName(get_config_string(ConfigSection, "Selected", ""));

  // Create the filter and setup initial settings
  ConvolutionMatrixFilter filter;
  filter.setTiledMode(context->getSettings()->getTiledMode());
  if (matrix != 0)
    filter.setMatrix(matrix);

  FilterManagerImpl filterMgr(context->getActiveDocument(), &filter);

  ConvolutionMatrixWindow window(filter, filterMgr, m_stock);
  if (window.doModal()) {
    if (filter.getMatrix() != NULL)
      set_config_string(ConfigSection, "Selected", filter.getMatrix()->getName());
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createConvolutionMatrixCommand()
{
  return new ConvolutionMatrixCommand;
}
