// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_OBSERVER_H_INCLUDED
#define APP_UI_EDITOR_OBSERVER_H_INCLUDED
#pragma once

namespace app {

class Editor;

class EditorObserver {
public:
  virtual ~EditorObserver() {}
  virtual void dispose() {}

  virtual void onDestroyEditor(Editor* editor) {}

  // Called when the editor's state changes.
  virtual void onStateChanged(Editor* editor) {}

  // Called when the scroll or zoom of the editor changes.
  virtual void onScrollChanged(Editor* editor) {}
  virtual void onZoomChanged(Editor* editor) {}

  // Called when the current frame of the editor changes.
  virtual void onBeforeFrameChanged(Editor* editor) {}
  virtual void onAfterFrameChanged(Editor* editor) {}

  // Called when the current layer of the editor changes.
  virtual void onBeforeLayerChanged(Editor* editor) {}
  virtual void onAfterLayerChanged(Editor* editor) {}
};

} // namespace app

#endif // APP_UI_EDITOR_OBSERVER_H_INCLUDED
