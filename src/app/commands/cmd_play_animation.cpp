/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#include "ui/ui.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/handle_anidir.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/ui/editor/editor.h"
#include "app/ui/main_window.h"
#include "app/ui/mini_editor.h"
#include "raster/conversion_alleg.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/sprite.h"

namespace app {

// TODO merge this with MiniEditor logic and create a new Editor state

using namespace ui;

class PlayAniWindow : public Window {
public:
  PlayAniWindow(Context* context, Editor* editor)
    : Window(DesktopWindow)
    , m_editor(editor)
    , m_oldFrame(editor->frame())
    , m_oldFlags(m_editor->editorFlags())
    , m_doc(editor->document())
    , m_docSettings(context->settings()->getDocumentSettings(m_doc))
    , m_oldOnionskinState(m_docSettings->getUseOnionskin())
    , m_playTimer(10)
  {
    m_editor->setEditorFlags(Editor::kNoneFlag);

    // Desactivate the onionskin
    m_docSettings->setUseOnionskin(false);

    // Clear extras (e.g. pen preview)
    m_doc->destroyExtraCel();

    setFocusStop(true);         // To receive keyboard messages

    m_curFrameTick = ji_clock;
    m_pingPongForward = true;
    m_nextFrameTime = editor->sprite()->getFrameDuration(editor->frame());

    m_playTimer.Tick.connect(&PlayAniWindow::onPlaybackTick, this);
    m_playTimer.start();
  }

protected:
  void onPlaybackTick() {
    if (m_nextFrameTime >= 0) {
      m_nextFrameTime -= (ji_clock - m_curFrameTick);

      while (m_nextFrameTime <= 0) {
        FrameNumber frame = calculate_next_frame(
          m_editor->sprite(),
          m_editor->frame(),
          m_docSettings,
          m_pingPongForward);

        m_editor->setFrame(frame);
        m_nextFrameTime += m_editor->sprite()->getFrameDuration(frame);
        invalidate();
      }

      m_curFrameTick = ji_clock;
    }
  }

  virtual bool onProcessMessage(Message* msg) OVERRIDE {
    switch (msg->type()) {

      case kOpenMessage:
        jmouse_set_cursor(kNoCursor);
        break;

      case kCloseMessage:
        // Restore onionskin flag
        m_docSettings->setUseOnionskin(m_oldOnionskinState);

        // Restore editor
        m_editor->setFrame(m_oldFrame);
        m_editor->setEditorFlags(m_oldFlags);
        break;

      case kMouseUpMessage: {
        closeWindow(this);
        break;
      }

      case kKeyDownMessage: {
        KeyMessage* keyMsg = static_cast<KeyMessage*>(msg);

        closeWindow(this);

        return true;
      }

      case kSetCursorMessage:
        jmouse_set_cursor(kNoCursor);
        return true;
    }

    return Window::onProcessMessage(msg);
  }

  virtual void onPaint(PaintEvent& ev) OVERRIDE {
    Graphics* g = ev.getGraphics();
    g->fillRect(gfx::rgba(0, 0, 0), getClientBounds());

    Graphics subG(g->getInternalSurface(),
      m_editor->getBounds().x + g->getInternalDeltaY(),
      m_editor->getBounds().y + g->getInternalDeltaY());

    m_editor->drawSpriteUnclippedRect(&subG,
      gfx::Rect(0, 0,
        m_editor->sprite()->width(),
        m_editor->sprite()->height()));
  }

private:
  Editor* m_editor;
  FrameNumber m_oldFrame;
  Editor::EditorFlags m_oldFlags;
  Document* m_doc;
  IDocumentSettings* m_docSettings;
  bool m_oldOnionskinState;
  bool m_pingPongForward;

  int m_nextFrameTime;
  int m_curFrameTick;
  ui::Timer m_playTimer;
};

class PlayAnimationCommand : public Command {
public:
  PlayAnimationCommand();
  Command* clone() const OVERRIDE { return new PlayAnimationCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

PlayAnimationCommand::PlayAnimationCommand()
  : Command("PlayAnimation",
            "Play Animation",
            CmdUIOnlyFlag)
{
}

bool PlayAnimationCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void PlayAnimationCommand::onExecute(Context* context)
{
  // Do not play one-frame images
  {
    ContextReader writer(context);
    Sprite* sprite(writer.sprite());
    if (!sprite || sprite->totalFrames() < 2)
      return;
  }

  // Hide mini editor
  MiniEditorWindow* miniEditor = App::instance()->getMainWindow()->getMiniEditor();
  bool enabled = (miniEditor ? miniEditor->isMiniEditorEnabled(): false);
  if (enabled)
    miniEditor->setMiniEditorEnabled(false);

  PlayAniWindow window(context, current_editor);
  window.openWindowInForeground();

  if (enabled)
    miniEditor->setMiniEditorEnabled(enabled);
}

Command* CommandFactory::createPlayAnimationCommand()
{
  return new PlayAnimationCommand;
}

} // namespace app
