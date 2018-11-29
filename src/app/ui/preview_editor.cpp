// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/preview_editor.h"

#include "app/app.h"
#include "app/doc.h"
#include "app/ini_file.h"
#include "app/loop_tag.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/editor/navigate_state.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "doc/sprite.h"
#include "gfx/rect.h"
#include "ui/base.h"
#include "ui/button.h"
#include "ui/close_event.h"
#include "ui/message.h"
#include "ui/system.h"

#include "doc/frame_tag.h"

namespace app {

using namespace app::skin;
using namespace ui;

class MiniCenterButton : public CheckBox {
public:
  MiniCenterButton() : CheckBox("") {
    setDecorative(true);
    setSelected(true);
    initTheme();
  }

protected:
  void onInitTheme(ui::InitThemeEvent& ev) override {
    CheckBox::onInitTheme(ev);
    setStyle(SkinTheme::instance()->styles.windowCenterButton());
  }

  void onSetDecorativeWidgetBounds() override {
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    Widget* window = parent();
    gfx::Rect rect(0, 0, 0, 0);
    gfx::Size centerSize = this->sizeHint();
    gfx::Size playSize = theme->calcSizeHint(this, theme->styles.windowPlayButton());
    gfx::Size closeSize = theme->calcSizeHint(this, theme->styles.windowCloseButton());

    rect.w = centerSize.w;
    rect.h = centerSize.h;
    rect.offset(window->bounds().x2()
                - theme->styles.windowCloseButton()->margin().width() - closeSize.w
                - theme->styles.windowPlayButton()->margin().width() - playSize.w
                - style()->margin().right() - centerSize.w,
                window->bounds().y + style()->margin().top());

    setBounds(rect);
  }

  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {

      case kSetCursorMessage:
        ui::set_mouse_cursor(kArrowCursor);
        return true;
    }

    return CheckBox::onProcessMessage(msg);
  }
};

class MiniPlayButton : public Button {
public:
  MiniPlayButton() : Button(""), m_isPlaying(false) {
    enableFlags(CTRL_RIGHT_CLICK);
    setDecorative(true);
    initTheme();
  }

  bool isPlaying() const { return m_isPlaying; }

  void setPlaying(bool state) {
    m_isPlaying = state;
    setupIcons();
  }

  obs::signal<void()> Popup;

private:
  void onInitTheme(ui::InitThemeEvent& ev) override {
    Button::onInitTheme(ev);
    setupIcons();
  }

  void onClick(Event& ev) override {
    m_isPlaying = !m_isPlaying;
    setupIcons();

    Button::onClick(ev);
  }

  void onSetDecorativeWidgetBounds() override {
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    Widget* window = parent();
    gfx::Rect rect(0, 0, 0, 0);
    gfx::Size playSize = this->sizeHint();
    gfx::Size closeSize = theme->calcSizeHint(this, theme->styles.windowCloseButton());
    gfx::Border margin(0, 0, 0, 0);

    rect.w = playSize.w;
    rect.h = playSize.h;
    rect.offset(window->bounds().x2()
                - theme->styles.windowCloseButton()->margin().width() - closeSize.w
                - style()->margin().right() - playSize.w,
                window->bounds().y + style()->margin().top());

    setBounds(rect);
  }

  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {

      case kSetCursorMessage:
        ui::set_mouse_cursor(kArrowCursor);
        return true;

      case kMouseUpMessage: {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        if (mouseMsg->right()) {
          if (hasCapture()) {
            releaseMouse();
            Popup();

            setSelected(false);
            return true;
          }
        }
        break;
      }
    }

    return Button::onProcessMessage(msg);
  }

  void setupIcons() {
    SkinTheme* theme = SkinTheme::instance();
    if (m_isPlaying)
      setStyle(theme->styles.windowStopButton());
    else
      setStyle(theme->styles.windowPlayButton());
  }

  bool m_isPlaying;
};

PreviewEditorWindow::PreviewEditorWindow()
  : Window(WithTitleBar, "Preview")
  , m_docView(NULL)
  , m_centerButton(new MiniCenterButton())
  , m_playButton(new MiniPlayButton())
  , m_refFrame(0)
  , m_aniSpeed(1.0)
  , m_relatedEditor(nullptr)
{
  setAutoRemap(false);
  setWantFocus(false);

  m_isEnabled = get_config_bool("MiniEditor", "Enabled", true);

  m_centerButton->Click.connect(base::Bind<void>(&PreviewEditorWindow::onCenterClicked, this));
  m_playButton->Click.connect(base::Bind<void>(&PreviewEditorWindow::onPlayClicked, this));
  m_playButton->Popup.connect(base::Bind<void>(&PreviewEditorWindow::onPopupSpeed, this));

  addChild(m_centerButton);
  addChild(m_playButton);

  initTheme();
}

