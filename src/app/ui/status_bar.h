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

#ifndef APP_UI_STATUS_BAR_H_INCLUDED
#define APP_UI_STATUS_BAR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "base/observers.h"
#include "raster/layer_index.h"
#include "ui/base.h"
#include "ui/link_label.h"
#include "ui/widget.h"

#include <string>
#include <vector>

namespace ui {
  class Box;
  class Button;
  class Entry;
  class Slider;
  class Window;
}

namespace app {
  class StatusBar;

  namespace tools {
    class Tool;
  }

  class Progress {
    friend class StatusBar;

  public:
    ~Progress();
    void setPos(double pos);
    double getPos() const;

  private:
    Progress();
    Progress(StatusBar* statusbar);
    StatusBar* m_statusbar;
    double m_pos;
  };

  class StatusBar : public ui::Widget {
    static StatusBar* m_instance;
  public:
    static StatusBar* instance() { return m_instance; }

    StatusBar();
    ~StatusBar();

    void clearText();

    bool setStatusText(int msecs, const char *format, ...);
    void showTip(int msecs, const char *format, ...);
    void showColor(int msecs, const char* text, const Color& color, int alpha);
    void showTool(int msecs, tools::Tool* tool);

    // Methods to add and remove progress bars
    Progress* addProgress();
    void removeProgress(Progress* progress);

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onPreferredSize(ui::PreferredSizeEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;

  private:
    void onCurrentToolChange();
    void updateFromLayer();
    void updateCurrentFrame();
    void newFrame();
    void updateSubwidgetsVisibility();

    enum State { SHOW_TEXT, SHOW_COLOR, SHOW_TOOL };

    typedef std::vector<Progress*> ProgressList;

    int m_timeout;
    State m_state;

    // Showing a tool
    tools::Tool* m_tool;

    // Showing a color
    Color m_color;
    int m_alpha;

    // Progress bar
    ProgressList m_progress;

    // Box of main commands
    ui::Widget* m_commandsBox;
    ui::Slider* m_slider;             // Opacity slider
    ui::Entry* m_currentFrame;        // Current frame and go to frame entry
    ui::Button* m_newFrame;           // Button to create a new frame
    ui::Button* m_b_first;            // Go to first frame
    ui::Button* m_b_prev;             // Go to previous frame
    ui::Button* m_b_play;             // Play animation
    ui::Button* m_b_next;             // Go to next frame
    ui::Button* m_b_last;             // Go to last frame

    // Box of notifications.
    ui::Widget* m_notificationsBox;
    ui::LinkLabel* m_linkLabel;

    // Tip window
    class CustomizedTipWindow;
    CustomizedTipWindow* m_tipwindow;
  };

} // namespace app

#endif
