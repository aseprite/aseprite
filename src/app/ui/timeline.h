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

#include "app/context_observer.h"
#include "app/document_observer.h"
#include "app/ui/editor/editor_observer.h"
#include "app/ui/skin/style.h"
#include "base/compiler_specific.h"
#include "raster/frame_number.h"
#include "ui/widget.h"

#include <vector>

namespace raster {
  class Cel;
  class Layer;
  class Sprite;
}

namespace ui {
  class Graphics;
}

namespace app {
  using namespace raster;

  class Context;
  class Document;
  class Editor;

  class Timeline : public ui::Widget
                 , public ContextObserver
                 , public DocumentObserver
                 , public EditorObserver {
  public:
    enum State {
      STATE_STANDBY,
      STATE_SCROLLING,
      STATE_SELECTING_LAYERS,
      STATE_SELECTING_FRAMES,
      STATE_SELECTING_CELS,
      STATE_MOVING_SEPARATOR,
      STATE_MOVING_LAYER,
      STATE_MOVING_CEL,
      STATE_MOVING_FRAME,
    };

    struct Range {
      Range() : m_enabled(false) { }

      bool enabled() const { return m_enabled; }
      int layerBegin() const { return MIN(m_layerBegin, m_layerEnd); }
      int layerEnd() const { return MAX(m_layerBegin, m_layerEnd); }
      FrameNumber frameBegin() const { return MIN(m_frameBegin, m_frameEnd); }
      FrameNumber frameEnd() const { return MAX(m_frameBegin, m_frameEnd); }

      bool inRange(int layer) const;
      bool inRange(FrameNumber frame) const;
      bool inRange(int layer, FrameNumber frame) const;

      void startRange(int layer, FrameNumber frame);
      void endRange(int layer, FrameNumber frame);
      void disableRange();

    private:
      bool m_enabled;
      int m_layerBegin;
      int m_layerEnd;
      FrameNumber m_frameBegin;
      FrameNumber m_frameEnd;
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

    Range range() const { return m_range; }

  protected:
    bool onProcessMessage(ui::Message* msg) OVERRIDE;
    void onPreferredSize(ui::PreferredSizeEvent& ev) OVERRIDE;
    void onPaint(ui::PaintEvent& ev) OVERRIDE;

    // DocumentObserver impl.
    void onAddLayer(DocumentEvent& ev) OVERRIDE;
    void onRemoveLayer(DocumentEvent& ev) OVERRIDE;
    void onAddFrame(DocumentEvent& ev) OVERRIDE;
    void onRemoveFrame(DocumentEvent& ev) OVERRIDE;

    // ContextObserver impl.
    void onCommandAfterExecution(Context* context) OVERRIDE;
    void onRemoveDocument(Context* context, Document* document) OVERRIDE;

    // EditorObserver impl.
    void dispose() OVERRIDE { }
    void onStateChanged(Editor* editor) OVERRIDE { }
    void onScrollChanged(Editor* editor) OVERRIDE { }
    void onFrameChanged(Editor* editor) OVERRIDE;
    void onLayerChanged(Editor* editor) OVERRIDE;

  private:
    void detachDocument();
    void setCursor(int x, int y);
    void getDrawableLayers(ui::Graphics* g, int* first_layer, int* last_layer);
    void getDrawableFrames(ui::Graphics* g, FrameNumber* first_frame, FrameNumber* last_frame);
    void drawPart(ui::Graphics* g, const gfx::Rect& bounds,
      const char* text, skin::Style* style,
      bool is_active = false, bool is_hover = false, bool is_clicked = false);
    void drawHeader(ui::Graphics* g);
    void drawHeaderFrame(ui::Graphics* g, FrameNumber frame);
    void drawLayer(ui::Graphics* g, int layer_index);
    void drawCel(ui::Graphics* g, int layer_index, FrameNumber frame, Cel* cel);
    void drawPaddings(ui::Graphics* g);
    bool drawPart(ui::Graphics* g, int part, int layer, FrameNumber frame);
    gfx::Rect getLayerHeadersBounds() const;
    gfx::Rect getFrameHeadersBounds() const;
    gfx::Rect getCelsBounds() const;
    gfx::Rect getPartBounds(int part, int layer = 0, FrameNumber frame = FrameNumber(0)) const;
    void invalidatePart(int part, int layer, FrameNumber frame);
    void regenerateLayers();
    void hotThis(int hot_part, int hot_layer, FrameNumber hotFrame);
    void centerCel(int layer, FrameNumber frame);
    void showCel(int layer, FrameNumber frame);
    void showCurrentCel();
    void cleanClk();
    void setScroll(int x, int y);
    int getLayerIndex(const Layer* layer) const;
    bool isLayerActive(const Layer* layer) const;
    bool isFrameActive(FrameNumber frame) const;

    skin::Style* m_timelineStyle;
    skin::Style* m_timelineBoxStyle;
    skin::Style* m_timelineOpenEyeStyle;
    skin::Style* m_timelineClosedEyeStyle;
    skin::Style* m_timelineOpenPadlockStyle;
    skin::Style* m_timelineClosedPadlockStyle;
    skin::Style* m_timelineLayerStyle;
    skin::Style* m_timelineEmptyFrameStyle;
    skin::Style* m_timelineKeyframeStyle;
    skin::Style* m_timelineFromLeftStyle;
    skin::Style* m_timelineFromRightStyle;
    skin::Style* m_timelineFromBothStyle;
    skin::Style* m_timelineGearStyle;
    skin::Style* m_timelinePaddingStyle;
    skin::Style* m_timelinePaddingTrStyle;
    skin::Style* m_timelinePaddingBlStyle;
    skin::Style* m_timelinePaddingBrStyle;
    skin::Style* m_timelineSelectedCelStyle;
    Context* m_context;
    Editor* m_editor;
    Document* m_document;
    Sprite* m_sprite;
    Layer* m_layer;
    FrameNumber m_frame;
    Range m_range;
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
