/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JINETE_JWINDOW_H_INCLUDED
#define JINETE_JWINDOW_H_INCLUDED

#include "jinete/jwidget.h"
#include "Vaca/Signal.h"

namespace Vaca {
  class CloseEvent { };		// TODO
}

class Frame : public Widget
{
  JWidget m_killer;
  bool m_is_desktop : 1;
  bool m_is_moveable : 1;
  bool m_is_sizeable : 1;
  bool m_is_ontop : 1;
  bool m_is_wantfocus : 1;
  bool m_is_foreground : 1;
  bool m_is_autoremap : 1;

public:
  Frame(bool is_desktop, const char* text);
  ~Frame();

  Widget* get_killer();

  void set_autoremap(bool state);
  void set_moveable(bool state);
  void set_sizeable(bool state);
  void set_ontop(bool state);
  void set_wantfocus(bool state);

  void remap_window();
  void center_window();
  void position_window(int x, int y);
  void move_window(JRect rect);

  void open_window();
  void open_window_fg();
  void open_window_bg();
  void closeWindow(Widget* killer);

  bool is_toplevel();
  bool is_foreground() const;
  bool is_desktop() const;
  bool is_ontop() const;
  bool is_wantfocus() const;

  // Signals
  Vaca::Signal1<void, Vaca::CloseEvent&> Close;

protected:
  bool onProcessMessage(JMessage msg);
  void onPreferredSize(PreferredSizeEvent& ev);

private:
  void window_set_position(JRect rect);
  int get_action(int x, int y);
  void limit_size(int* w, int* h);
  void move_window(JRect rect, bool use_blit);

};

#endif
