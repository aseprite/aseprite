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

  void setStatusText(int msecs, const char *format, ...);
  void showTip(int msecs, const char *format, ...);
  void showColor(int msecs, int imgtype, color_t color);
  
  Progress* addProgress();
  void removeProgress(Progress* progress);

protected:
  virtual bool msg_proc(JMessage msg);

private:
  void onCurrentToolChange();
  void updateFromLayer();

  int m_timeout;

  // Progress bar
  JList m_progress;

  // Box of main commands
  Widget* m_commands_box;
  Widget* m_slider;			// Opacity slider
  Widget* m_b_first;			// Go to first frame
  Widget* m_b_prev;			// Go to previous frame
  Widget* m_b_play;			// Play animation
  Widget* m_b_next;			// Go to next frame
  Widget* m_b_last;			// Go to last frame

  // Tip window
  Frame* m_tipwindow;

  int m_hot_layer;
};

#endif
