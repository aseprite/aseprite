/* Aseprite
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

#include "app/ui/mini_editor.h"

#include "app/document.h"
#include "app/handle_anidir.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/skin/skin_button.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui/toolbar.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "gfx/rect.h"
#include "doc/sprite.h"
#include "ui/base.h"
#include "ui/button.h"
#include "ui/close_event.h"
#include "ui/message.h"
#include "ui/system.h"

namespace app {

using namespace app::skin;
using namespace ui;

class MiniPlayButton : public SkinButton<Button> {
public:
  MiniPlayButton()
    : SkinButton<Button>(PART_WINDOW_PLAY_BUTTON_NORMAL,
                         PART_WINDOW_PLAY_BUTTON_HOT,
                         PART_WINDOW_PLAY_BUTTON_SELECTED)
    , m_isPlaying(false)
  {
    setup_bevels(this, 0, 0, 0, 0);
    setDecorative(true);
  }

  bool isPlaying() { return m_isPlaying; }

protected:
  void onClick(Event& ev) override
  {
    m_isPlaying = !m_isPlaying;
    if (m_isPlaying)
      setParts(PART_WINDOW_STOP_BUTTON_NORMAL,
               PART_WINDOW_STOP_BUTTON_HOT,
               PART_WINDOW_STOP_BUTTON_SELECTED);
    else
      setParts(PART_WINDOW_PLAY_BUTTON_NORMAL,
               PART_WINDOW_PLAY_BUTTON_HOT,
               PART_WINDOW_PLAY_BUTTON_SELECTED);

    SkinButton<Button>::onClick(ev);
  }

  void onSetDecorativeWidgetBounds() override
  {
    SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
    Widget* window = getParent();
    gfx::Rect rect(0, 0, 0, 0);
    gfx::Size playSize = theme->get_part_size(PART_WINDOW_PLAY_BUTTON_NORMAL);
    gfx::Size closeSize = theme->get_part_size(PART_WINDOW_CLOSE_BUTTON_NORMAL);

    rect.w = playSize.w;
    rect.h = playSize.h;

    rect.offset(window->getBounds().x2() - 3*jguiscale()
                - playSize.w - 1*jguiscale() - closeSize.w,
                window->getBounds().y + 3*jguiscale());

    setBounds(rect);
  }

  bool onProcessMessage(Message* msg) override
  {
    switch (msg->type()) {

      case kSetCursorMessage:
        jmouse_set_cursor(kArrowCursor);
        return true;
    }

    return SkinButton<Button>::onProcessMessage(msg);
  }

private:
  bool m_isPlaying;
};

MiniEditorWindow::MiniEditorWindow()
  : Window(WithTitleBar, "Mini-Editor")
  , m_docView(NULL)
  , m_playButton(new MiniPlayButton())
  , m_playTimer(10)
  , m_pingPongForward(true)
{
  child_spacing = 0;
  setAutoRemap(false);
  setWantFocus(false);

  m_isEnabled = get_config_bool("MiniEditor", "Enabled", true);

  m_playButton->Click.connect(Bind<void>(&MiniEditorWindow::onPlayClicked, this));
  addChild(m_playButton);

  m_playTimer.Tick.connect(&MiniEditorWindow::onPlaybackTick, this);
}

MiniEditorWindow::~MiniEditorWindow()
{
  set_config_bool("MiniEditor", "Enabled", m_isEnabled);
}

void MiniEditorWindow::setMiniEditorEnabled(bool state)
{
  m_isEnabled = state;

  updateUsingEditor(current_editor);
}

bool MiniEditorWindow::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      {
        // Default bounds
        int width = ui::display_w()/4;
        int height = ui::display_h()/4;
        int extra = 2*kEditorViewScrollbarWidth*jguiscale();
        setBounds(
          gfx::Rect(
            ui::display_w() - width - ToolBar::instance()->getBounds().w - extra,
            ui::display_h() - height - StatusBar::instance()->getBounds().h - extra,
            width, height));

        load_window_pos(this, "MiniEditor");
        invalidate();
      }
      break;

    case kCloseMessage:
      save_window_pos(this, "MiniEditor");
      break;

  }

  return Window::onProcessMessage(msg);
}

void MiniEditorWindow::onClose(ui::CloseEvent& ev)
{
  Button* closeButton = dynamic_cast<Button*>(ev.getSource());
  if (closeButton != NULL &&
      closeButton->getId() == SkinTheme::kThemeCloseButtonId) {
    // Here we don't use "setMiniEditorEnabled" to change the state of
    // "m_isEnabled" because we're coming from a close event of the
    // window.
    m_isEnabled = false;

    // Redraw the tool bar because it shows the mini editor enabled
    // state. TODO abstract this event
    ToolBar::instance()->invalidate();

    delete m_docView;
    m_docView = NULL;
  }
}

void MiniEditorWindow::onWindowResize()
{
  Window::onWindowResize();

  DocumentView* view = UIContext::instance()->activeView();
  if (view)
    updateUsingEditor(view->getEditor());
}

void MiniEditorWindow::onPlayClicked()
{
  if (m_playButton->isPlaying()) {
    Editor* miniEditor = (m_docView ? m_docView->getEditor(): NULL);
    if (miniEditor && miniEditor->document() != NULL)
      m_nextFrameTime = miniEditor->sprite()->getFrameDuration(miniEditor->frame());
    else
      m_nextFrameTime = -1;

    m_curFrameTick = ui::clock();
    m_pingPongForward = true;

    m_playTimer.start();
  }
  else {
    m_playTimer.stop();
  }
}

void MiniEditorWindow::updateUsingEditor(Editor* editor)
{
  if (!m_isEnabled || !editor) {
    hideWindow();
    return;
  }

  Document* document = editor->document();
  Editor* miniEditor = (m_docView ? m_docView->getEditor(): NULL);

  if (!isVisible())
    openWindow();

  gfx::Rect visibleBounds = editor->getVisibleSpriteBounds();
  gfx::Point pt = visibleBounds.getCenter();

  // Set the same location as in the given editor.
  if (!miniEditor || miniEditor->document() != document) {
    delete m_docView;
    m_docView = new DocumentView(document, DocumentView::Mini); // MiniEditorDocumentView(document, this);
    addChild(m_docView);

    miniEditor = m_docView->getEditor();
    miniEditor->setZoom(0);
    miniEditor->setState(EditorStatePtr(new EditorState));
    layout();
  }

  miniEditor->centerInSpritePoint(pt.x, pt.y);
  miniEditor->setLayer(editor->layer());
  miniEditor->setFrame(editor->frame());
}

void MiniEditorWindow::hideWindow()
{
  delete m_docView;
  m_docView = NULL;

  if (isVisible())
    closeWindow(NULL);
}

void MiniEditorWindow::onPlaybackTick()
{
  Editor* miniEditor = (m_docView ? m_docView->getEditor(): NULL);
  if (!miniEditor)
    return;

  Document* document = miniEditor->document();
  Sprite* sprite = miniEditor->sprite();
  if (!document || !sprite)
    return;

  ISettings* settings = UIContext::instance()->settings();
  IDocumentSettings* docSettings = settings->getDocumentSettings(document);
  if (!docSettings)
    return;

  if (m_nextFrameTime >= 0) {
    m_nextFrameTime -= (ui::clock() - m_curFrameTick);

    while (m_nextFrameTime <= 0) {
      FrameNumber frame = calculate_next_frame(
        sprite,
        miniEditor->frame(),
        docSettings,
        m_pingPongForward);

      miniEditor->setFrame(frame);

      m_nextFrameTime += miniEditor->sprite()->getFrameDuration(frame);
    }

    m_curFrameTick = ui::clock();
  }

  invalidate();
}

} // namespace app
