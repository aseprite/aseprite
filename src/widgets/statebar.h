/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2012  David Capello
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

#include <vector>
#include <string>

#include "app/color.h"
#include "base/compiler_specific.h"
#include "gui/base.h"
#include "gui/link_label.h"
#include "gui/widget.h"
#include "listeners.h"

class Box;
class Button;
class ColorButton;
class Frame;
class Slider;
class StatusBar;

namespace tools { class Tool; }

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

class StatusBarListener
{
public:
  virtual ~StatusBarListener() { }
  virtual void dispose() = 0;
  virtual void onChangeTransparentColor(const Color& color) = 0;
};

typedef Listeners<StatusBarListener> StatusBarListeners;

class StatusBar : public Widget
{
public:
  StatusBar();
  ~StatusBar();

  void addListener(StatusBarListener* listener);
  void removeListener(StatusBarListener* listener);

  void clearText();

  bool setStatusText(int msecs, const char *format, ...);
  void showTip(int msecs, const char *format, ...);
  void showColor(int msecs, const char* text, const Color& color, int alpha);
  void showTool(int msecs, tools::Tool* tool);

  void showMovePixelsOptions();
  void hideMovePixelsOptions();

  Color getTransparentColor();

  // Methods to add and remove progress bars
  Progress* addProgress();
  void removeProgress(Progress* progress);

  // Method to show notifications (each notification can contain a link).
  void showNotification(const char* text, const char* link);

protected:
  bool onProcessMessage(Message* msg) OVERRIDE;

private:
  void onCurrentToolChange();
  void onTransparentColorChange();
  void updateFromLayer();

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
  Widget* m_commandsBox;
  Slider* m_slider;                     // Opacity slider
  Button* m_b_first;                    // Go to first frame
  Button* m_b_prev;                     // Go to previous frame
  Button* m_b_play;                     // Play animation
  Button* m_b_next;                     // Go to next frame
  Button* m_b_last;                     // Go to last frame

  // Box of notifications.
  Widget* m_notificationsBox;
  LinkLabel* m_linkLabel;

  // Box with move-pixels commands (when the user drag-and-drop selected pixels using the editor)
  Box* m_movePixelsBox;
  Widget* m_transparentLabel;
  ColorButton* m_transparentColor;

  // Tip window
  Frame* m_tipwindow;

  int m_hot_layer;

  StatusBarListeners m_listeners;
};

#endif
