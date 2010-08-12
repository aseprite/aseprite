/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#ifndef WIDGETS_STATEBAR_H_INCLUDED
#define WIDGETS_STATEBAR_H_INCLUDED

#include "jinete/jbase.h"
#include "jinete/jwidget.h"

#include "core/color.h"

class Frame;
class StatusBar;
class Tool;

class Progress
{
  friend class StatusBar;

public:
  ~Progress();
  void setPos(float pos);
  float getPos() const;

private:
  Progress();
  Progress(StatusBar* statusbar);
  StatusBar* m_statusbar;
  float m_pos;
};

class StatusBar : public Widget
{
public:
  StatusBar();
  ~StatusBar();

  void clearText();

  bool setStatusText(int msecs, const char *format, ...);
  void showTip(int msecs, const char *format, ...);
  void showColor(int msecs, const char* text, color_t color, int alpha);
  void showTool(int msecs, Tool* tool);

  void showMovePixelsOptions();
  void hideMovePixelsOptions();

  color_t getTransparentColor();

  // Methods to add and remove progress bars
  Progress* addProgress();
  void removeProgress(Progress* progress);

protected:
  bool onProcessMessage(JMessage msg);

private:
  void onCurrentToolChange();
  void updateFromLayer();

  enum State { SHOW_TEXT, SHOW_COLOR, SHOW_TOOL };

  int m_timeout;
  State m_state;

  // Showing a tool
  Tool* m_tool;

  // Showing a color
  color_t m_color;
  int m_alpha;

  // Progress bar
  JList m_progress;

  // Box of main commands
  Widget* m_commandsBox;
  Widget* m_slider;			// Opacity slider
  Widget* m_b_first;			// Go to first frame
  Widget* m_b_prev;			// Go to previous frame
  Widget* m_b_play;			// Play animation
  Widget* m_b_next;			// Go to next frame
  Widget* m_b_last;			// Go to last frame

  // Box with move-pixels commands (when the user drag-and-drop selected pixels using the editor)
  Widget* m_movePixelsBox;
  Widget* m_transparentLabel;
  Widget* m_transparentColor;

  // Tip window
  Frame* m_tipwindow;

  int m_hot_layer;
};

#endif
