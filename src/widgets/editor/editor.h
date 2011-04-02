/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#ifndef WIDGETS_EDITOR_H_INCLUDED
#define WIDGETS_EDITOR_H_INCLUDED

#include "app/color.h"
#include "base/compiler_specific.h"
#include "base/signal.h"
#include "document.h"
#include "gui/base.h"
#include "gui/widget.h"
#include "widgets/editor/editor_listeners.h"

#define MIN_ZOOM 0
#define MAX_ZOOM 5

class Context;
class PixelsMovement;
class Sprite;
class View;

namespace tools {
  class Tool;
  class ToolLoop;
  class ToolLoopManager;
}

class Editor : public Widget
{
public:
  // in editor.c

  Editor();
  ~Editor();

  Document* getDocument() { return m_document; }
  void setDocument(Document* document);

  Sprite* getSprite() { return m_sprite; }

  int getZoom() const { return m_zoom; }
  int getOffsetX() const { return m_offset_x; }
  int getOffsetY() const { return m_offset_y; }
  int getCursorThick() { return m_cursor_thick; }

  void setZoom(int zoom) { m_zoom = zoom; }
  void setOffsetX(int x) { m_offset_x = x; }
  void setOffsetY(int y) { m_offset_y = y; }

  void setEditorScroll(int x, int y, int use_refresh_region);

  // Updates the Editor's view.
  void updateEditor();

  // Draws the specified portion of sprite in the editor.
  // Warning: You should setup the clip of the ji_screen before
  // calling this routine.
  void drawSprite(int x1, int y1, int x2, int y2);

  // Draws the sprite taking care of the whole clipping region.
  // For each rectangle calls Editor::drawSprite.
  void drawSpriteSafe(int x1, int y1, int x2, int y2);

  void drawMask();
  void drawMaskSafe();

  void flashCurrentLayer();
  void setMaskColorForPixelsMovement(const Color& color);

  void screenToEditor(int xin, int yin, int *xout, int *yout);
  void editorToScreen(int xin, int yin, int *xout, int *yout);

  void showDrawingCursor();
  void hideDrawingCursor();

  void addListener(EditorListener* listener);
  void removeListener(EditorListener* listener);

  void editor_update_statusbar_for_standby();

  // in cursor.c

  static int get_raw_cursor_color();
  static bool is_cursor_mask();
  static Color get_cursor_color();
  static void set_cursor_color(const Color& color);

  static void editor_cursor_init();
  static void editor_cursor_exit();

private:

  void editor_update_statusbar_for_pixel_movement();
  void editor_update_quicktool();

  void editor_draw_cursor(int x, int y, bool refresh = true);
  void editor_move_cursor(int x, int y, bool refresh = true);
  void editor_clean_cursor(bool refresh = true);
  bool editor_cursor_is_subpixel();

  // keys.c

  bool editor_keys_toset_zoom(int scancode);

public:

  // click.c

  enum {
    MODE_CLICKANDRELEASE,
    MODE_CLICKANDCLICK,
  };

  void editor_click_start(int mode, int *x, int *y, int *b);
  void editor_click_continue(int mode, int *x, int *y);
  void editor_click_done();
  int editor_click(int *x, int *y, int *update,
		   void (*scroll_callback) (int before_change));
  int editor_click_cancel();

protected:
  bool onProcessMessage(Message* msg) OVERRIDE;
  void onCurrentToolChange();

private:
  void drawGrid(const gfx::Rect& gridBounds, const Color& color);

  void controlInfiniteScroll(Message* msg);
  void dropPixels();

  tools::Tool* getCurrentEditorTool();

  void editor_request_size(int *w, int *h);
  void editor_setcursor(int x, int y);
  void editor_update_candraw();
  void setZoomAndCenterInMouse(int zoom, int mouse_x, int mouse_y);

  tools::ToolLoop* createToolLoopImpl(Context* context, Message* msg);

  void for_each_pixel_of_pen(int screen_x, int screen_y,
			     int sprite_x, int sprite_y, int color,
			     void (*pixel)(BITMAP *bmp, int x, int y, int color));

  // editor states
  enum State {
    EDITOR_STATE_STANDBY,
    EDITOR_STATE_SCROLLING,
    EDITOR_STATE_DRAWING,
  };

  // Main properties
  State m_state;		// Editor main state
  Document* m_document;		// Active document in the editor
  Sprite* m_sprite;		// Active sprite in the editor
  int m_zoom;			// Zoom in the editor

  // Drawing cursor
  int m_cursor_thick;
  int m_cursor_screen_x; /* position in the screen (view) */
  int m_cursor_screen_y;
  int m_cursor_editor_x; /* position in the editor (model) */
  int m_cursor_editor_y;
  bool m_cursor_candraw : 1;

  // True if the cursor is inside the mask/selection
  bool m_insideSelection : 1;

  // Current selected quicktool (this genererally should be NULL if
  // the user is not pressing any keyboard key).
  tools::Tool* m_quicktool;

  /* offset for the sprite */
  int m_offset_x;
  int m_offset_y;

  /* marching ants stuff */
  int m_mask_timer_id;
  int m_offset_count;

  /* region that must be updated */
  JRegion m_refresh_region;

  // Tool-loop manager
  tools::ToolLoopManager* m_toolLoopManager;

  // Helper member to move selection. If this member is NULL it means the
  // user is not moving pixels.
  PixelsMovement* m_pixelsMovement;

  // This slot is used to disconnect the Editor from CurrentToolChange
  // signal (because the editor can be destroyed and the application
  // still continue running and generating CurrentToolChange
  // signals).
  Slot0<void>* m_currentToolChangeSlot;

  EditorListeners m_listeners;

};

View* editor_view_new();
int editor_type();

#endif
