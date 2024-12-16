// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/editor/editor_observers.h"

#include "app/ui/editor/editor_observer.h"

namespace app {

void EditorObservers::notifyDestroyEditor(Editor* editor)
{
  notify_observers(&EditorObserver::onDestroyEditor, editor);
}

void EditorObservers::notifyStateChanged(Editor* editor)
{
  notify_observers(&EditorObserver::onStateChanged, editor);
}

void EditorObservers::notifyScrollChanged(Editor* editor)
{
  notify_observers(&EditorObserver::onScrollChanged, editor);
}

void EditorObservers::notifyZoomChanged(Editor* editor)
{
  notify_observers(&EditorObserver::onZoomChanged, editor);
}

void EditorObservers::notifyBeforeFrameChanged(Editor* editor)
{
  notify_observers(&EditorObserver::onBeforeFrameChanged, editor);
}

void EditorObservers::notifyAfterFrameChanged(Editor* editor)
{
  notify_observers(&EditorObserver::onAfterFrameChanged, editor);
}

void EditorObservers::notifyBeforeLayerChanged(Editor* editor)
{
  notify_observers(&EditorObserver::onBeforeLayerChanged, editor);
}

void EditorObservers::notifyAfterLayerChanged(Editor* editor)
{
  notify_observers(&EditorObserver::onAfterLayerChanged, editor);
}

} // namespace app
