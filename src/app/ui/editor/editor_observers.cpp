// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/editor_observers.h"

#include "app/ui/editor/editor_observer.h"
#include "base/bind.h"

namespace app {

EditorObservers::EditorObservers()
{
}

void EditorObservers::addObserver(EditorObserver* observer)
{
  m_observers.addObserver(observer);
}

void EditorObservers::removeObserver(EditorObserver* observer)
{
  m_observers.removeObserver(observer);
}

void EditorObservers::notifyDestroyEditor(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::onDestroyEditor, editor);
}

void EditorObservers::notifyStateChanged(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::onStateChanged, editor);
}

void EditorObservers::notifyScrollChanged(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::onScrollChanged, editor);
}

void EditorObservers::notifyZoomChanged(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::onZoomChanged, editor);
}

void EditorObservers::notifyBeforeFrameChanged(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::onBeforeFrameChanged, editor);
}

void EditorObservers::notifyAfterFrameChanged(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::onAfterFrameChanged, editor);
}

void EditorObservers::notifyBeforeLayerChanged(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::onBeforeLayerChanged, editor);
}

void EditorObservers::notifyAfterLayerChanged(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::onAfterLayerChanged, editor);
}

} // namespace app
