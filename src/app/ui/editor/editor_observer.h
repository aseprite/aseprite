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

#ifndef APP_UI_EDITOR_OBSERVER_H_INCLUDED
#define APP_UI_EDITOR_OBSERVER_H_INCLUDED
#pragma once

namespace app {
  
  class Editor;

  class EditorObserver {
  public:
    virtual ~EditorObserver() { }
    virtual void dispose() = 0;

    // Called when the editor's state changes.
    virtual void onStateChanged(Editor* editor) = 0;

    // Called when the scroll or zoom of the editor changes.
    virtual void onScrollChanged(Editor* editor) = 0;

    // Called when the current frame of the editor changes.
    virtual void onFrameChanged(Editor* editor) = 0;

    // Called when the current layer of the editor changes.
    virtual void onLayerChanged(Editor* editor) = 0;
  };

} // namespace app

#endif  // APP_UI_EDITOR_OBSERVER_H_INCLUDED
