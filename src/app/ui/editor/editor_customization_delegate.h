// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_EDITOR_CUSTOMIZATION_DELEGATE_H_INCLUDED
#define APP_UI_EDITOR_CUSTOMIZATION_DELEGATE_H_INCLUDED
#pragma once

#include "app/ui/keyboard_shortcuts.h"

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

    // Returns what action is pressed at this moment.
    virtual KeyAction getPressedKeyAction(KeyContext context) = 0;
  };

} // namespace app

#endif  // APP_UI_EDITOR_CUSTOMIZATION_DELEGATE_H_INCLUDED
