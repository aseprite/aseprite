// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/ui.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/modules/gfx.h"
#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_render.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/preview_editor.h"
#include "app/ui/status_bar.h"
#include "app/util/conversion_to_surface.h"
#include "doc/image.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "gfx/matrix.h"
#include "os/surface.h"
#include "os/system.h"

#if LAF_SKIA
  #include "os/skia/skia_surface.h"
#endif

#include <cmath>
#include <cstring>

#define PREVIEW_TILED           1
#define PREVIEW_FIT_ON_SCREEN   2

namespace app {

using namespace ui;
using namespace doc;
using namespace filters;

class PreviewWindow : public Window {
public:
  PreviewWindow(Context* context, Editor* editor)
    : Window(DesktopWindow)
    , m_context(context)
    , m_editor(editor)
    , m_doc(editor->document())
    , m_sprite(editor->sprite())
    , m_pal(m_sprite->palette(editor->frame()))
    , m_proj(editor->projection())
    , m_index_bg_color(-1) {
    // Do not use DocWriter (do not lock the document) because we
    // will call other sub-commands (e.g. previous frame, next frame,
    // etc.).
    View* view = View::getView(editor);
    DocumentPreferences& docPref = Preferences::instance().document(m_doc);
    m_tiled = (filters::TiledMode)docPref.tiled.mode();

    // Free mouse
    editor->manager()->freeMouse();

    // Clear extras (e.g. pen preview)
    m_doc->setExtraCel(ExtraCelRef(nullptr));

    gfx::Rect vp = view->viewportBounds();
    gfx::Point scroll = view->viewScroll();

    m_oldMousePos = mousePosInDisplay();
    m_pos.x = -scroll.x + vp.x + editor->padding().x;
    m_pos.y = -scroll.y + vp.y + editor->padding().y;

    setFocusStop(true);
    captureMouse();
  }

protected:
  virtual bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {

      case kCloseMessage:
        releaseMouse();
        break;

      case kMouseMoveMessage: {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        gfx::Point mousePos = mouseMsg->position();

        gfx::Rect bounds = this->bounds();
        gfx::Border border;
        if (bounds.w > 64*guiscale()) {
          border.left(32*guiscale());
          border.right(32*guiscale());
        }
        if (bounds.h > 64*guiscale()) {
          border.top(32*guiscale());
          border.bottom(32*guiscale());
        }

        m_delta += mousePos - m_oldMousePos;
        m_oldMousePos = mousePos;

        invalidate();
        break;
      }

      case kMouseUpMessage: {
        closeWindow(this);
        break;
      }

      case kKeyDownMessage: {
        KeyMessage* keyMsg = static_cast<KeyMessage*>(msg);
        Command* command = NULL;
        Params params;
        KeyboardShortcuts::instance()
          ->getCommandFromKeyMessage(msg, &command, &params);

        // Change frame
        if (command != NULL &&
            (command->id() == CommandId::GotoFirstFrame() ||
             command->id() == CommandId::GotoPreviousFrame() ||
             command->id() == CommandId::GotoNextFrame() ||
             command->id() == CommandId::GotoLastFrame())) {
          m_context->executeCommand(command, params);
          m_repaint = true; // Re-render
          invalidate();
        }
#if 0
        // Play the animation
        else if (command != NULL &&
                 std::strcmp(command->short_name(), CommandId::PlayAnimation()) == 0) {
          // TODO
        }
#endif
        // Change background color
        else if (keyMsg->scancode() == kKeyPlusPad ||
                 keyMsg->unicodeChar() == '+') {
          if (m_index_bg_color == -1 ||
            m_index_bg_color < m_pal->size()-1) {
            ++m_index_bg_color;

            invalidate();
          }
        }
        else if (keyMsg->scancode() == kKeyMinusPad ||
                 keyMsg->unicodeChar() == '-') {
          if (m_index_bg_color >= 0) {
            --m_index_bg_color;     // can be -1 which is the checkered background

            invalidate();
          }
        }
        else {
          closeWindow(this);
        }

        return true;
      }

      case kSetCursorMessage:
        ui::set_mouse_cursor(kNoCursor);
        return true;
    }

    return Window::onProcessMessage(msg);
  }

