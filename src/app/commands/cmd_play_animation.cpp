// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
#include "app/ui/preview_editor.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/sprite.h"

namespace app {

// TODO merge this with PreviewEditor logic and create a new Editor state

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

    m_curFrameTick = ui::clock();
    m_pingPongForward = true;
    m_nextFrameTime = editor->sprite()->frameDuration(editor->frame());

    m_playTimer.Tick.connect(&PlayAniWindow::onPlaybackTick, this);
    m_playTimer.start();
  }

protected:
  void onPlaybackTick() {
    if (m_nextFrameTime >= 0) {
      m_nextFrameTime -= (ui::clock() - m_curFrameTick);

      while (m_nextFrameTime <= 0) {
        frame_t frame = calculate_next_frame(
          m_editor->sprite(),
          m_editor->frame(),
          m_docSettings,
          m_pingPongForward);

        m_editor->setFrame(frame);
        m_nextFrameTime += m_editor->sprite()->frameDuration(frame);
        invalidate();
      }

      m_curFrameTick = ui::clock();
    }
  }

  virtual bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {

      case kOpenMessage:
        ui::set_mouse_cursor(kNoCursor);
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
        closeWindow(this);
        return true;
      }

      case kSetCursorMessage:
        ui::set_mouse_cursor(kNoCursor);
        return true;
    }

    return Window::onProcessMessage(msg);
  }

  virtual void onPaint(PaintEvent& ev) override {
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
  frame_t m_oldFrame;
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
  Command* clone() const override { return new PlayAnimationCommand(*this); }

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
  PreviewEditorWindow* preview =
    App::instance()->getMainWindow()->getPreviewEditor();
  bool enabled = (preview ? preview->isPreviewEnabled(): false);
  if (enabled)
    preview->setPreviewEnabled(false);

  PlayAniWindow window(context, current_editor);
  window.openWindowInForeground();

  if (enabled)
    preview->setPreviewEnabled(enabled);
}

Command* CommandFactory::createPlayAnimationCommand()
{
  return new PlayAnimationCommand;
}

} // namespace app
