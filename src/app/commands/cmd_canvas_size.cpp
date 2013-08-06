/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/find_widget.h"
#include "app/load_widget.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/settings/settings.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/select_box_state.h"
#include "app/undo_transaction.h"
#include "base/bind.h"
#include "base/unique_ptr.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "ui/ui.h"

#include <allegro/unicode.h>

namespace app {

using namespace ui;

// Disable warning about usage of "this" in initializer list.
#pragma warning(disable:4355)

// Window used to show canvas parameters.
class CanvasSizeWindow : public Window
                       , public SelectBoxDelegate
{
public:
  CanvasSizeWindow(int left, int top, int right, int bottom)
    : Window(false, "Canvas Size")
    , m_editor(current_editor)
    , m_rect(-left, -top,
             current_editor->getSprite()->getWidth() + left + right,
             current_editor->getSprite()->getHeight() + top + bottom)
    , m_selectBoxState(new SelectBoxState(this, m_rect,
                                          SelectBoxState::PaintRulers |
                                          SelectBoxState::PaintDarkOutside))
    , m_mainBox(app::load_widget<Widget>("canvas_size.xml", "main_box"))
    , m_left(app::find_widget<Entry>(m_mainBox, "left"))
    , m_top(app::find_widget<Entry>(m_mainBox, "top"))
    , m_right(app::find_widget<Entry>(m_mainBox, "right"))
    , m_bottom(app::find_widget<Entry>(m_mainBox, "bottom"))
    , m_ok(app::find_widget<Button>(m_mainBox, "ok"))
  {
    addChild(m_mainBox);

    m_left->setTextf("%d", left);
    m_right->setTextf("%d", right);
    m_top->setTextf("%d", top);
    m_bottom->setTextf("%d", bottom);

    m_left  ->EntryChange.connect(Bind<void>(&CanvasSizeWindow::onEntriesChange, this));
    m_right ->EntryChange.connect(Bind<void>(&CanvasSizeWindow::onEntriesChange, this));
    m_top   ->EntryChange.connect(Bind<void>(&CanvasSizeWindow::onEntriesChange, this));
    m_bottom->EntryChange.connect(Bind<void>(&CanvasSizeWindow::onEntriesChange, this));

    m_editor->setState(m_selectBoxState);
  }

  ~CanvasSizeWindow()
  {
    m_editor->backToPreviousState();
  }

  bool pressedOk() { return getKiller() == m_ok; }

  int getLeft()   const { return m_left->getTextInt(); }
  int getRight()  const { return m_right->getTextInt(); }
  int getTop()    const { return m_top->getTextInt(); }
  int getBottom() const { return m_bottom->getTextInt(); }

protected:
  // SelectBoxDelegate impleentation
  virtual void onChangeRectangle(const gfx::Rect& rect) OVERRIDE
  {
    m_rect = rect;

    m_left->setTextf("%d", -m_rect.x);
    m_top->setTextf("%d", -m_rect.y);
    m_right->setTextf("%d", (m_rect.x + m_rect.w) - current_editor->getSprite()->getWidth());
    m_bottom->setTextf("%d", (m_rect.y + m_rect.h) - current_editor->getSprite()->getHeight());
  }

  void onEntriesChange()
  {
    int left = getLeft();
    int top = getTop();

    m_rect = gfx::Rect(-left, -top,
                       m_editor->getSprite()->getWidth() + left + getRight(),
                       m_editor->getSprite()->getHeight() + top + getBottom());

    static_cast<SelectBoxState*>(m_selectBoxState.get())->setBoxBounds(m_rect);

    // Redraw new rulers position
    m_editor->invalidate();
  }

  virtual void onBroadcastMouseMessage(WidgetsList& targets) OVERRIDE
  {
    Window::onBroadcastMouseMessage(targets);

    // Add the editor as receptor of mouse events too.
    targets.push_back(View::getView(m_editor));
  }

private:
  Editor* m_editor;
  Widget* m_mainBox;
  Entry* m_left;
  Entry* m_right;
  Entry* m_top;
  Entry* m_bottom;
  Widget* m_ok;
  gfx::Rect m_rect;
  EditorStatePtr m_selectBoxState;
};

class CanvasSizeCommand : public Command {
  int m_left, m_right, m_top, m_bottom;

public:
  CanvasSizeCommand();
  Command* clone() const { return new CanvasSizeCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

CanvasSizeCommand::CanvasSizeCommand()
  : Command("CanvasSize",
            "Canvas Size",
            CmdRecordableFlag)
{
  m_left = m_right = m_top = m_bottom = 0;
}

bool CanvasSizeCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void CanvasSizeCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Sprite* sprite(reader.sprite());

  if (context->isUiAvailable()) {
    // load the window widget
    base::UniquePtr<CanvasSizeWindow> window(new CanvasSizeWindow(0, 0, 0, 0));

    window->remapWindow();
    window->centerWindow();

    load_window_pos(window, "CanvasSize");
    window->setVisible(true);
    window->openWindowInForeground();
    save_window_pos(window, "CanvasSize");

    if (!window->pressedOk())
      return;

    m_left   = window->getLeft();
    m_right  = window->getRight();
    m_top    = window->getTop();
    m_bottom = window->getBottom();
  }

  // Resize canvas

  int x1 = -m_left;
  int y1 = -m_top;
  int x2 = sprite->getWidth() + m_right;
  int y2 = sprite->getHeight() + m_bottom;

  if (x2 <= x1) x2 = x1+1;
  if (y2 <= y1) y2 = y1+1;

  {
    ContextWriter writer(reader);
    Document* document = writer.document();
    Sprite* sprite = writer.sprite();
    UndoTransaction undoTransaction(writer.context(), "Canvas Size");
    DocumentApi api = document->getApi();
    int bgcolor = color_utils::color_for_image(context->getSettings()->getBgColor(), sprite->getPixelFormat());
    bgcolor = color_utils::fixup_color_for_background(sprite->getPixelFormat(), bgcolor);

    api.cropSprite(sprite, gfx::Rect(x1, y1, x2-x1, y2-y1), bgcolor);
    undoTransaction.commit();

    document->generateMaskBoundaries();
    update_screen_for_document(document);
  }
}

Command* CommandFactory::createCanvasSizeCommand()
{
  return new CanvasSizeCommand;
}

} // namespace app