  virtual void onPaint(PaintEvent& ev) override {
    Graphics* g = ev.graphics();
    EditorRender& render = m_editor->renderEngine();
    render.setRefLayersVisiblity(false);
    render.disableOnionskin();
    render.setTransparentBackground();

    // Render sprite and leave the result in 'm_render' variable
    if (m_render == nullptr) {
      m_render = os::instance()->makeRgbaSurface(m_sprite->width(),
                                                 m_sprite->height());

#if LAF_SKIA
      // The SimpleRenderer renders unpremultiplied surfaces when
      // rendering on transparent background (this is the only place
      // where this happens).
      if (render.properties().outputsUnpremultiplied) {
        // We use a little tricky with Skia indicating that the alpha
        // is unpremultiplied
        ((os::SkiaSurface*)m_render.get())
          ->bitmap().setAlphaType(kUnpremul_SkAlphaType);
      }
#endif

      m_repaint = true;
    }

    if (m_repaint) {
      m_repaint = false;

      m_render->clear();
      render.setProjection(render::Projection());
      render.renderSprite(
        m_render.get(), m_sprite, m_editor->frame(),
        gfx::ClipF(0, 0, 0, 0, m_sprite->width(), m_sprite->height()));
    }

    float x, y, w, h, u, v;
    x = m_pos.x + m_delta.x;
    y = m_pos.y + m_delta.y;
    w = m_proj.applyX(m_sprite->width());
    h = m_proj.applyY(m_sprite->height());

    if (int(m_tiled) & int(TiledMode::X_AXIS)) x = SGN(x) * std::fmod(ABS(x), w);
    if (int(m_tiled) & int(TiledMode::Y_AXIS)) y = SGN(y) * std::fmod(ABS(y), h);

    if (m_index_bg_color == -1) {
      render.setProjection(m_proj);
      render.setupBackground(m_doc, IMAGE_RGB);
      render.renderCheckeredBackground(
        g->getInternalSurface(), m_sprite,
        gfx::Clip(g->getInternalDeltaX(),
                  g->getInternalDeltaY(),
                  -m_pos.x, -m_pos.y,
                  2*g->getInternalSurface()->width(),
                  2*g->getInternalSurface()->height()));

      // Invalidate the whole Graphics (as we've just modified its
      // internal os::Surface directly).
      g->invalidate(g->getClipBounds());
    }
    else {
      auto col = m_pal->getEntry(m_index_bg_color);
      g->fillRect(gfx::rgba(doc::rgba_getr(col),
                            doc::rgba_getg(col),
                            doc::rgba_getb(col),
                            doc::rgba_geta(col)),
                  clientBounds());
    }

    const double sx = m_proj.scaleX();
    const double sy = m_proj.scaleY();

    gfx::RectF tiledBounds;
    tiledBounds.w = (2+std::ceil(float(clientBounds().w)/w))*w;
    tiledBounds.h = (2+std::ceil(float(clientBounds().h)/h))*h;
    tiledBounds.x = x-w;
    tiledBounds.y = y-h;
    g->save();

    switch (m_tiled) {
      case TiledMode::NONE:
        g->setMatrix(gfx::Matrix::MakeTrans(x, y));
        g->concat(gfx::Matrix::MakeScale(sx, sy));
        g->drawRgbaSurface(m_render.get(), 0, 0);
        break;
      case TiledMode::X_AXIS:
        for (u=tiledBounds.x; u<tiledBounds.x2(); u+=w) {
          g->setMatrix(gfx::Matrix::MakeTrans(u, y));
          g->concat(gfx::Matrix::MakeScale(sx, sy));
          g->drawRgbaSurface(m_render.get(), 0, 0);
        }
        break;
      case TiledMode::Y_AXIS:
        for (v=tiledBounds.y; v<tiledBounds.y2(); v+=h) {
          g->setMatrix(gfx::Matrix::MakeTrans(x, v));
          g->concat(gfx::Matrix::MakeScale(sx, sy));
          g->drawRgbaSurface(m_render.get(), 0, 0);
        }
        break;
      case TiledMode::BOTH:
        for (v=tiledBounds.y; v<tiledBounds.y2(); v+=h) {
          for (u=tiledBounds.x; u<tiledBounds.x2(); u+=w) {
            g->setMatrix(gfx::Matrix::MakeTrans(u, v));
            g->concat(gfx::Matrix::MakeScale(sx, sy));
            g->drawRgbaSurface(m_render.get(), 0, 0);
          }
        }
        break;
    }
    g->restore();
  }

private:
  Context* m_context;
  Editor* m_editor;
  Doc* m_doc;
  Sprite* m_sprite;
  const Palette* m_pal;
  gfx::Point m_pos;
  gfx::Point m_oldMousePos;
  gfx::Point m_delta;
  render::Projection m_proj;
  int m_index_bg_color;
  os::SurfaceRef m_render;
  bool m_repaint = true;
  filters::TiledMode m_tiled;
};

class FullscreenPreviewCommand : public Command {
public:
  FullscreenPreviewCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

FullscreenPreviewCommand::FullscreenPreviewCommand()
  : Command(CommandId::FullscreenPreview(), CmdUIOnlyFlag)
{
}

bool FullscreenPreviewCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

// Hides the preview editor only if needed, and returns a boolean to
// tell if the preview editor was enabled before calling this function.
static bool hidePreviewEditor(const PreviewWindow& previewWindow)
{
  auto previewEditor = App::instance()->mainWindow()->getPreviewEditor();
  bool isPreviewEnabled = previewEditor->isPreviewEnabled();
  if (isPreviewEnabled) {
    auto pvEditorFrame = previewEditor->window()->display()->nativeWindow()->frame();
    auto pvWindowFrame = previewWindow.window()->display()->nativeWindow()->frame();

    // Disable the preview editor if it is enabled and the full screen preview window's frame
    // rectangle touches the preview editor window's frame rectangle.
    if (pvWindowFrame.intersects(pvEditorFrame)) {
      previewEditor->setPreviewEnabled(false);
    }
  }
  return isPreviewEnabled;
}

static void showPreviewEditor()
{
  App::instance()->mainWindow()->getPreviewEditor()->setPreviewEnabled(true);
}

// Shows the sprite using the complete screen.
void FullscreenPreviewCommand::onExecute(Context* context)
{
  auto editor = Editor::activeEditor();

  // Cancel operation if current editor does not have a sprite
  if (!editor || !editor->sprite())
    return;

  PreviewWindow previewWindow(context, editor);

  bool wasPreviewEnabled = hidePreviewEditor(previewWindow);

  previewWindow.openWindowInForeground();

  // Enable the preview editor if it was enabled before showing the full screen preview.
  if (wasPreviewEnabled) {
    showPreviewEditor();
  }

  // Check that the full screen invalidation code is working
  // correctly. This check is just in case that some regression is
  // introduced in ui::Manager() that doesn't handle correctly the
  // invalidation of the manager when it's fully covered by the closed
  // window (desktop windows, like PreviewWindow, match this case).
  ASSERT(editor->manager()->hasFlags(DIRTY));
}

Command* CommandFactory::createFullscreenPreviewCommand()
{
  return new FullscreenPreviewCommand;
}

} // namespace app