PreviewEditorWindow::~PreviewEditorWindow()
{
  set_config_bool("MiniEditor", "Enabled", m_isEnabled);
}

void PreviewEditorWindow::setPreviewEnabled(bool state)
{
  m_isEnabled = state;

  updateUsingEditor(current_editor);
}

void PreviewEditorWindow::pressPlayButton()
{
  m_playButton->setSelected(
    !m_playButton->isSelected());
  onPlayClicked();
}

bool PreviewEditorWindow::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      {
        SkinTheme* theme = SkinTheme::instance();

        // Default bounds
        int width = ui::display_w()/4;
        int height = ui::display_h()/4;
        int extra = 2*theme->dimensions.miniScrollbarSize();
        setBounds(
          gfx::Rect(
            ui::display_w() - width - ToolBar::instance()->bounds().w - extra,
            ui::display_h() - height - StatusBar::instance()->bounds().h - extra,
            width, height));

        load_window_pos(this, "MiniEditor", false);
        invalidate();
      }
      break;

    case kCloseMessage:
      save_window_pos(this, "MiniEditor");
      break;

  }

  return Window::onProcessMessage(msg);
}

void PreviewEditorWindow::onInitTheme(ui::InitThemeEvent& ev)
{
  Window::onInitTheme(ev);
  setChildSpacing(0);
}

void PreviewEditorWindow::onClose(ui::CloseEvent& ev)
{
  ButtonBase* closeButton = dynamic_cast<ButtonBase*>(ev.getSource());
  if (closeButton &&
      closeButton->type() == kWindowCloseButtonWidget) {
    // Here we don't use "setPreviewEnabled" to change the state of
    // "m_isEnabled" because we're coming from a close event of the
    // window.
    m_isEnabled = false;

    // Redraw the tool bar because it shows the mini editor enabled
    // state. TODO abstract this event
    ToolBar::instance()->invalidate();

    destroyDocView();
  }
}

void PreviewEditorWindow::onWindowResize()
{
  Window::onWindowResize();

  DocView* view = UIContext::instance()->activeView();
  if (view)
    updateUsingEditor(view->editor());
}

bool PreviewEditorWindow::hasDocument() const
{
  return (m_docView && m_docView->document() != nullptr);
}

DocumentPreferences& PreviewEditorWindow::docPref()
{
  Doc* doc = (m_docView ? m_docView->document(): nullptr);
  return Preferences::instance().document(doc);
}

void PreviewEditorWindow::onCenterClicked()
{
  if (!m_relatedEditor || !hasDocument())
    return;

  bool autoScroll = m_centerButton->isSelected();
  docPref().preview.autoScroll(autoScroll);
  if (autoScroll)
    updateUsingEditor(m_relatedEditor);
}

void PreviewEditorWindow::onPlayClicked()
{
  Editor* miniEditor = (m_docView ? m_docView->editor(): nullptr);
  if (!miniEditor || !miniEditor->document())
    return;

  if (m_playButton->isPlaying()) {
    m_refFrame = miniEditor->frame();
    miniEditor->play(Preferences::instance().preview.playOnce(),
                     Preferences::instance().preview.playAll());
  }
  else {
    miniEditor->stop();
    if (m_relatedEditor)
      miniEditor->setFrame(m_relatedEditor->frame());
  }
}

void PreviewEditorWindow::onPopupSpeed()
{
  Editor* miniEditor = (m_docView ? m_docView->editor(): nullptr);
  if (!miniEditor || !miniEditor->document())
    return;

  auto& pref = Preferences::instance();

  miniEditor->showAnimationSpeedMultiplierPopup(
    pref.preview.playOnce,
    pref.preview.playAll,
    false);
  m_aniSpeed = miniEditor->getAnimationSpeedMultiplier();
}

Editor* PreviewEditorWindow::previewEditor() const
{
  return (m_docView ? m_docView->editor(): nullptr);
}

