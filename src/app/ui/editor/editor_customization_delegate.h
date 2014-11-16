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

#ifndef APP_UI_EDITOR_CUSTOMIZATION_DELEGATE_H_INCLUDED
#define APP_UI_EDITOR_CUSTOMIZATION_DELEGATE_H_INCLUDED
#pragma once

namespace tools {
  class Tool;
}

namespace app {
  class Editor;

  class EditorCustomizationDelegate {
  public:
    virtual ~EditorCustomizationDelegate() { }
    virtual void dispose() = 0;

    // Called to know if the user is pressing a keyboard shortcut to
    // select another tool temporarily (a "quick tool"). The given
    // "currentTool" is the current tool selected in the toolbox.
    virtual tools::Tool* getQuickTool(tools::Tool* currentTool) = 0;

    // Returns true if the user wants to copy the selection instead of
    // to move it.
    virtual bool isCopySelectionKeyPressed() = 0;

    // Returns true if the user wants to snap to grid when he's moving
    // the selection.
    virtual bool isSnapToGridKeyPressed() = 0;

    // Returns true if the user wants to activate angle snap, so he can
    // easily specify common angles (45, 90, 135, 180, etc.).
    virtual bool isAngleSnapKeyPressed() = 0;

    // Returns true if the user wants to maintain the aspect ratio when
    // he is scaling the selection.
    virtual bool isMaintainAspectRatioKeyPressed() = 0;

    // Returns true if the user wants to lock the X or Y axis when he is
    // dragging the selection.
    virtual bool isLockAxisKeyPressed() = 0;

    virtual bool isAddSelectionPressed() = 0;

    virtual bool isSubtractSelectionPressed() = 0;

    virtual bool isAutoSelectLayerPressed() = 0;
  };

} // namespace app

#endif  // APP_UI_EDITOR_CUSTOMIZATION_DELEGATE_H_INCLUDED
