// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_OBSERVERS_H_INCLUDED
#define APP_UI_EDITOR_OBSERVERS_H_INCLUDED
#pragma once

#include "app/ui/editor/editor_observer.h"
#include "obs/observable.h"

namespace app {
class Editor;

class EditorObservers : public obs::observable<EditorObserver> {
public:
  void notifyDestroyEditor(Editor* editor);
  void notifyStateChanged(Editor* editor);
  void notifyScrollChanged(Editor* editor);
  void notifyZoomChanged(Editor* editor);
  void notifyBeforeFrameChanged(Editor* editor);
  void notifyAfterFrameChanged(Editor* editor);
  void notifyBeforeLayerChanged(Editor* editor);
  void notifyAfterLayerChanged(Editor* editor);
};

} // namespace app

#endif // APP_UI_EDITOR_OBSERVERS_H_INCLUDED
