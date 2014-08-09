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

#include "ui/ui.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "app/util/render.h"
#include "raster/conversion_she.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/primitives.h"
#include "raster/sprite.h"

#define PREVIEW_TILED           1
#define PREVIEW_FIT_ON_SCREEN   2

namespace app {

using namespace ui;
using namespace raster;
using namespace filters;

class PreviewWindow : public Window {
public:
  PreviewWindow(Context* context, Editor* editor)
    : Window(DesktopWindow)
    , m_context(context)
    , m_editor(editor)
    , m_doc(editor->document())
    , m_sprite(editor->sprite())
    , m_pal(m_sprite->getPalette(editor->frame()))
    , m_index_bg_color(-1)
    , m_doublebuf(Image::create(IMAGE_RGB, JI_SCREEN_W, JI_SCREEN_H)) {
    // Do not use DocumentWriter (do not lock the document) because we
    // will call other sub-commands (e.g. previous frame, next frame,
    // etc.).
    View* view = View::getView(editor);
    IDocumentSettings* docSettings = context->settings()->getDocumentSettings(m_doc);
    m_tiled = docSettings->getTiledMode();

    // Free mouse
    editor->getManager()->freeMouse();

    // Clear extras (e.g. pen preview)
    m_doc->destroyExtraCel();

    gfx::Rect vp = view->getViewportBounds();
    gfx::Point scroll = view->getViewScroll();

    m_first_mouse_pos = m_old_pos = ui::get_mouse_position();
    ui::set_mouse_position(gfx::Point(JI_SCREEN_W/2, JI_SCREEN_H/2));

    m_pos.x = -scroll.x + vp.x + editor->offsetX();
    m_pos.y = -scroll.y + vp.y + editor->offsetY();
    m_zoom = editor->zoom();

    ui::set_mouse_position(m_first_mouse_pos);

    setFocusStop(true);         // To receive keyboard messages
  }

protected:
  virtual bool onProcessMessage(Message* msg) OVERRIDE {
    switch (msg->type()) {

      case kCloseMessage:
        ui::set_mouse_position(m_first_mouse_pos);
        break;

      case kMouseMoveMessage: {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        gfx::Point mousePos = mouseMsg->position();

        gfx::Rect bounds = getBounds();
        gfx::Border border;
        if (bounds.w > 64*jguiscale()) {
          border.left(32*jguiscale());
          border.right(32*jguiscale());
        }
        if (bounds.h > 64*jguiscale()) {
          border.top(32*jguiscale());
          border.bottom(32*jguiscale());
        }

        m_delta += mousePos - m_old_pos;
        mousePos = ui::control_infinite_scroll(this, bounds.shrink(border), mousePos);
        m_old_pos = mousePos;

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
        get_command_from_key_message(msg, &command, NULL);

        // Change frame
        if (command != NULL &&
            (strcmp(command->short_name(), CommandId::GotoFirstFrame) == 0 ||
             strcmp(command->short_name(), CommandId::GotoPreviousFrame) == 0 ||
             strcmp(command->short_name(), CommandId::GotoNextFrame) == 0 ||
             strcmp(command->short_name(), CommandId::GotoLastFrame) == 0)) {
          m_context->executeCommand(command);
          invalidate();
          m_render.reset(NULL); // Re-render
        }
#if 0
        // Play the animation
        else if (command != NULL &&
                 strcmp(command->short_name(), CommandId::PlayAnimation) == 0) {
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
            --m_index_bg_color;     // can be -1 which is the checked background

            invalidate();
          }
        }
        else {
          closeWindow(this);
        }

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

    // Render sprite and leave the result in 'render' variable
    if (m_render == NULL) {
      RenderEngine renderEngine(
        m_doc, m_sprite,
        m_editor->layer(),
        m_editor->frame());

      m_render.reset(
        renderEngine.renderSprite(
          0, 0, m_sprite->width(), m_sprite->height(),
          m_editor->frame(), 0, false, false));
    }

    int x, y, w, h, u, v;
    x = m_pos.x + ((m_delta.x >> m_zoom) << m_zoom);
    y = m_pos.y + ((m_delta.y >> m_zoom) << m_zoom);
    w = (m_sprite->width()<<m_zoom);
    h = (m_sprite->height()<<m_zoom);

    if (m_tiled & TILED_X_AXIS) x = SGN(x) * (ABS(x)%w);
    if (m_tiled & TILED_Y_AXIS) y = SGN(y) * (ABS(y)%h);

    if (m_index_bg_color == -1)
      RenderEngine::renderCheckedBackground(m_doublebuf, -m_pos.x, -m_pos.y, m_zoom);
    else
      raster::clear_image(m_doublebuf, m_pal->getEntry(m_index_bg_color));

    switch (m_tiled) {
      case TILED_NONE:
        RenderEngine::renderImage(m_doublebuf, m_render, m_pal, x, y, m_zoom);
        break;
      case TILED_X_AXIS:
        for (u=x-w; u<JI_SCREEN_W+w; u+=w)
          RenderEngine::renderImage(m_doublebuf, m_render, m_pal, u, y, m_zoom);
        break;
      case TILED_Y_AXIS:
        for (v=y-h; v<JI_SCREEN_H+h; v+=h)
          RenderEngine::renderImage(m_doublebuf, m_render, m_pal, x, v, m_zoom);
        break;
      case TILED_BOTH:
        for (v=y-h; v<JI_SCREEN_H+h; v+=h)
          for (u=x-w; u<JI_SCREEN_W+w; u+=w)
            RenderEngine::renderImage(m_doublebuf, m_render, m_pal, u, v, m_zoom);
        break;
    }

    raster::convert_image_to_surface(m_doublebuf, g->getInternalSurface(), 0, 0, m_pal);
  }

private:
  Context* m_context;
  Editor* m_editor;
  Document* m_doc;
  Sprite* m_sprite;
  const Palette* m_pal;
  gfx::Point m_pos;
  gfx::Point m_old_pos;
  gfx::Point m_first_mouse_pos;
  gfx::Point m_delta;
  int m_zoom;
  int m_index_bg_color;
  base::UniquePtr<Image> m_render;
  base::UniquePtr<Image> m_doublebuf;
  filters::TiledMode m_tiled;
};

class PreviewCommand : public Command {
public:
  PreviewCommand();
  Command* clone() const OVERRIDE { return new PreviewCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

PreviewCommand::PreviewCommand()
  : Command("Preview",
            "Preview",
            CmdUIOnlyFlag)
{
}

bool PreviewCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

// Shows the sprite using the complete screen.
void PreviewCommand::onExecute(Context* context)
{
  Editor* editor = current_editor;

  // Cancel operation if current editor does not have a sprite
  if (!editor || !editor->sprite())
    return;

  PreviewWindow window(context, editor);
  window.openWindowInForeground();
}

Command* CommandFactory::createPreviewCommand()
{
  return new PreviewCommand;
}

} // namespace app
