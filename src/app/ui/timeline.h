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

#ifndef APP_UI_TIMELINE_H_INCLUDED
#define APP_UI_TIMELINE_H_INCLUDED

#include "app/document_observer.h"
#include "base/compiler_specific.h"
#include "raster/frame_number.h"
#include "ui/widget.h"

#include <vector>

namespace raster {
  class Cel;
  class Layer;
  class Sprite;
}

namespace app {
  using namespace raster;

  class Context;
  class Document;
  class Editor;

  class Timeline : public ui::Widget
                 , public DocumentObserver {
  public:
    enum State {
      STATE_STANDBY,
      STATE_SCROLLING,
      STATE_MOVING_SEPARATOR,
      STATE_MOVING_LAYER,
      STATE_MOVING_CEL,
      STATE_MOVING_FRAME,
    };

    Timeline();
    ~Timeline();

    void updateUsingEditor(Editor* editor);

    Sprite* getSprite() { return m_sprite; }
    Layer* getLayer() { return m_layer; }
    FrameNumber getFrame() { return m_frame; }

    void setLayer(Layer* layer);
    void setFrame(FrameNumber frame);

    void centerCurrentCel();
    State getState() const { return m_state; }
    bool isMovingCel() const;

  protected:
    bool onProcessMessage(ui::Message* msg) OVERRIDE;
    void onPreferredSize(ui::PreferredSizeEvent& ev) OVERRIDE;

    // DocumentObserver impl.
    void onAddLayer(DocumentEvent& ev) OVERRIDE;
    void onRemoveLayer(DocumentEvent& ev) OVERRIDE;
    void onAddFrame(DocumentEvent& ev) OVERRIDE;
    void onRemoveFrame(DocumentEvent& ev) OVERRIDE;
    void onTotalFramesChanged(DocumentEvent& ev) OVERRIDE;

  private:
    void setCursor(int x, int y);
    void getDrawableLayers(const gfx::Rect& clip, int* first_layer, int* last_layer);
    void getDrawableFrames(const gfx::Rect& clip, FrameNumber* first_frame, FrameNumber* last_frame);
    void drawHeader(const gfx::Rect& clip);
    void drawHeaderFrame(const gfx::Rect& clip, FrameNumber frame);
    void drawHeaderPart(const gfx::Rect& clip, int x1, int y1, int x2, int y2,
                        bool is_hot, bool is_clk,
                        const char* line1, int align1,
                        const char* line2, int align2);
    void drawSeparator(const gfx::Rect& clip);
    void drawLayer(const gfx::Rect& clip, int layer_index);
    void drawLayerPadding();
    void drawCel(const gfx::Rect& clip, int layer_index, FrameNumber frame, Cel* cel);
    bool drawPart(int part, int layer, FrameNumber frame);
    void regenerateLayers();
    void hotThis(int hot_part, int hot_layer, FrameNumber hotFrame);
    void centerCel(int layer, FrameNumber frame);
    void showCel(int layer, FrameNumber frame);
    void showCurrentCel();
    void cleanClk();
    void setScroll(int x, int y, bool use_refresh_region);
    int getLayerIndex(const Layer* layer);

    Context* m_context;
    Document* m_document;
    Sprite* m_sprite;
    Layer* m_layer;
    FrameNumber m_frame;
    State m_state;
    std::vector<Layer*> m_layers;
    int m_scroll_x;
    int m_scroll_y;
    int m_separator_x;
    int m_separator_w;
    // The 'hot' part is where the mouse is on top of
    int m_hot_part;
    int m_hot_layer;
    FrameNumber m_hot_frame;
    // The 'clk' part is where the mouse's button was pressed (maybe for a drag & drop operation)
    int m_clk_part;
    int m_clk_layer;
    FrameNumber m_clk_frame;
    // Keys
    bool m_space_pressed;
  };

} // namespace app

#endif