void PreviewEditorWindow::updateUsingEditor(Editor* editor)
{
  if (!m_isEnabled || !editor) {
    hideWindow();
    m_relatedEditor = nullptr;
    return;
  }

  if (!editor->isActive())
    return;

  m_relatedEditor = editor;

  Doc* document = editor->document();
  Editor* miniEditor = (m_docView ? m_docView->editor(): nullptr);

  if (!isVisible())
    openWindow();

  // Document preferences used to store the preferred zoom/scroll point
  auto& docPref = Preferences::instance().document(document);
  bool autoScroll = docPref.preview.autoScroll();

  // Set the same location as in the given editor.
  if (!miniEditor || miniEditor->document() != document) {
    destroyDocView();

    m_docView = new DocView(document, DocView::Preview, this);
    addChild(m_docView);

    miniEditor = m_docView->editor();
    miniEditor->setZoom(render::Zoom::fromScale(docPref.preview.zoom()));
    miniEditor->setLayer(editor->layer());
    miniEditor->setFrame(editor->frame());
    miniEditor->setState(EditorStatePtr(new NavigateState));
    miniEditor->setAnimationSpeedMultiplier(m_aniSpeed);
    miniEditor->add_observer(this);
    layout();

    if (!autoScroll)
      miniEditor->setEditorScroll(docPref.preview.scroll());
  }

  m_centerButton->setSelected(autoScroll);
  if (autoScroll) {
    gfx::Point centerPoint = editor->getVisibleSpriteBounds().center();
    miniEditor->centerInSpritePoint(centerPoint);

    saveScrollPref();
  }

  if (!m_playButton->isPlaying()) {
    miniEditor->stop();
    miniEditor->setLayer(editor->layer());
    miniEditor->setFrame(editor->frame());
  }
  else {
    if (miniEditor->isPlaying()) {
      doc::FrameTag* tag = editor
        ->getCustomizationDelegate()
        ->getFrameTagProvider()
        ->getFrameTagByFrame(editor->frame(), true);

      doc::FrameTag* playingTag = editor
        ->getCustomizationDelegate()
        ->getFrameTagProvider()
        ->getFrameTagByFrame(m_refFrame, true);

      if (tag == playingTag)
        return;

      miniEditor->stop();
    }

    if (!miniEditor->isPlaying())
      miniEditor->setFrame(m_refFrame = editor->frame());

    miniEditor->play(Preferences::instance().preview.playOnce(),
                     Preferences::instance().preview.playAll());
  }
}

void PreviewEditorWindow::uncheckCenterButton()
{
  if (m_centerButton->isSelected()) {
    m_centerButton->setSelected(false);
    onCenterClicked();
  }
}

void PreviewEditorWindow::onStateChanged(Editor* editor)
{
  // Sync editor playing state with MiniPlayButton state
  if (m_playButton->isPlaying() != editor->isPlaying())
    m_playButton->setPlaying(editor->isPlaying());
}

void PreviewEditorWindow::onScrollChanged(Editor* miniEditor)
{
  if (miniEditor->hasCapture()) {
    saveScrollPref();
    uncheckCenterButton();
  }
}

void PreviewEditorWindow::onZoomChanged(Editor* miniEditor)
{
  saveScrollPref();
}

void PreviewEditorWindow::saveScrollPref()
{
  ASSERT(m_docView);
  if (!m_docView)
    return;

  Editor* miniEditor = m_docView->editor();
  ASSERT(miniEditor);

  docPref().preview.scroll(View::getView(miniEditor)->viewScroll());
  docPref().preview.zoom(miniEditor->zoom().scale());
}

void PreviewEditorWindow::onScrollOtherEditor(Editor* editor)
{
  updateUsingEditor(editor);
}

void PreviewEditorWindow::onDisposeOtherEditor(Editor* editor)
{
  if (m_relatedEditor == editor)
    updateUsingEditor(nullptr);
}

void PreviewEditorWindow::onPreviewOtherEditor(Editor* editor)
{
  updateUsingEditor(editor);
}

void PreviewEditorWindow::hideWindow()
{
  destroyDocView();
  if (isVisible())
    closeWindow(NULL);
}

void PreviewEditorWindow::destroyDocView()
{
  if (m_docView) {
    m_docView->editor()->remove_observer(this);

    delete m_docView;
    m_docView = nullptr;
  }
}

} // namespace app
