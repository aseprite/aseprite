// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/commands/new_params.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/button_set.h"
#include "app/ui/color_bar.h"
#include "app/ui/doc_view.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/select_box_state.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/clamp.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include "canvas_size.xml.h"

namespace app {

using namespace ui;
using namespace app::skin;

// Disable warning about usage of "this" in initializer list.
#ifdef _MSC_VER
#pragma warning(disable:4355)
#endif

struct CanvasSizeParams : public NewParams {
  Param<int> left { this, 0, "left" };
  Param<int> right { this, 0, "right" };
  Param<int> top { this, 0, "top" };
  Param<int> bottom { this, 0, "bottom" };
  Param<bool> trimOutside { this, false, "trimOutside" };
};

#ifdef ENABLE_UI

// Window used to show canvas parameters.
class CanvasSizeWindow : public app::gen::CanvasSize
                       , public SelectBoxDelegate
{
public:
  enum class Dir { NW, N, NE, W, C, E, SW, S, SE };

  CanvasSizeWindow(const CanvasSizeParams& params)
    : m_editor(current_editor)
    , m_rect(0, 0, current_editor->sprite()->width(), current_editor->sprite()->height())
    , m_selectBoxState(
      new SelectBoxState(
        this, m_rect,
        SelectBoxState::Flags(
          int(SelectBoxState::Flags::Rulers) |
          int(SelectBoxState::Flags::DarkOutside)))) {
    setWidth(m_rect.w);
    setHeight(m_rect.h);
    setLeft(0);
    setRight(0);
    setTop(0);
    setBottom(0);

    width() ->Change.connect(base::Bind<void>(&CanvasSizeWindow::onSizeChange, this));
    height()->Change.connect(base::Bind<void>(&CanvasSizeWindow::onSizeChange, this));
    dir()   ->ItemChange.connect(base::Bind<void>(&CanvasSizeWindow::onDirChange, this));;
    left()  ->Change.connect(base::Bind<void>(&CanvasSizeWindow::onBorderChange, this));
    right() ->Change.connect(base::Bind<void>(&CanvasSizeWindow::onBorderChange, this));
    top()   ->Change.connect(base::Bind<void>(&CanvasSizeWindow::onBorderChange, this));
    bottom()->Change.connect(base::Bind<void>(&CanvasSizeWindow::onBorderChange, this));

    m_editor->setState(m_selectBoxState);

    dir()->setSelectedItem((int)Dir::C);
    trim()->setSelected(params.trimOutside());
    updateIcons();
  }

  ~CanvasSizeWindow() {
    m_editor->backToPreviousState();
  }

  bool pressedOk() { return closer() == ok(); }

  int getWidth()  { return width()->textInt(); }
  int getHeight() { return height()->textInt(); }
  int getLeft()   { return left()->textInt(); }
  int getRight()  { return right()->textInt(); }
  int getTop()    { return top()->textInt(); }
  int getBottom() { return bottom()->textInt(); }
  bool getTrimOutside() { return trim()->isSelected(); }

protected:

  // SelectBoxDelegate impleentation
  void onChangeRectangle(const gfx::Rect& rect) override {
    m_rect = rect;

    updateSizeFromRect();
    updateBorderFromRect();
    updateIcons();
  }

  std::string onGetContextBarHelp() override {
    return "Select new canvas size";
  }

  void onSizeChange() {
    updateBorderFromSize();
    updateRectFromBorder();
    updateEditorBoxFromRect();
  }

  void onDirChange() {
    updateIcons();
    updateBorderFromSize();
    updateRectFromBorder();
    updateEditorBoxFromRect();
  }

  void onBorderChange() {
    updateIcons();
    updateRectFromBorder();
    updateSizeFromRect();
    updateEditorBoxFromRect();
  }

  virtual void onBroadcastMouseMessage(WidgetsList& targets) override {
    Window::onBroadcastMouseMessage(targets);

    // Add the editor as receptor of mouse events too.
    targets.push_back(View::getView(m_editor));
  }

private:

  void updateBorderFromSize() {
    int w = getWidth() - m_editor->sprite()->width();
    int h = getHeight() - m_editor->sprite()->height();
    int l, r, t, b;
    l = r = t = b = 0;

    switch ((Dir)dir()->selectedItem()) {
      case Dir::NW:
      case Dir::W:
      case Dir::SW:
        r = w;
        break;
      case Dir::N:
      case Dir::C:
      case Dir::S:
        l = r = w/2;
        if (w & 1)
          r += (w >= 0 ? 1: -1);
        break;
      case Dir::NE:
      case Dir::E:
      case Dir::SE:
        l = w;
        break;
    }

    switch ((Dir)dir()->selectedItem()) {
      case Dir::NW:
      case Dir::N:
      case Dir::NE:
        b = h;
        break;
      case Dir::W:
      case Dir::C:
      case Dir::E:
        b = t = h/2;
        if (h & 1)
          t += (h >= 0 ? 1: -1);
        break;
      case Dir::SW:
      case Dir::S:
      case Dir::SE:
        t = h;
        break;
    }

    setLeft(l);
    setRight(r);
    setTop(t);
    setBottom(b);
  }

  void updateRectFromBorder() {
    int left = getLeft();
    int top = getTop();

    m_rect = gfx::Rect(-left, -top,
      m_editor->sprite()->width() + left + getRight(),
      m_editor->sprite()->height() + top + getBottom());
  }

  void updateSizeFromRect() {
    setWidth(m_rect.w);
    setHeight(m_rect.h);
  }

  void updateBorderFromRect() {
    setLeft(-m_rect.x);
    setTop(-m_rect.y);
    setRight((m_rect.x + m_rect.w) - current_editor->sprite()->width());
    setBottom((m_rect.y + m_rect.h) - current_editor->sprite()->height());
  }

  void updateEditorBoxFromRect() {
    static_cast<SelectBoxState*>(m_selectBoxState.get())->setBoxBounds(m_rect);

    // Redraw new rulers position
    m_editor->invalidate();
  }

  void updateIcons() {
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

    int sel = dir()->selectedItem();

    int c = 0;
    for (int v=0; v<3; ++v) {
      for (int u=0; u<3; ++u) {
        SkinPartPtr icon = theme->parts.canvasEmpty();

        if (c == sel) {
          icon = theme->parts.canvasC();
        }
        else if (u+1 < 3 && (u+1)+3*v == sel) {
          icon = theme->parts.canvasW();
        }
        else if (u-1 >= 0 && (u-1)+3*v == sel) {
          icon = theme->parts.canvasE();
        }
        else if (v+1 < 3 && u+3*(v+1) == sel) {
          icon = theme->parts.canvasN();
        }
        else if (v-1 >= 0 && u+3*(v-1) == sel) {
          icon = theme->parts.canvasS();
        }
        else if (u+1 < 3 && v+1 < 3 && (u+1)+3*(v+1) == sel) {
          icon = theme->parts.canvasNw();
        }
        else if (u-1 >= 0 && v+1 < 3 && (u-1)+3*(v+1) == sel) {
          icon = theme->parts.canvasNe();
        }
        else if (u+1 < 3 && v-1 >= 0 && (u+1)+3*(v-1) == sel) {
          icon = theme->parts.canvasSw();
        }
        else if (u-1 >= 0 && v-1 >= 0 && (u-1)+3*(v-1) == sel) {
          icon = theme->parts.canvasSe();
        }

        dir()->getItem(c)->setIcon(icon);
        ++c;
      }
    }
  }

  void setWidth(int v)  { width()->setTextf("%d", v); }
  void setHeight(int v) { height()->setTextf("%d", v); }
  void setLeft(int v)   { left()->setTextf("%d", v); }
  void setRight(int v)  { right()->setTextf("%d", v); }
  void setTop(int v)    { top()->setTextf("%d", v); }
  void setBottom(int v) { bottom()->setTextf("%d", v); }

  Editor* m_editor;
  gfx::Rect m_rect;
  EditorStatePtr m_selectBoxState;
};

#endif // ENABLE_UI

class CanvasSizeCommand : public CommandWithNewParams<CanvasSizeParams> {
public:
  CanvasSizeCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

CanvasSizeCommand::CanvasSizeCommand()
  : CommandWithNewParams(CommandId::CanvasSize(), CmdRecordableFlag)
{
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
  auto& params = this->params();

#ifdef ENABLE_UI
  if (context->isUIAvailable()) {
    if (!params.trimOutside.isSet()) {
      params.trimOutside(Preferences::instance().canvasSize.trimOutside());
    }

    // load the window widget
    std::unique_ptr<CanvasSizeWindow> window(new CanvasSizeWindow(params));

    window->remapWindow();

    // Find best position for the window on the editor
    if (DocView* docView = static_cast<UIContext*>(context)->activeView()) {
      window->positionWindow(
        docView->bounds().x2() - window->bounds().w,
        docView->bounds().y);
    }
    else
      window->centerWindow();

    load_window_pos(window.get(), "CanvasSize");
    window->setVisible(true);
    window->openWindowInForeground();
    save_window_pos(window.get(), "CanvasSize");

    if (!window->pressedOk())
      return;

    params.left(window->getLeft());
    params.right(window->getRight());
    params.top(window->getTop());
    params.bottom(window->getBottom());
    params.trimOutside(window->getTrimOutside());

    Preferences::instance().canvasSize.trimOutside(params.trimOutside());
  }
#endif

  // Resize canvas

  int x1 = -params.left();
  int y1 = -params.top();
  int x2 = sprite->width() + params.right();
  int y2 = sprite->height() + params.bottom();

  if (x2 <= x1) x2 = x1+1;
  if (y2 <= y1) y2 = y1+1;

  {
    ContextWriter writer(reader);
    Doc* doc = writer.document();
    Sprite* sprite = writer.sprite();
    Tx tx(writer.context(), "Canvas Size");
    DocApi api = doc->getApi(tx);

    api.cropSprite(sprite,
                   gfx::Rect(x1, y1,
                             base::clamp(x2-x1, 1, DOC_SPRITE_MAX_WIDTH),
                             base::clamp(y2-y1, 1, DOC_SPRITE_MAX_HEIGHT)),
                   params.trimOutside());
    tx.commit();

#ifdef ENABLE_UI
    if (context->isUIAvailable())
      update_screen_for_document(doc);
#endif
  }
}

Command* CommandFactory::createCanvasSizeCommand()
{
  return new CanvasSizeCommand;
}

} // namespace app
